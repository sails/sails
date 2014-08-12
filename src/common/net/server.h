#ifndef SERVER_H
#define SERVER_H
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <memory>
#include <thread>
#include <list>
#include <common/base/util.h>
#include <common/base/string.h>
#include <common/base/thread_pool.h>
#include <common/base/handle.h>
#include <common/base/event_loop.h>
#include <common/net/connector.h>
#include <common/log/logging.h>

namespace sails {
namespace common{
namespace net {

const int MAX_EVENTS = 1000;

template<typename T> class Server;
template<typename T> using HANDLE_CB = void (*)(std::shared_ptr<common::net::Connector<T>> connector, T *message);
template<typename T> using ACCEPTAFTER_CB = void (*)(std::shared_ptr<common::net::Connector<T>> connector, struct sockaddr_in &local);
template<typename T>
class ConnectionnHandleParam {
public:
    ConnectionnHandleParam() {
	connector = NULL;
	packet = NULL;
	conn_fd = 0;
    }
    std::shared_ptr<common::net::Connector<T>> connector;
    T *packet;
    int conn_fd;
};;


template<typename T>
class Server {
public:
    Server();
    ~Server();

    void init(int port=8000, int connector_timeout=10, int work_thread_num=4, int hanle_request_queue_size=1000);
    void set_accept_after_cb(ACCEPTAFTER_CB<T> cb);
    void set_parser_cb(PARSER_CB<T> cb);
    void set_handle_cb(HANDLE_CB<T> cb);
    void set_connector_close_cb(CLOSE_CB<T> cb);

    void addTimer(ExpiryAction action, void *data, int when, int tick=1);

    void start();
    void stop();

    int getListenFd();

private:
    static void accept_socket(common::event* e, int revents);
    static void read_data(common::event*, int revents);

    ACCEPTAFTER_CB<T> accept_after_cb;
    PARSER_CB<T> parser_cb;
    HANDLE_CB<T> handle_cb;
    CLOSE_CB<T> connector_close_cb;

    static void handle(void *message);
    
    
    static void timeout_cb(common::net::Connector<T> *connector);
    static void close_cb(common::net::Connector<T> *connector);
    static void connector_event_stop_cb(common::event* ev);
    static void listen_event_stop_cb(common::event* ev);
    static void delete_connector_cb(common::net::Connector<T>* connector);

private:
    int listenfd;
    EventLoop* ev_loop;
    ConnectorTimeout<T>* connect_timer;
    int work_thread_num;
    int hanle_request_queue_size;
    log::Logger *log;
    long drop_packet_num;
    std::list<Timer*> timerList;
};




template<typename T>
Server<T>::Server()
{
    this->work_thread_num = 0;
    this->hanle_request_queue_size = 0;
    this->listenfd = 0;
    this->accept_after_cb = 0;
    this->parser_cb = 0;
    this->handle_cb = 0;
    this->connector_close_cb = 0;
    this->drop_packet_num = 0;
}

template<typename T>
Server<T>::~Server() {
    delete ev_loop;
    this->ev_loop = NULL;
    delete connect_timer;
    this->connect_timer = NULL;
    delete log;
    this->log = NULL;
    while(!timerList.empty()) {
	Timer* timer = timerList.front();
	timerList.pop_front();
	delete timer;
    }
}

template<typename T>
void Server<T>::init(int port, int connector_timeout, int work_thread_num, int hanle_request_queue_size) {

    this->work_thread_num = work_thread_num;
    this->hanle_request_queue_size = hanle_request_queue_size;
    log = new log::Logger(common::log::Logger::LOG_LEVEL_DEBUG,
			  "./log/sails.log", common::log::Logger::SPLIT_DAY);
	
    ev_loop = new EventLoop();
    ev_loop->init();
    connect_timer = new ConnectorTimeout<T>(connector_timeout);
    connect_timer->init(ev_loop);
    
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("create listen socket");
	exit(EXIT_FAILURE);
    }
    struct sockaddr_in servaddr, local;
    int addrlen = sizeof(struct sockaddr_in);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    int flag=1,len=sizeof(int); // for can restart right now
    if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1) 
    {
	perror("setsockopt"); 
	exit(EXIT_FAILURE); 
    }
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    sails::common::setnonblocking(listenfd);

    if(listen(listenfd, 10) < 0) {
	perror("listen");
	exit(EXIT_FAILURE);
    }

    sails::common::event listen_ev;
    emptyEvent(listen_ev);
    listen_ev.fd = listenfd;
    listen_ev.events = sails::common::EventLoop::Event_READ;
    listen_ev.cb = Server::accept_socket;
    listen_ev.stop_cb = listen_event_stop_cb;

    listen_ev.data = this;

    if(!ev_loop->event_ctl(sails::common::EventLoop::EVENT_CTL_ADD,
				&listen_ev)) {
	fprintf(stderr, "add listen fd to event loop fail");
        exit(EXIT_FAILURE);
    }

}


template<typename T>
void Server<T>::set_accept_after_cb(ACCEPTAFTER_CB<T> cb) {
    this->accept_after_cb = cb;
}

template<typename T>
void Server<T>::set_parser_cb(PARSER_CB<T> cb) {
    this->parser_cb = cb;
}

template<typename T>
void Server<T>::set_connector_close_cb(CLOSE_CB<T> cb) {
    this->connector_close_cb = cb;
}

template<typename T>
void Server<T>::set_handle_cb(HANDLE_CB<T>  cb) {
    this->handle_cb = cb;
}

template<typename T>
void Server<T>::addTimer(ExpiryAction action, void *data, int when, int tick) {
    Timer* timer = new Timer(this->ev_loop, tick);
    timer->init(action, data, when);
    this->timerList.push_back(timer);
}

template<typename T>
void Server<T>::start() {
    ev_loop->start_loop();
}

template<typename T>
void Server<T>::stop() {
    ev_loop->stop_loop();
    ev_loop->delete_all_event();
}

template<typename T>
int Server<T>::getListenFd() {
    return this->listenfd;
}



template<typename T>
void Server<T>::accept_socket(common::event* e, int revents) {
    if(revents & common::EventLoop::Event_READ) {
	struct sockaddr_in local;
	int addrlen = sizeof(struct sockaddr_in);
        Server* server = (Server*)e->data;
	for (;;) {
	    memset(&local, 0, addrlen);

	    int connfd = -1;
	    do {
	        connfd = accept(e->fd, 
				(struct sockaddr*)&local, 
				(socklen_t*)&addrlen);
	    }while((connfd < 0) && (errno == EINTR));

	    if (connfd > 0) {
		sails::common::setnonblocking(connfd);

		sails::common::event ev;
		emptyEvent(ev);
		ev.fd = connfd;
		ev.events = sails::common::EventLoop::Event_READ;
		ev.cb = Server::read_data;

		// set timeout
		std::shared_ptr<common::net::Connector<T>> connector (new common::net::Connector<T>(connfd));
		connector->setServer(server);
		connector->set_parser_cb(server->parser_cb);
		connector->set_delete_cb(Server::delete_connector_cb);
		connector->set_close_cb(Server::close_cb);
		connector->set_timeout_cb(Server::timeout_cb);
	        server->connect_timer->update_connector_time(connector);


		SharedPtrAdapter<Connector<T>>* connectorAdapter = new SharedPtrAdapter<Connector<T>>();
		connectorAdapter->ptr = connector;

		ev.data = connectorAdapter;
		ev.stop_cb = connector_event_stop_cb;
		if(!server->ev_loop->event_ctl(common::EventLoop::EVENT_CTL_ADD, &ev)){
		    //do noting, connector delete will close fd
		}
		if (server->accept_after_cb != NULL) {
		    server->accept_after_cb(connector, local);
		}
	    }else {
		break;
	    }
	}
    }
}

template<typename T>
void Server<T>::read_data(common::event* ev, int revents) {

    if(ev == NULL || ev->fd < 0) {
	return;
    }
    int connfd = ev->fd;
    common::net::SharedPtrAdapter<common::net::Connector<T>>* connectorAdapter = (common::net::SharedPtrAdapter<common::net::Connector<T>>*)ev->data;
    std::shared_ptr<common::net::Connector<T>> connector = connectorAdapter->ptr;


    if (connector == NULL || connector.use_count() <= 0) {
	return;
    }

    bool readerror = false;

    // read nonblock connfd
    for(;;) {
	int lasterror = 0;
	int n = 0;
	do {
	    n = connector->read();
	    lasterror = errno;
	}while((n == -1) && (lasterror == EINTR)); // The  call  was interrupted by a signal before any data was read
	

	if (n > 0) {
	    if (n < READBYTES) { // no data
		break;
	    }else {
		continue;
	    }

	}else if (n == 0) { // client close or shutdown send, and there is no error,  errno will not reset, so don't print errno
	    readerror = true;
	     char errormsg[100];
	    memset(errormsg, '\0', 100);
	    sprintf(errormsg, "read connfd %d, return:%d", connfd, n);
	    perror(errormsg);
	    break;
	}else if (n == -1) {
	    if (lasterror == EAGAIN || lasterror == EWOULDBLOCK) { // no data
		break;
	    }else { // read fault
		readerror = true;
		char errormsg[100];
		memset(errormsg, '\0', 100);
		sprintf(errormsg, "read connfd %d, return:%d, errno:%d", connfd, n, lasterror);
		perror(errormsg);
		break;
	    }
	}
    }
    if (readerror) {
	if (!connector->isClosed()) {
	    connector->close();
	}
    }

    else {
	
        connector->getServer()->connect_timer->update_connector_time(connector);// update timeout
	connector->parser();
	if (! connector->isClosed()) {
	    T *packet = NULL;

	    while((packet=connector->get_next_packet()) != NULL) {
		ConnectionnHandleParam<T> *param = new ConnectionnHandleParam<T>();
		param->connector = connector;
		param->packet = packet;
		param->conn_fd = connfd;
		// use thread pool to handle request
		static common::ThreadPool parser_pool(connector->getServer()->work_thread_num, connector->getServer()->hanle_request_queue_size);	
		common::ThreadPoolTask task;
		task.fun = Server::handle;
		task.argument = param;
		if (parser_pool.add_task(task) == -1) {
		    connector->getServer()->drop_packet_num++;
		}else {
		}
	    }
	}
    }
    
}


template<typename T>
void Server<T>::handle(void* message) {
    ConnectionnHandleParam<T> *param = NULL;
    param = (ConnectionnHandleParam<T> *)message;
    
    if (param != NULL){
	std::shared_ptr<common::net::Connector<T>> connector = param->connector;
	if (connector->getServer() != NULL && connector->getServer()->handle_cb != NULL) {
	    T * msg = param->packet;
	    connector->getServer()->handle_cb(connector, msg);
	    free(msg);
	}

        delete param;
	param = NULL;

    }
}


template<typename T>
void Server<T>::timeout_cb(common::net::Connector<T> *connector) {

    if (!connector->isClosed()) {
	connector->close();
    }

}

template<typename T>
void Server<T>::close_cb(common::net::Connector<T> *connector) {
    connector->getServer()->ev_loop->event_stop(connector->get_connector_fd());
    if (connector->getServer()->connector_close_cb != 0) {
        connector->getServer()->connector_close_cb(connector);
    }

}

template<typename T>
void Server<T>::connector_event_stop_cb(common::event* ev) {
    if (ev->data != NULL) {
	common::net::SharedPtrAdapter<common::net::Connector<T>>* connectorAdapter = (common::net::SharedPtrAdapter<common::net::Connector<T>>*)ev->data;
	delete connectorAdapter;
        ev->data = NULL;
    }
}

template<typename T>
void Server<T>::listen_event_stop_cb(common::event* ev) {
    if (ev->data != NULL) {
	printf("delete listen event\n");
        ev->data = NULL;
    }
}

template<typename T>
void Server<T>::delete_connector_cb(common::net::Connector<T>* connector) {
    printf("delete connector\n");
}




} // namespace net
} // namepsace common
} // namepsace sails

#endif /* SERVER_H */

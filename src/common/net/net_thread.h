#ifndef NET_THREAD_H
#define NET_THREAD_H

#include <memory>
#include <thread>
#include <common/base/util.h>
#include <common/base/thread_queue.h>
#include <common/base/event_loop.h>
#include <common/net/connector_list.h>


namespace sails {
namespace common {
namespace net {


template <typename T> class EpollServer;


// 定义数据队列中的结构
template <typename T>
struct TagRecvData
{
    uint32_t        uid;           // 连接标示
    T*              data;          // 接收到的内容
    std::string     ip;            // 远程连接的ip
    uint16_t        port;          // 远程连接的端口
    int             fd;
};

struct TagSendData
{
    char            cmd;            // 命令:'c',关闭fd; 's',有数据需要发送
    uint32_t        uid;            // 连接标示
    std::string     buffer;         // 需要发送的内容
    std::string     ip;             // 远程连接的ip
    uint16_t        port;           // 远程连接的端口
};

template <typename T>
using recv_queue = ThreadQueue<TagRecvData<T>*, std::deque<TagRecvData<T>*>>;
typedef ThreadQueue<TagSendData*, std::deque<TagSendData*>> send_queue;


template <typename T>
class NetThread {
public:

    // 链接状态
    struct ConnStatus
    {
	std::string     ip;
	int32_t         uid;
	uint16_t        port;
	int             timeout;
	int             iLastRefreshTime;
    };

    // 网络线程运行状态
    enum RunStatus {
	RUNING,
	PAUSE,
	STOPING
    };

    // 状态
    struct ThreadStatus {
	int thread_num;
	RunStatus status;
	long run_time;
	int listen_port;	// only for recv accept
	int connector_num;	
    };

    // 监听端口信息
    struct BindSocketInfo {
	int listen_port;
	long accept_times;
    };
    
    NetThread(EpollServer<T> *server);
    virtual ~NetThread();
    
    // 创建一个epoll的事件循环
    void create_event_loop();
    
    EventLoop* get_event_loop() { return this->ev_loop; }

    // 监听端口
    int bind(int port);

    // 创建一个connector超时管理器
    void setEmptyConnTimeout(int connector_read_timeout=10);

    static void timeoutCb(common::net::Connector* connector);

    static void startEvLoop(NetThread<T>* netThread);
    

    void run();

    void terminate();

    bool isTerminate() { return status == STOPING;}
    
    void join();

    // 接收连接请求
    static void accept_socket_cb(common::event* e, int revents, void* owner);
    // 增加connector
    void add_connector(std::shared_ptr<common::net::Connector> connector);
    // 接收连接数据
    static void read_data_cb(common::event* e, int revents, void* owner);
    
    static void read_pipe_cb(common::event* e, int revents, void* owner);
    
    // 获取连接数
    size_t get_connector_count() { return connector_list.size();}


    // 接收队列大小
    size_t get_recvqueue_size();

    // 发送队列大小
    size_t get_sendqueue_size();

    // 用于io线程自身解析完之后
    void addRecvList(TagRecvData<T> *data);

    // 用于dispacher线程
    void getRecvData(TagRecvData<T>* &data, int millisecond);

    // 发送数据,把data放入一个send list中,然后再触发epoll的可写事件
    void send(const std::string &ip, uint16_t port,int uid, const std::string &data);
    
    // 关闭连接
    void close_connector(const std::string &ip, uint16_t port, int uid, int fd);
    
    // 获取服务
    EpollServer<T>* getServer();
protected:
    void accept_socket(common::event* e, int revents);
    
    void read_data(common::event* e, int revents);
    
private:
    EpollServer<T> *server;
    // 接收的数据队列
    recv_queue<T> recvlist;

    // 发送的数据队列
    send_queue sendlist;

    int status;
    std::thread *thread;
    int listenfd;
    int listen_port;
    
    EventLoop *ev_loop; 	// 事件循环

    ConnectorTimeout* connect_timer; // 连接超时器
    std::list<Timer*> timerList;	// 定时器
    
    ConnectorList connector_list; 
    
    int shutdown;		// 管道(用于关闭服务)
    int notify;			// 管道(用于通知有数据要发送)
    
};




template <typename T>
NetThread<T>::NetThread(EpollServer<T> *server) {
    this->server = server;
    status = NetThread::STOPING;
    thread = NULL;
    listenfd = 0;
    listen_port = 0;
    ev_loop = NULL;
    connector_list.init(10000);
    connect_timer = NULL;
    notify = socket(AF_INET, SOCK_STREAM, 0);
}


template <typename T>
NetThread<T>::~NetThread() {
    if (status != NetThread::STOPING) {
	this->terminate();
	this->join();
	delete thread;
	thread = NULL;
    }
    if (connect_timer != NULL) {
	delete connect_timer;
	connect_timer = NULL;
    }

    if (ev_loop != NULL) {
	delete ev_loop;
	ev_loop = NULL;
    }

}


template <typename T>
void NetThread<T>::create_event_loop() {
    ev_loop = new EventLoop(this); 
    ev_loop->init();

    // 创建 notify 事件
    sails::common::event notify_ev;
    emptyEvent(notify_ev);
    notify_ev.fd = notify;
    notify_ev.events = sails::common::EventLoop::Event_READ;
    notify_ev.cb = NetThread<T>::read_pipe_cb;

    assert(ev_loop->event_ctl(EventLoop::EVENT_CTL_ADD,
			      &notify_ev));


}

template <typename T>
int NetThread<T>::bind(int port) {
     
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
    ::bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    sails::common::setnonblocking(listenfd);

    if(listen(listenfd, 10) < 0) {
	perror("listen");
	exit(EXIT_FAILURE);
    }

    sails::common::event listen_ev;
    emptyEvent(listen_ev);
    listen_ev.fd = listenfd;
    listen_ev.events = sails::common::EventLoop::Event_READ;
    listen_ev.cb = NetThread<T>::accept_socket_cb;

    if(!ev_loop->event_ctl(sails::common::EventLoop::EVENT_CTL_ADD,
				&listen_ev)) {
	fprintf(stderr, "add listen fd to event loop fail");
        exit(EXIT_FAILURE);
    }

    
    return listenfd;
}

template <typename T>
void NetThread<T>::accept_socket_cb(common::event* e, int revents, void* owner) {
    if (owner != NULL) {
	NetThread<T>* net_thread = (NetThread<T>*)owner;
	net_thread->accept_socket(e, revents);
    }
}


template <typename T>
void NetThread<T>::accept_socket(common::event* e, int revents) {
    if(revents & common::EventLoop::Event_READ) {
	struct sockaddr_in local;
	int addrlen = sizeof(struct sockaddr_in);
	
	//循环accept,因为使用的是epoll,当多个连接同时连上时,只通知一次
	for (;;) {
	    memset(&local, 0, addrlen);

	    int connfd = -1;
	    do {
	        connfd = accept(e->fd, 
				(struct sockaddr*)&local, 
				(socklen_t*)&addrlen);
	    }while((connfd < 0) && (errno == EINTR));// 被中断

	    if (connfd > 0) {
		sails::common::setnonblocking(connfd);

		// 新建connector
		std::shared_ptr<common::net::Connector> connector(new common::net::Connector(connfd));
		uint32_t uid = connector_list.getUniqId();
		connector->setId(uid);
		int port = ntohs(local.sin_port);
		connector->setPort(port);
		char sAddr[20] = {'\0'};
		inet_ntop(AF_INET, &(local.sin_addr), sAddr, 20);
		std::string ip = sAddr;
		connector->setIp(ip);
		connector->setTimeoutCB(NetThread<T>::timeoutCb);

		server->addConnector(connector, connfd);

		
	    }else {
		break;
	    }
	}
    }

}



// 增加connector
template <typename T>
void NetThread<T>::add_connector(std::shared_ptr<common::net::Connector> connector) {
    
    connector->owner = this;
    connector_list.add(connector);
    connect_timer->update_connector_time(connector);
    
    
    // 加入event poll中
    sails::common::event ev;
    emptyEvent(ev);
    ev.fd = connector->get_connector_fd();
    ev.events = sails::common::EventLoop::Event_READ;
    ev.cb = NetThread<T>::read_data_cb;
    ev.data.u32 = connector->getId();
    
    if(!ev_loop->event_ctl(common::EventLoop::EVENT_CTL_ADD, &ev)){
	connector_list.del(connector->getId());
    }
}


template <typename T>
void NetThread<T>::read_data_cb(common::event* e, int revents, void* owner) {
    if (owner != NULL) {
	NetThread<T>* net_thread = (NetThread<T>*)owner;
	net_thread->read_data(e, revents);
    }
}

template<typename T>
size_t NetThread<T>::get_recvqueue_size() {
    return this->recvlist.size();
}


template<typename T>
size_t NetThread<T>::get_sendqueue_size() {
    return this->sendlist.size();
}

template <typename T>
void NetThread<T>::read_data(common::event* ev, int revents) {
    
    if(ev == NULL || ev->fd < 0) {
	return;
    }

    uint32_t uid = ev->data.u32;

    std::shared_ptr<common::net::Connector> connector = connector_list.get(uid);


    if (connector == NULL || connector.use_count() <= 0) {
	return;
    }

    bool readerror = false;

    // read nonblock connfd
    int totalNum = 0;
    while(true) {
	int lasterror = 0;
	int n = 0;
	do {
	    n = connector->read();
	    lasterror = errno;
	}while((n == -1) && (lasterror == EINTR)); // read 调用被信号中断
	

	if (n > 0) {
	    totalNum+=n;
	    if (totalNum >= 4096) { // 大于4k就开始解析,防止数据过多
		this->server->parseImp(connector);
	    }
	    if (n < READBYTES) { // no data
		break;
	    }else {
		continue;
	    }

	}else if (n == 0) { // client close or shutdown send, and there is no error,  errno will not reset, so don't print errno
	    readerror = true;
	     char errormsg[100];
	    memset(errormsg, '\0', 100);
	    sprintf(errormsg, "read connfd %d, return:%d", connector->get_connector_fd(), n);
	    perror(errormsg);
	    break;
	}else if (n == -1) {
	    if (lasterror == EAGAIN || lasterror == EWOULDBLOCK) { // 没有数据
		break;
	    }else { // read fault
		readerror = true;
		char errormsg[100];
		memset(errormsg, '\0', 100);
		sprintf(errormsg, "read connfd %d, return:%d, errno:%d", connector->get_connector_fd(), n, lasterror);
		perror(errormsg);
		break;
	    }
	}
    }

    if (readerror) {
	if (!connector->isClosed()) {
	    close_connector(connector->getIp(), connector->getPort(), connector->getId(), connector->get_connector_fd());
	}
    }

    else {
	
        connect_timer->update_connector_time(connector);// update timeout
	this->server->parseImp(connector);
    }
    
}



template <typename T>
void NetThread<T>::read_pipe_cb(common::event* e, int revents, void* owner) {
    NetThread<T>* net_thread = NULL;
    if (owner != NULL) {
	net_thread = (NetThread<T>*)owner;
	if (net_thread == NULL) {
	    return ;
	}
    }else {
	return;
    }

    TagSendData* data = NULL;
    do {
	data = NULL;
	net_thread->sendlist.pop_front(data, 0);

	if (data != NULL) {
	    int uid = data->uid;
	    std::string ip = data->ip;
	    uint16_t port = data->port;

	    char cmd = data->cmd;
	    if (cmd == 's') {
		std::shared_ptr<Connector> connector = net_thread->connector_list.get(uid);
		if (connector != NULL) {
		    // 判断是否是正确的连接
		    if (connector->getPort() == port && connector->getIp() == ip) {
			connector->write( data->buffer.c_str(), data->buffer.length());
			connector->send();
		    }
		}
	    }else if (cmd == 'c') {
		std::shared_ptr<Connector> connector = net_thread->connector_list.get(uid);
		if (connector != NULL) {
		    if (connector->getPort() == port && connector->getIp() == ip) {			// 从event loop中删除
			net_thread->ev_loop->event_stop(connector->get_connector_fd());
			connector->close();
			net_thread->connector_list.del(uid);
		    }
		}
	    }
	    

	}
    }while(data != NULL);

}


template <typename T>
void NetThread<T>::setEmptyConnTimeout(int connector_read_timeout) {
    int timeout = connector_read_timeout>0?connector_read_timeout:10;
    connect_timer = new ConnectorTimeout(timeout);
    connect_timer->init(ev_loop);

}

template <typename T>
void NetThread<T>::timeoutCb(common::net::Connector* connector) {
    if (connector != NULL && connector->owner != NULL) {
	NetThread<T>* netThread = (NetThread<T>*)connector->owner;
	netThread->close_connector(connector->getIp(), connector->getPort(), connector->getId(), connector->get_connector_fd());
    }
}

template <typename T>
EpollServer<T>* NetThread<T>::getServer() {
    return this->server;
}

template <typename T>
void NetThread<T>::startEvLoop(NetThread<T>* netThread) {
    netThread->setEmptyConnTimeout();
    netThread->ev_loop->start_loop();
}

template <typename T>
void NetThread<T>::run() {
    thread = new std::thread(startEvLoop, this);
    status = NetThread::RUNING;
}

template <typename T>
void NetThread<T>::terminate() {
    if (thread != NULL) {
	// 向epoll管理的0号连接发一个终止事件,让epoll wait结束,然后再退出
	ev_loop->stop_loop();
    }
}


template <typename T>
void NetThread<T>::addRecvList(TagRecvData<T> *data) {
    recvlist.push_back(data);
    server->notify_dispacher();
}

template <typename T>
void NetThread<T>::getRecvData(TagRecvData<T>* &data, int millisecond) {
    recvlist.pop_front(data, millisecond);
}

template <typename T>
void NetThread<T>::send(const std::string &ip, uint16_t port, int uid,const std::string &s) {
    TagSendData* data = new TagSendData();
    data->cmd = 's';
    data->uid = uid;
    data->buffer = s;
    data->ip = ip;
    data->port = port;
    sendlist.push_back(data);
    // 通知epoll_wait
    sails::common::event notify_ev;
    emptyEvent(notify_ev);
    notify_ev.fd = notify;
    notify_ev.events = sails::common::EventLoop::Event_WRITE;
    notify_ev.cb = NetThread<T>::read_pipe_cb;
    ev_loop->event_ctl(sails::common::EventLoop::EVENT_CTL_MOD, &notify_ev);
}

template <typename T>
void NetThread<T>::close_connector(const std::string &ip, uint16_t port, int uid, int fd) {
    TagSendData* data = new TagSendData();
    data->cmd = 'c';
    data->uid = uid;
    data->ip = ip;
    data->port = port;
    sendlist.push_back(data);

    // 通知epoll_wait
    sails::common::event notify_ev;
    emptyEvent(notify_ev);
    notify_ev.fd = notify;
    notify_ev.events = sails::common::EventLoop::Event_WRITE;
    notify_ev.cb = NetThread<T>::read_pipe_cb;
    ev_loop->event_ctl(sails::common::EventLoop::EVENT_CTL_MOD, &notify_ev);
}





template <typename T>
void NetThread<T>::join() {
    if (thread != NULL) {
	thread->join();
	status = NetThread::STOPING;
	delete thread;
	thread = NULL;
    }
}



} // namespace net
} // namespace common
} // namespace sails 



#endif /* NET_THREAD_H */

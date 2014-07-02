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
#include "config.h"
#include "connection.h"
#include "module_load.h"
#include <signal.h>
#include <common/base/util.h>
#include <common/base/string.h>
#include <common/base/event_loop.h>
#include <common/net/connector.h>
//#include <common/net/http_connector.h>
//#include <common/net/com_connector.h>
const int MAX_EVENTS = 1000;

namespace sails {

common::EventLoop ev_loop;
common::net::ConnectorTimeout<common::net::PacketCommon> connect_timer(10);
Config config;
std::map<std::string, std::string> modules;


void init_config(int argc, char *argv[]) {
    config.get_modules(&modules);
}

int register_service() {
    std::map<std::string, std::string>::iterator iter;
    for(iter = modules.begin(); iter != modules.end()
	    ; iter++) {
	if(!iter->second.empty()) {
	    ModuleLoad::load(iter->second);
	}
    }     
    return 0;
}

void accept_socket(common::event* e, int revents) {
    if(revents & common::EventLoop::Event_READ) {
	struct sockaddr_in local;
	int addrlen = sizeof(struct sockaddr_in);
	int connfd = accept(e->fd, 
			    (struct sockaddr*)&local, 
			    (socklen_t*)&addrlen);
	if (connfd == -1) {
	    perror("accept");
	    exit(EXIT_FAILURE);
	}
	sails::common::setnonblocking(connfd);

	sails::common::event ev;
	ev.fd = connfd;
	ev.events = sails::common::EventLoop::Event_READ;
	ev.cb = sails::read_data;

	// set timeout
//	common::net::HttpConnector *con = new common::net::HttpConnector(connfd);
//	common::net::ComConnector *con = new common::net::ComConnector(connfd);
	common::net::Connector<common::net::PacketCommon> *con = new common::net::Connector<common::net::PacketCommon>(connfd);
	con->set_parser_fun(parser_cb);
	
	connect_timer.update_connector_time(con);

	ev.data = con;
	ev.next = NULL;
	if(!ev_loop.event_ctl(common::EventLoop::EVENT_CTL_ADD, &ev)){
	    close(connfd);
	    delete (sails::common::net::Connector<common::net::PacketCommon>*)ev.data;
	}
    }
}

void sails_init(int argc, char *argv[]) {
    sails::ev_loop.init();
    connect_timer.init(&ev_loop);
    init_config(argc, argv);
    if (register_service() != 0) {
	perror("register service");
	exit(EXIT_FAILURE);
    }
    Connection::set_max_connectfd(config.get_max_connfd());
}

void sails_exit() {
    modules.clear();
    ModuleLoad::unload();

    printf("on exit\n");
    exit(EXIT_SUCCESS);
}

void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
	    sails_exit();
	    break;
	}
    }
}

} //namespace sails




int main(int argc, char *argv[]) {
    // configure
    sails::sails_init(argc, argv);

    // 
    struct sigaction act;
    act.sa_sigaction = sails::sails_signal_handle;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) == -1) {
	perror("sigaction error");
	exit(EXIT_FAILURE);
    }

    int port = sails::config.get_listen_port();
    if(port < 8000) {
	printf("port must be more than 8000\n");
	return 1;
    }
    int  listenfd = 0;
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
    listen_ev.fd = listenfd;
    listen_ev.events = sails::common::EventLoop::Event_READ;
    listen_ev.cb = sails::accept_socket;
    listen_ev.next = NULL;

    if(!sails::ev_loop.event_ctl(sails::common::EventLoop::EVENT_CTL_ADD,
				&listen_ev)) {
	fprintf(stderr, "add listen fd to event loop fail");
        exit(EXIT_FAILURE);
    }

    sails::ev_loop.start_loop();
}











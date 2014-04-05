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
#include "util.h"
#include "connection.h"
#include "module_load.h"
#include <signal.h>
#include <common/net/event_loop.h>
const int MAX_EVENTS = 1000;

namespace sails {

common::net::EventLoop ev_loop;
Config config;
std::map<std::string, std::string> modules;

void init_config(int argc, char *argv[]) {
     modules = config.get_modules();
}

int register_service() {
     ModuleLoad module_load;
     std::map<std::string, std::string>::iterator iter;
     for(iter = modules.begin(); iter != modules.end()
	      ; iter++) {
	  if(!iter->second.empty()) {
	       module_load.load(iter->second);
	  }
     }     
     return 0;
}

void accept_socket(common::net::event* e, int revents) {
    if(revents & common::net::EventLoop::Event_READ) {
	struct sockaddr_in local;
	int addrlen = sizeof(struct sockaddr_in);
	int connfd = accept(e->fd, 
			    (struct sockaddr*)&local, 
			    (socklen_t*)&addrlen);
	if (connfd == -1) {
	    perror("accept");
	    exit(EXIT_FAILURE);
	}
	sails::setnonblocking(connfd);

	sails::common::net::event ev;
	ev.fd = connfd;
	ev.events = sails::common::net::EventLoop::Event_READ;
	ev.cb = sails::read_data;
	ev.next = NULL;
	ev_loop.event_ctl(common::net::EventLoop::EVENT_CTL_ADD, &ev);
    }
}

void sails_init(int argc, char *argv[]) {
     init_config(argc, argv);
     if (register_service() != 0) {
	  perror("register service");
	  exit(EXIT_FAILURE);
     }
     Connection::set_max_connectfd(config.get_max_connfd());
}

} //namespace sails




int main(int argc, char *argv[]) {
     // configure
     sails::sails_init(argc, argv);

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
     sails::setnonblocking(listenfd);

     if(listen(listenfd, 10) < 0) {
	  perror("listen");
	  exit(EXIT_FAILURE);
     }

     sails::common::net::event listen_ev;
     listen_ev.fd = listenfd;
     listen_ev.events = sails::common::net::EventLoop::Event_READ;
     listen_ev.cb = sails::accept_socket;
     listen_ev.next = NULL;

     sails::ev_loop.init();
     sails::ev_loop.event_ctl(sails::common::net::EventLoop::EVENT_CTL_ADD,
	 &listen_ev);

     sails::ev_loop.start_loop();
}

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
#define MAX_EVENTS 1000

namespace sails {

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

     int epollfd = epoll_create(10);
     if(epollfd == -1) {
	  perror("epoll_create");
	  exit(EXIT_FAILURE);
     }
     struct epoll_event ev, events[MAX_EVENTS];
     ev.events = EPOLLIN;
     ev.data.fd = listenfd;

     if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev)) {
	  perror("epoll_ctl:listen_sock");
	  exit(EXIT_FAILURE);
     }

     for (;;) {
	  int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
	  if (nfds == -1) {
	       perror("epoll_pwait");
	       exit(EXIT_FAILURE);
	  }
	  for (int n = 0; n < nfds; n++) {
	       if (events[n].data.fd == listenfd) {
		    int connfd = accept(listenfd, 
					(struct sockaddr*)&local, (socklen_t*)&addrlen);
		    if (connfd == -1) {
			 perror("accept");
			 exit(EXIT_FAILURE);
		    }
		    sails::setnonblocking(connfd);
		    ev.events = EPOLLIN | EPOLLET;
		    ev.data.fd = connfd;
		    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) == -1) {
			 perror("epoll_ctl: connfd");
			 exit(EXIT_FAILURE);
		    }
	       } else {
		    sails::read_data(events[n].data.fd);
	       }
	  }
     }
     return 0;
}



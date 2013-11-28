#include "connection.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

namespace sails {


void Connection::accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	if(EV_ERROR & revents) {
		printf("accept error!\n");
		return;
	}

	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(struct sockaddr);
	int listenfd = watcher->fd;
	struct ev_io *recv_w = (struct ev_io *)malloc(sizeof(struct ev_io));
	int connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &len);
	printf("accept connection and connfd:%d\n", connfd);
	fcntl(connfd, F_SETFL, O_NONBLOCK);
	
	ev_io_init(recv_w, sails::Connection::recv_cb, connfd, EV_READ);
	ev_io_start(loop, recv_w);
	
}

void Connection::recv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents) {
	if(EV_ERROR & revents) {
		printf("recv error!\n");
		return;
	}
	if(!EV_READ) {
		printf("not readable \n");
		return;
	}

	int connfd = watcher->fd;
	int len = 10 * 1024;
	char buf[len];
	int n = 0;

	memset(buf, 0, len);
	n = read(connfd, buf, len);

	if(n == 0) {
		//client close connection
		ev_io_stop(loop, watcher);
		free(watcher);
		printf("n 0 and free watcher for connfd:%d\n", connfd);
		return;
	}

	if(n == -1) {
		//error
		ev_io_stop(loop, watcher);
		free(watcher);
		printf("n -1 and free watcher for connfd:%d \n", connfd);
		return;
	}
	
	if(strlen(buf) == 0) {
		printf("read 0 byte \n");
		return;
	}else {
		printf("read buf :%s", buf);
		HttpHandle http_handle;
	        size_t size = http_handle.parser_http(buf);
	}
}

} //namespace sails

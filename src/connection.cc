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
#include <iostream>
#include "connection.h"
#include "util.h"
#include "thread_pool.h"
#include "request.h"
#include "response.h"
#include "filter.h"
#include "filter_default.h"
#include "handle.h"
#include "handle_default.h"
#include "handle_proto_decode.h"

namespace sails {

void Connection::accept_cb(struct ev_loop *loop, struct ev_io *watcher,
			   int revents) {
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

void Connection::recv_cb(struct ev_loop *loop, struct ev_io *watcher, 
			 int revents) {
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
	char *buf = (char *)malloc(len);
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

		ConnectionnHandleParam *param = (ConnectionnHandleParam *)malloc(sizeof(ConnectionnHandleParam));
		param->message = buf;
		param->connfd = watcher->fd;

		// use thread pool to parser http from string buf
		static ThreadPool parser_pool(2, 100);	
		ThreadPoolTask task;
		task.fun = Connection::handle;
		task.argument = param;
		parser_pool.add_task(task);
	}
}

void Connection::handle(void *message) 
{
	HttpHandle http_handle(HTTP_REQUEST);
	ConnectionnHandleParam *param = NULL;
	param = (ConnectionnHandleParam *)message;
	if(param == 0) {
		return;
	}

	// http_parser
	size_t size = http_handle.parser_http(param->message);

	Request *request = new Request(&http_handle.msg);
	Response *response = new Response();
	printf("param->connfd:%d\n", param->connfd);
	response->connfd = param->connfd;
	

	// filter chain
	FilterChain<Request*, Response*> chain;
	FilterDefault *default_filter = new FilterDefault();
	chain.add_filter(default_filter);

	chain.do_filter(request, response);

	// handle chain
	HandleChain<Request*, Response*> handle_chain;
	HandleDefault *default_handle = new HandleDefault();
	handle_chain.add_handle(default_handle);
	HandleProtoDecode *proto_decode = new HandleProtoDecode();
	handle_chain.add_handle(proto_decode);
	handle_chain.do_handle(request, response);

	// out put
	response->to_str();
	printf("response:%s\n", response->get_raw());
	int n = write(response->connfd, response->get_raw(), 
		      strlen(response->get_raw()));
	if(request->raw_data->should_keep_alive != 1) {
		close(response->connfd);
	}else {
		
	}


	if(param != NULL) {
		free(param->message);
		param->message = NULL;
		free(param);
		param = NULL;
	}

	delete request;
	delete response;

}
	
} // namespace sails


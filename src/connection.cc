#include "connection.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "connection.h"
#include "thread_pool.h"
#include "filter.h"
#include "filter_default.h"
#include "handle.h"
#include "handle_default.h"
#include "handle_proto_decode.h"
#include "config.h"

namespace sails {


extern Config config;
extern common::net::EventLoop ev_loop;

//void read_data(int connfd) {
void read_data(common::net::event* ev, int revents) {
    if(ev == NULL || ev->fd < 0) {
	return;
    }
    int connfd = ev->fd;

    common::net::HttpConnector *connector = (common::net::HttpConnector *)ev->data;
    int n = connector->read();
    if(n == 0 || n == -1) { // n=0: client close or shutdown send
	                     // n=1: recv signal_pending before read data
	  perror("read connfd");
	  delete(connector);
	  ev_loop.event_stop(connfd);
	  close(connfd); // will delete from epoll set auto
	  return;
     }
    connector->httpparser();

    common::net::HttpRequest *request = NULL;

    while((request=connector->get_next_httprequest()) != NULL) {
	ConnectionnHandleParam *param = (ConnectionnHandleParam *)malloc(sizeof(ConnectionnHandleParam));
	param->request = request;
	param->conn_fd = connfd;
	
	// use thread pool to handle request
	static ThreadPool parser_pool(config.get_handle_thread_pool(),
				      config.get_handle_request_queue_size());	
	ThreadPoolTask task;
	task.fun = Connection::handle;
	task.argument = param;
	parser_pool.add_task(task);
    }
}

void Connection::handle(void *message) 
{
	ConnectionnHandleParam *param = NULL;
	param = (ConnectionnHandleParam *)message;
	if(param == 0) {
		return;
	}
	
        common::net::HttpRequest *request = param->request;
        common::net::HttpResponse *response = new common::net::HttpResponse();
	response->connfd = param->conn_fd;
	
	printf("filter start");
	// filter chain
	FilterChain<common::net::HttpRequest*, common::net::HttpResponse*> chain;
	FilterDefault *default_filter = new FilterDefault();
	chain.add_filter(default_filter);

	chain.do_filter(request, response);


	// handle chain
	HandleChain<common::net::HttpRequest*, 
		    common::net::HttpResponse*> handle_chain;
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


	delete request;
	delete response;
}
	


void Connection::set_max_connectfd(int max_connfd)
{
//     set_max_connfd(max_connfd);
}

} // namespace sails


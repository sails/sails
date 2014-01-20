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
#include "util.h"
#include "thread_pool.h"
#include "request.h"
#include "response.h"
#include "filter.h"
#include "filter_default.h"
#include "handle.h"
#include "handle_default.h"
#include "handle_proto_decode.h"
#include "config.h"

namespace sails {


extern Config config;

void read_data(int connfd) {
     int len = 4 * 1024;
     char *buf = (char *)malloc(len);	  
     int n = 0;
     memset(buf, 0, len);
     
     n = read(connfd, buf, len);
     
     if(n == 0 || n == -1) { // n=0: client close or shutdown send
	                     // n=1: recv signal_pending before read data
	  perror("read connfd");
	  close(connfd); // will delete from epoll set auto
	  return;
     }
     
     printf("read buf :%s\n", buf);


     // parser http 
     size_t size = HttpHandle::instance()->parser_http(buf, connfd);
     free(buf); 
     buf = NULL;
     if(size <= 0) {
	  close(connfd);
	  return;
     }

     struct message* msg = get_message_by_connfd(connfd);
     if(!msg->body_is_final && !msg->message_complete_on_eof) {
	  return; // http be made from mutil tcp message
     }
     reset_message_by_connfd(connfd);

     
     ConnectionnHandleParam *param = (ConnectionnHandleParam *)malloc(sizeof(ConnectionnHandleParam));
     param->message = msg;
     param->connfd = connfd;

//     Connection::handle(param);

     // use thread pool to handle request
     static ThreadPool parser_pool(config.get_handle_thread_pool(),
	  config.get_handle_request_queue_size());	
     ThreadPoolTask task;
     task.fun = Connection::handle;
     task.argument = param;
     parser_pool.add_task(task);

}

void Connection::handle(void *message) 
{
	ConnectionnHandleParam *param = NULL;
	param = (ConnectionnHandleParam *)message;
	if(param == 0) {
		return;
	}
	
	Request *request = new Request(param->message);
	Response *response = new Response();
	printf("param->connfd:%Xbd\n", param->connfd);
	response->connfd = param->connfd;
	
	printf("filter start");
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


	delete request;
	delete response;

}
	


void Connection::set_max_connectfd(int max_connfd)
{
     set_max_connfd(max_connfd);
}

} // namespace sails


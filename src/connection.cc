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
#include <common/base/thread_pool.h>
#include <common/base/handle.h>
#include "handle_rpc.h"
#include "config.h"

namespace sails {

extern common::net::ConnectorTimeout connect_timer;
extern Config config;
extern common::EventLoop ev_loop;

//void read_data(int connfd) {
void read_data(common::event* ev, int revents) {
    if(ev == NULL || ev->fd < 0) {
	return;
    }
    int connfd = ev->fd;

    common::net::ComConnector *connector = (common::net::ComConnector *)ev->data;
    int n = connector->read();
    if(n == 0 || n == -1) { // n=0: client close or shutdown send
	// n=-1: recv signal_pending before read data
	perror("read connfd");
	ev->data = NULL;
	ev_loop.event_stop(connfd);
	delete(connector);
	connector = NULL;
	return;
    }

    connect_timer.update_connector_time(connector);// update timeout

    connector->parser();

    common::net::PacketCommon *packet = NULL;

    while((packet=connector->get_next_packet()) != NULL) {
	if (packet->type.opcode == common::net::PACKET_HEARTBEAT) {
	    return;
	}else if (packet->type.opcode == common::net::PACKET_PROTOBUF_CALL) {
	    ConnectionnHandleParam *param = (ConnectionnHandleParam *)malloc(sizeof(ConnectionnHandleParam));
	    param->connector = connector;
	    param->packet = packet;
	    param->conn_fd = connfd;
	
	    // use thread pool to handle request
	    static common::ThreadPool parser_pool(config.get_handle_thread_pool(), config.get_handle_request_queue_size());	
	    common::ThreadPoolTask task;
	    task.fun = Connection::handle_rpc;
	    task.argument = param;
	    parser_pool.add_task(task);
	}
    }
}

void Connection::handle_rpc(void *message) 
{
    ConnectionnHandleParam *param = NULL;
    param = (ConnectionnHandleParam *)message;
    if(param == 0) {
	return;
    }
	
    common::net::ComConnector *connector = param->connector;
    common::net::PacketCommon *request = param->packet;
    int connfd = param->conn_fd;

    int response_len = sizeof(common::net::PacketRPC)+2048;
    common::net::PacketCommon *response = (common::net::PacketCommon*)malloc(response_len);
    memset(response, 0, response_len);

	
    common::HandleChain<common::net::PacketCommon*, 
			common::net::PacketCommon*> handle_chain;
    HandleRPC proto_decode;
    handle_chain.add_handle(&proto_decode);
	
    handle_chain.do_handle(request, response);

    if(response != NULL) {
	// out put
	int n = write(connfd, response, response->len);
    }
	
    free(request);
    free(response);
    if(param != NULL) {
	free(param);
    }
}
	


void Connection::set_max_connectfd(int max_connfd)
{
//     set_max_connfd(max_connfd);
}

} // namespace sails


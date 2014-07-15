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

extern common::net::ConnectorTimeout<common::net::PacketCommon> connect_timer;
extern Config config;
extern common::EventLoop ev_loop;

long drop_packet_num = 0;

std::mutex connector_lock;


void delete_connector(common::net::Connector<common::net::PacketCommon> *connector)
{
    ev_loop.event_stop(connector->get_connector_fd());
}

common::net::PacketCommon* parser_cb(
    common::net::Connector<common::net::PacketCommon> *connector) {


    if (connector->readable() < sizeof(common::net::PacketCommon)) {
	return NULL;
    }
    common::net::PacketCommon *packet = (common::net::PacketCommon*)connector->peek();
    if (packet->type.opcode >= common::net::PACKET_MAX
	|| packet->type.opcode <= common::net::PACKET_MIN) { // error, and empty all data
	connector->retrieve(connector->readable());
	if (connector->get_invalid_msg_cb() != NULL) {
	    connector->get_invalid_msg_cb()(connector);
	}
	return NULL;
    }


    if (packet != NULL) {
	int packetlen = packet->len;
	if (packetlen < sizeof(common::net::PacketCommon)) {
	    return NULL;
	}
	if(connector->readable() >= packetlen) {
	    common::net::PacketCommon *item = (common::net::PacketCommon*)malloc(packetlen);
	    memset(item, 0, packetlen);
	    memcpy(item, packet, packetlen);
	    connector->retrieve(packetlen);

	    return item;
	}
    }
    return NULL;
}

//void read_data(int connfd) {
void read_data(common::event* ev, int revents) {
    if(ev == NULL || ev->fd < 0) {
	return;
    }
    int connfd = ev->fd;
    common::net::Connector<common::net::PacketCommon> *connector = (common::net::Connector<common::net::PacketCommon> *)ev->data;

    if (connector == NULL) {
	return;
    }

    connector_lock.lock();
    connector->extern_data++;
    connector_lock.unlock();

    int n = connector->read();

    if(n == 0 || n == -1) { // n=0: client close or shutdown send
	// n=-1: recv signal_pending before read data
	perror("read connfd");
        connector->close();
    }else {
	
	connect_timer.update_connector_time(connector);// update timeout

	connector->parser(); // maybe you want to close connect when parser invalid msg, so you can set connector is_closed to true

	if (! connector->isClosed()) {
	    common::net::PacketCommon *packet = NULL;

	    while((packet=connector->get_next_packet()) != NULL) {
		if (packet->type.opcode == common::net::PACKET_HEARTBEAT) {

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
		    if (parser_pool.add_task(task) == -1) {
			drop_packet_num++;
			printf("drop_packet_num:%ld\n", drop_packet_num);
		    }else {
			connector_lock.lock();
			connector->extern_data++;
			connector_lock.unlock();
		    }
		}
	    }
	}
    }

    connector_lock.lock();
    connector->extern_data--;
    if (connector->isClosed() && connector->extern_data == 0) {
	delete connector;
	connector = NULL;
    }
    connector_lock.unlock();
}

void Connection::handle_rpc(void *message) 
{
    ConnectionnHandleParam *param = NULL;
    param = (ConnectionnHandleParam *)message;
    if(param == 0) {

    }else{
	common::net::Connector<common::net::PacketCommon> *connector = param->connector;
	common::net::PacketCommon *request = param->packet;
	int connfd = param->conn_fd;

	param->connector = NULL;
	param->packet = NULL;

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


	connector_lock.lock();
	connector->extern_data--;
	if (connector->extern_data == 0 && connector->isClosed()) { // task 
	    delete connector;
	    connector = NULL;
	}
	connector_lock.unlock();
    
	free(request);
	free(response);
	if(param != NULL) {
	    free(param);
	    param = NULL;
	}

    }
	
}
	

} // namespace sails


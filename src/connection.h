#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <string>
#include <vector>
#include <thread>
#include <common/base/event_loop.h>
#include <common/net/packets.h>
#include <common/net/connector.h>
//#include <common/net/com_connector.h>

namespace sails {

//void read_data(int connfd);
void read_data(common::event*, int revents);

void event_stop_cb(common::event*);

typedef struct ConnectionnHandleParam {
    std::shared_ptr<common::net::Connector<common::net::PacketCommon>> connector;
    common::net::PacketCommon *packet;
    int conn_fd;
} ConnectionnHandleParam;

class Connection {
public:
     static void handle_rpc(void *message);
};

common::net::PacketCommon* parser_cb(
    common::net::Connector<common::net::PacketCommon> *connector);

void timeout_cb(
    common::net::Connector<common::net::PacketCommon> *connector);

void close_cb(
    common::net::Connector<common::net::PacketCommon> *connector);

void delete_connector_cb(
    common::net::Connector<common::net::PacketCommon> *connector);



} //namespace sails

#endif /* _CONNECTION_H_ */















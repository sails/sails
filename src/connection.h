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

typedef struct ConnectionnHandleParam {
    common::net::Connector<common::net::PacketCommon> *connector;
    common::net::PacketCommon *packet;
    int conn_fd;
} ConnectionnHandleParam;

class Connection {
public:
     static void handle_rpc(void *message);
};

class ConnectorDeleter {
public:
    ConnectorDeleter(common::net::Connector<common::net::PacketCommon> *connector);
    ~ConnectorDeleter();
private:
    common::net::Connector<common::net::PacketCommon> *connector;
};

common::net::PacketCommon* parser_cb(
    common::net::Connector<common::net::PacketCommon> *connector);

void delete_connector(
    common::net::Connector<common::net::PacketCommon> *connector);

} //namespace sails

#endif /* _CONNECTION_H_ */















#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <string>
#include <vector>
#include <thread>
#include <common/base/event_loop.h>
#include <common/net/packets.h>
#include <common/net/com_connector.h>

namespace sails {

//void read_data(int connfd);
void read_data(common::event*, int revents);

typedef struct ConnectionnHandleParam {
    common::net::ComConnector *connector;
    common::net::PacketCommon *packet;
    int conn_fd;
} ConnectionnHandleParam;

class Connection {
public:
     static void set_max_connectfd(int max_connfd);
     static void handle(void *message);
};

} //namespace sails

#endif /* _CONNECTION_H_ */















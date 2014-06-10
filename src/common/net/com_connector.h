#ifndef _COM_CONNECTOR_H_
#define _COM_CONNECTOR_H_

#include <list>
#include <common/net/packets.h>
#include <common/net/connector.h>

namespace sails {
namespace common {
namespace net {


// common packets connector
class ComConnector : public Connector {
public:
    ComConnector(int connect_fd);
    ComConnector();
    ~ComConnector();

    void parser();
    PacketCommon* get_next_packet();
private:

void push_recv_list(PacketCommon *packet);
    std::list<PacketCommon *> recv_list;
};


} // namespace net
} // namespace common
} // namespace sails

#endif /* _COM_CONNECTOR_H_ */

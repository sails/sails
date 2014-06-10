#include <common/net/com_connector.h>

namespace sails {
namespace common {
namespace net {


ComConnector::ComConnector(int connect_fd)
    :Connector(connect_fd)
{
    
}


ComConnector::ComConnector():Connector()
{
    
}

ComConnector::~ComConnector() {
    
}


void ComConnector::parser() {
    if (in_buf.readable() < sizeof(PacketCommon)) {
	return;
    }
    PacketCommon *packet = (PacketCommon*)in_buf.peek();
    if (packet->type.opcode >= PACKET_MAX) { // error, and empty all data
	in_buf.retrieve_all();
	return;
    }
    if (packet != NULL) {
	int packetlen = packet->len;
	if(in_buf.readable() >= packetlen) {
	    PacketCommon *item = (PacketCommon*)malloc(packetlen);
	    memcpy(item, packet, packetlen);
	    in_buf.retrieve(packetlen);
	    push_recv_list(item);
	    return;
	}
    }
}

void ComConnector::push_recv_list(PacketCommon *packet) {
    if(recv_list.size() <= 20) {
	recv_list.push_back(packet);
    }else {
	char msg[100];
	memset(msg, '\0', 100);
	sprintf(msg, "connect fd %d unhandle recv list more than 20 and can't parser", connect_fd);
	perror(msg);
    }
}

PacketCommon* ComConnector::get_next_packet() {
    if (!recv_list.empty()) {
	PacketCommon *packet = recv_list.front();
	recv_list.pop_front();
	return packet;
    }
    return NULL;
}


} // namespace net
} // namespace common
} // namespace sails

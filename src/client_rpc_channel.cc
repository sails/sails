#include "client_rpc_channel.h"
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <common/net/packets.h>

using namespace std;
using namespace google::protobuf;


namespace sails {


RpcChannelImp::RpcChannelImp(string ip, int port):ip(ip),port(port) {
    assert(connector.connect(ip.c_str(), 8000, true));
    connector.set_parser_fun(RpcChannelImp::parser_cb);
}

void RpcChannelImp::CallMethod(const MethodDescriptor* method, 
			       RpcController *controller, 
			       const Message *request, 
			       Message *response, 
			       Closure *done) 
{
    int ret = this->sync_call(method, controller, request, response);
    if(ret == 0 && done != NULL) {
	done->Run();		
    }
}

common::net::PacketCommon* RpcChannelImp::parser_cb(
    common::net::Connector<common::net::PacketCommon> *connector) {

    if (connector->readable() < sizeof(common::net::PacketCommon)) {
	return NULL;
    }
    common::net::PacketCommon *packet = (common::net::PacketCommon*)connector->peek();
    if (packet->type.opcode >= common::net::PACKET_MAX) { // error, and empty all data
	connector->retrieve(connector->readable());
	return NULL;
    }
    if (packet != NULL) {
	int packetlen = packet->len;
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
int RpcChannelImp::sync_call(const google::protobuf::MethodDescriptor *method, 
			     google::protobuf::RpcController *controller, 
			     const google::protobuf::Message *request, 
			     google::protobuf::Message *response)
{

    const string service_name = method->service()->name();
    string content = request->SerializeAsString();

    int len = sizeof(common::net::PacketRPC)+content.length()-1;
    common::net::PacketRPC *packet = (common::net::PacketRPC*)malloc(len);
    memset(packet, 0, len);
    packet->common.type.opcode = common::net::PACKET_PROTOBUF_CALL;
    packet->common.len = len;
    memcpy(packet->service_name, service_name.c_str(), service_name.length());
    memcpy(packet->method_name, method->name().c_str(), method->name().length());
    packet->method_index = method->index();
    memcpy(packet->data, content.c_str(), content.length());

    connector.write((char *)packet, len);
    connector.send();
    int n = connector.read();
    if(n > 0) {
	connector.parser();
    }

    common::net::PacketCommon *resp = NULL;
    while((resp=connector.get_next_packet()) != NULL) {
	char *body = ((common::net::PacketRPC*)resp)->data;
	if(strlen(body) > 0) {
	    // protobuf message
	    response->ParseFromString(string(body));
	}
	delete(resp);
    }
	
    return 0;
}


} // namespace sails




#include "handle_rpc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "service_register.h"

using namespace std;
using namespace google::protobuf;

namespace sails {

void HandleRPC::do_handle(common::net::PacketCommon *request, 
			      common::net::PacketCommon *response, 
			      common::HandleChain<common::net::PacketCommon *, common::net::PacketCommon *> *chain)
{
    if(request != NULL) {
	if (request->type.opcode == common::net::PACKET_PROTOBUF_CALL ||
	    request->type.opcode == common::net::PACKET_PROTOBUF_RET) {
	    decode_protobuf((common::net::PacketRPC*)request, 
			    (common::net::PacketRPC*)response, chain);
	}
	chain->do_handle(request, response);
	return;
    }
}

void HandleRPC::decode_protobuf(common::net::PacketRPC *request, common::net::PacketRPC *response, common::HandleChain<common::net::PacketCommon *, common::net::PacketCommon *> *chain)
{
    string service_name(request->service_name);
    string method_name(request->method_name);
    int method_index = request->method_index;

//    cout << service_name << " " << method_name << " " << method_index << endl;
    if(!service_name.empty() && !method_name.empty()) {
	google::protobuf::Service* service = ServiceRegister::instance()->get_service(service_name);
	if(service != NULL) {
	    // or find by method_index
	    const MethodDescriptor *method_desc = service->GetDescriptor()->method(method_index);
	    Message *request_msg = service->GetRequestPrototype(method_desc).New();
	    Message *response_mg = service->GetResponsePrototype(method_desc).New();

	    string msgstr(request->data);
	    request_msg->ParseFromString(msgstr);
	    service->CallMethod(method_desc,NULL, request_msg, response_mg, NULL);
	    string response_content = response_mg->SerializeAsString();

	    const char* data = response_content.c_str();
	    int len = sizeof(common::net::PacketRPC)+response_content.length()-1;
	    response->common.type.opcode = common::net::PACKET_PROTOBUF_RET;
	    response->common.len = len;
	    memcpy(response->data, data, response_content.length());

	    delete request_msg;
	    delete response_mg;
	}
    }
}

} // namespace sails 

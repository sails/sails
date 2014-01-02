#include "handle_proto_decode.h"
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



void HandleProtoDecode::do_handle(sails::Request *request, 
				sails::Response *response, 
				HandleChain<sails::Request *, sails::Response *> *chain)
{
	if(request != 0) {
		int proto_type = get_request_protocol(request);
		if(proto_type == PROTOBUF_PROTOCOL) {
			printf("protobuf protocol decode handle\n");
			decode_protobuf(request, response, chain);
		}
	}
	chain->do_handle(request, response);
	return;
}

void HandleProtoDecode::decode_protobuf(sails::Request *request, sails::Response *response, HandleChain<sails::Request *, sails::Response *> *chain)
{
	string service_name = request->getparam("serviceName");
	string method_name = request->getparam("methodName");
	int method_index = stoi(request->getparam("methodIndex"));

	if(!service_name.empty() && !method_name.empty()) {
		google::protobuf::Service* service = ServiceRegister::instance()->get_service(service_name);
		if(service != NULL) {
//			const MethodDescriptor *method_desc = service->GetDescriptor()->FindMethodByName(method_name);
			// or find by method_index
			const MethodDescriptor *method_desc = service->GetDescriptor()->method(method_index);
			Message *request_msg = service->GetRequestPrototype(method_desc).New();
			Message *response_mg = service->GetResponsePrototype(method_desc).New();
			printf("body:%s\n", request->raw_data->body+14);
			string msgstr(request->raw_data->body+14);
			request_msg->ParseFromString(msgstr);
			service->CallMethod(method_desc,NULL, request_msg, response_mg, NULL);
			string response_content = response_mg->SerializeAsString();
		        response->set_header("serviceName", method_desc->service()->name().c_str());
			response->set_header("methodName", method_desc->name().c_str());

			response_content = "sails:protobuf"+response_content;
			response->set_body(response_content.c_str());
		}
	}
}


} // namespace sails


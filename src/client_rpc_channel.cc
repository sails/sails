#include "client_rpc_channel.h"
#include <stdio.h>
#include <sstream>
#include <assert.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <common/net/http_connector.h>

using namespace std;
using namespace google::protobuf;


namespace sails {

RpcChannelImp::RpcChannelImp(string ip, int port):ip(ip),port(port) {
    assert(connector.connect(ip.c_str(), 8000, true));
    
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

int RpcChannelImp::sync_call(const google::protobuf::MethodDescriptor *method, 
			     google::protobuf::RpcController *controller, 
			     const google::protobuf::Message *request, 
			     google::protobuf::Message *response)
{

    const string service_name = method->service()->name();
    string content = request->SerializeAsString();


    // construct http request
    common::net::http_message *raw_data = (common::net::http_message*)malloc(sizeof(common::net::http_message));
    common::net::http_message_init(raw_data);
    common::net::HttpRequest http_request(raw_data);
    http_request.set_default_header();
    stringstream port_str;
    port_str << port;
    http_request.set_header("Host", (ip+":"+port_str.str()).c_str());
    http_request.set_request_method(2);
    http_request.set_header("serviceName", method->service()->name().c_str());
    http_request.set_header("methodName", method->name().c_str());
    stringstream indexstr;
    indexstr << method->index();
    http_request.set_header("methodIndex", indexstr.str().c_str());

    content = string(common::net::PROTOBUF)+content;
    http_request.set_body(content.c_str());
		
    stringstream body_len_str;
    body_len_str<<content.length();
    http_request.set_header("Content-Length", body_len_str.str().c_str());
    http_request.to_str();
    char *data = http_request.get_raw();

//    printf("%s\n", data);

    connector.write(data, strlen(data));
    connector.send();

    int n = connector.read();
    if(n > 0) {
	connector.httpparser();
    }

    common::net::HttpResponse *resp = NULL;
    while((resp=connector.get_next_httpresponse()) != NULL) {
	char *body = resp->get_body();
//	printf("response body:%s\n", body);
		
	if(strlen(body) > 0) {
	    if(strncasecmp(body, 
			   common::net::PROTOBUF, strlen(common::net::PROTOBUF)) == 0) {
		// protobuf message
		response->ParseFromString(string(body+14));
				
	    }
	}
	delete(resp);
    }
	
    return 0;
}


} // namespace sails




#include "client_rpc.h"
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <common/base/string.h>
#include <common/net/http.h>
#include <common/net/http_connector.h>

using namespace std;
using namespace google::protobuf;


namespace sails {

RpcClient::RpcClient(std::string ip, int port):ip(ip),port(port){
	
}

int RpcClient::sync_call(const google::protobuf::MethodDescriptor *method, 
			 google::protobuf::RpcController *controller, 
			 const google::protobuf::Message *request, 
			 google::protobuf::Message *response)
{
    RpcClientConnection connection(ip, port);
    int connectfd = connection.get_available_con_fd();
    if(connectfd > 0) {

	common::net::HttpConnector connector(connectfd);
	const string service_name = method->service()->name();
	string content = request->SerializeAsString();


	// construct http request
	common::net::http_message *raw_data = (common::net::http_message*)malloc(sizeof(common::net::http_message));
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

	cout << data << endl;

	write(connectfd, data, strlen(data));


        int n = connector.read();
	if(n > 0) {
	    connector.httpparser();
	}

	common::net::HttpResponse *resp = NULL;
	while((resp=connector.get_next_httpresponse()) != NULL) {
	    printf("parser ok...............\n");
	    char *body = resp->get_body();
	    printf("response body:%s\n", body);
		
	    if(strlen(body) > 0) {
		if(strncasecmp(body, 
			       common::net::PROTOBUF, strlen(common::net::PROTOBUF)) == 0) {
		    // protobuf message
		    response->ParseFromString(string(body+14));
				
		}
	    }
	}
	
		
	printf("\n");
    }
	
    return 0;
}



// connection
RpcClientConnection::RpcClientConnection(string ip, int port)
{
    struct sockaddr_in serveraddr;
    connectfd = socket(AF_INET, SOCK_STREAM, 0);
    if(connectfd == -1) {
	printf("new clientfd error\n");
	abort();
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ip.c_str());
    serveraddr.sin_port = htons(port);

    int ret = connect(connectfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(ret == -1) {
	printf("connect failed\n");
	abort();
    }
}

int RpcClientConnection::get_available_con_fd()
{
    return this->connectfd;
}


} // namespace sails








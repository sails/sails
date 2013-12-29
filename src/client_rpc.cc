#include "client_rpc.h"
#include <iostream>
#include <sstream>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "request.h"

using namespace std;
using namespace google::protobuf;


namespace sails {


int RpcClient::sync_call(const google::protobuf::MethodDescriptor *method, 
		     google::protobuf::RpcController *controller, 
		     const google::protobuf::Message *request, 
		     google::protobuf::Message *response)
{
	RpcClientConnection connection("127.0.0.1", 8000);
	int connectfd = connection.get_available_con_fd();
	if(connectfd > 0) {
		const string service_name = method->service()->name();
		string content = request->SerializeAsString();


		// construct http request
		struct message *raw_data = (struct message*)malloc(sizeof(struct message));
		Request http_request(raw_data);
		http_request.set_default_header();
		http_request.set_header("Host", "localhost:8000");
		http_request.set_request_method(2);

		content = "sails:protobuf"+content;
		http_request.set_body(content.c_str());
		
	        stringstream body_len_str;
		body_len_str<<content.length();
		http_request.set_header("Content-Length", body_len_str.str().c_str());
		http_request.to_str();
		char *data = http_request.get_raw();

		cout << data << endl;

		write(connectfd, data, strlen(data));
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








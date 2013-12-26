#include "client_rpc.h"
#include <iostream>
#include <google/protobuf/descriptor.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
using namespace google::protobuf;


namespace sails {


int RpcClient::sync_call(const google::protobuf::MethodDescriptor *method, 
		     google::protobuf::RpcController *controller, 
		     const google::protobuf::Message *request, 
		     google::protobuf::Message *response)
{
	RpcClientConnection connection("127.0.0.1", 8000);
	int connectfd = connection.connectfd;
	
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








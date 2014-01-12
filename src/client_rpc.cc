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
#include "request.h"
#include "util.h"
#include "http.h"

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
		const string service_name = method->service()->name();
		string content = request->SerializeAsString();


		// construct http request
		struct message *raw_data = (struct message*)malloc(sizeof(struct message));
		Request http_request(raw_data);
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

		content = string(PROTOBUF)+content;
		http_request.set_body(content.c_str());
		
	        stringstream body_len_str;
		body_len_str<<content.length();
		http_request.set_header("Content-Length", body_len_str.str().c_str());
		http_request.to_str();
		char *data = http_request.get_raw();

		cout << data << endl;

		write(connectfd, data, strlen(data));
		/*
		  // test segments tcp send a request
		int data_len = strlen(data);
		int data_send = 0;
		for(int i = 0; i <= data_len/10; i++) {
		     int to_send = 10;
		     if(i == data_len/10) {
			  if(i == 0) {
			       to_send = data_len;
			  }else {
			       to_send = data_len -(10*(i-1));
			  }
			  
		     }
		     char tmp[10];
		     memset(tmp, '\0', 10);
		     strncpy(tmp, data+data_send, to_send);
		     printf("send:%s\n", tmp);
		     write(connectfd, data+data_send, to_send);
		     data_send = data_send+to_send;
		}
		*/

		int n = 0;
		int len = 100 * 1024;
		char *recv_buf = (char *)malloc(len);
		memset(recv_buf, '\0', len);
		n = read(connectfd, recv_buf, len);
		printf("recv msg:%s\n", recv_buf);
		HttpHandle::instance()->parser_http(recv_buf,connectfd);
		printf("parser ok...............\n");
		struct message* msg = get_message_by_connfd(connectfd);
		printf("response body:%s\n", msg->body);
		
		if(strlen(msg->body) > 0) {
			if(strncasecmp(msg->body, 
				       PROTOBUF, strlen(PROTOBUF)) == 0) {
				// protobuf message
				response->ParseFromString(string(msg->body+14));
				
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








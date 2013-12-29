#ifndef _CLIENT_RPC_H_
#define _CLIENT_RPC_H_

#include <iostream>
#include <google/protobuf/service.h>


namespace sails {

class RpcClientConnection
{
public:
	RpcClientConnection(std::string ip, int port);
	int get_available_con_fd();
private:
	int connectfd;
	
};

class RpcClient {
public:
	RpcClient(std::string ip, int port);
	int sync_call(const google::protobuf::MethodDescriptor *method,
		      google::protobuf::RpcController* controller,
		      const google::protobuf::Message* request,
		      google::protobuf::Message* response);

private:
	std::string ip;
	int port;
};

} // namespace sails

#endif /* _CLIENT_RPC_H_ */
















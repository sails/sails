#include "client_rpc_channel.h"
#include <iostream>
#include <google/protobuf/descriptor.h>
#include "client_rpc.h"

using namespace std;
using namespace google::protobuf;


namespace sails {

RpcChannelImp::RpcChannelImp(string ip, int port):ip(ip),port(port) {

}

void RpcChannelImp::CallMethod(const MethodDescriptor* method, 
			       RpcController *controller, 
			       const Message *request, 
			       Message *response, 
			       Closure *done) 
{
//	cout << "method:" << method->name() << endl;
	RpcClient *client = new RpcClient();
	int ret = client->sync_call(method, controller, request, response);
	if(ret == 0 && done != NULL) {
		done->Run();		
	}
}


} // namespace sails




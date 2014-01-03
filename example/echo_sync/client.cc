#include <stdio.h>
#include <iostream>
#include "client_rpc_channel.h"
#include "client_rpc_controller.h"
#include "client_rpc.h"
#include "addressbook.pb.h"

using namespace sails;
using namespace google::protobuf;
using namespace test;


void DoneCallback(AddressBook *response) {
	printf("done call back\n");
}

int main(int argc, char *argv[])
{
	int port = 8000;
	if(argc == 2) {
		port = atoi(argv[1]);
	}
        RpcChannelImp channel("127.0.0.1", port);
	RpcControllerImp controller;

	AddressBookService::Stub stub(&channel);

        AddressBook request;
        AddressBook response;


	Person *p1 = request.add_person();
	p1->set_id(1);
	p1->set_name("xu");
	p1->set_email("sailsxu@gmail.com");

	Closure* callback = NewCallback(&DoneCallback, &response);

	stub.add(&controller, &request, &response, callback);
	
	printf("response:\n");
	std::cout << response.DebugString() << std::endl;

	return 0;

}











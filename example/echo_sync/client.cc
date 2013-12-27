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
        RpcChannelImp channel("l127.0.0.1", 9000);
	RpcControllerImp controller;

	AddressBookService::Stub stub(&channel);

        AddressBook request;
        AddressBook response;

	Closure* callback = NewCallback(&DoneCallback, &response);

	stub.add(&controller, &request, &response, callback);

	return 0;

}

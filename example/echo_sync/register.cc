#include <stdio.h>
#include "addressbook.pb.h"

namespace test {

class AddressBookServiceImp : public AddressBookService
{
	virtual void add(::google::protobuf::RpcController* controller,
                       const ::test::AddressBook* request,
                       ::test::AddressBook* response,
			 ::google::protobuf::Closure* done) {
		printf("add method call\n");
	}
};

}


extern "C" {
	google::protobuf::Service* register_module() {
		test::AddressBookServiceImp *service = new test::AddressBookServiceImp();
		printf("start register\n");
		return service;

	}
}


















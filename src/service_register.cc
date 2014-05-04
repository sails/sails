#include "service_register.h"
#include <stdio.h>
#include <google/protobuf/descriptor.h>


using namespace std;
using namespace google::protobuf;

namespace sails {

ServiceRegister *ServiceRegister::_instance = NULL;

bool ServiceRegister::register_service(google::protobuf::Service *service) {
	
    service_map.insert(
	map<string, Service*>::value_type(service->GetDescriptor()->name(), 
					  service));

    return true;
}

google::protobuf::Service* ServiceRegister::get_service(string key) {
    map<string, Service*>::iterator iter;
    iter = service_map.find(key);
    if(iter == service_map.end()) {
	return NULL;
    }else {
	return iter->second;
    }
}


} // namespace sails
















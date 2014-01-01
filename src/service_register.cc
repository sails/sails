#include "service_register.h"
#include <stdio.h>
#include <google/protobuf/descriptor.h>


using namespace std;
using namespace google::protobuf;

namespace sails {

ServiceRegister *ServiceRegister::_instance = NULL;

bool ServiceRegister::register_service(google::protobuf::Service *service) {
	
	const ServiceDescriptor *service_descriptor = 
		service->GetDescriptor();
	service_map.insert(map<string, Service*>::value_type(service->GetDescriptor()->name(), service));
/*
	for(int i = 0; i < service_descriptor->method_count(); i++) {
		const MethodDescriptor *method_descriptor = 
			service_descriptor->method(i);
		string key = service_descriptor->name()+method_descriptor->name();
		cout << "register key:" << key << endl;
		map<string,Service*>::iterator it = service_map.find(key);
		if(it != service_map.end()) {
			printf("error duplicate server:%s and method:%s!\n"
			       ,service_descriptor->name().c_str()
			       ,method_descriptor->name().c_str());
			abort();
		}else {
			service_map.insert(map<string, Service*>::value_type(key, service));
		}
		
	}
*/
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
















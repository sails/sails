#include "request.h"
#include <stdlib.h>
#include <string.h>

namespace sails {

Request::Request(struct message *raw_data) {
	this->raw_data = raw_data;
	
	// set request param
	
}

int Request::get_protocol_type() {
	if(this->raw_data != NULL) {
		int type = 1; // default basic http
		return type;
	}
	return 0;
}

std::string Request::getparam(std::string) {
	
}





// protocol type
int get_request_protocol(Request* request) 
{
	if(request != NULL) {
		char *body = request->raw_data->body;
		if(strlen(body) > 0) {
			if(strncasecmp(body, "sails:protobuf", 14) == 0) {
				return PROTOBUF_PROTOCOL;	
			}

		}
		return NORMAL_PROTOCOL;
	}else {
		return ERROR_PROTOCOL;
	}

}


} // namespace sails

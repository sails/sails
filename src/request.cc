#include "request.h"

namespace sails {

Request::Request(struct message *raw_data) {
	this->raw_data = raw_data;
}

int Request::get_protocol_type() {
	if(this->raw_data != NULL) {
		int type = 1; // default basic http
		return type;
	}
	return 0;
}

} // namespace sails

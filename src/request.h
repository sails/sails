#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "http.h"

namespace sails {

class Request {
public:
	Request(struct message *raw_data);
	int get_protocol_type();
public:
	struct message *raw_data;
};
	
} // namespace sails

#endif /* _REQUEST_H_ */

#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <map>
#include <string>
#include "http.h"

namespace sails {

class Request {
public:
	Request(struct message *raw_data);
	int get_protocol_type();

	std::string getparam(std::string);
	
public:
	struct message *raw_data;
private:
	std::map<std::string, std::string> param;
};


// get protocol type 
enum PROTOCOL { ERROR_PROTOCOL=-1, NORMAL_PROTOCOL=0, PROTOBUF_PROTOCOL };
int get_request_protocol(Request *request);

	
} // namespace sails

#endif /* _REQUEST_H_ */

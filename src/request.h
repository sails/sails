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
	
} // namespace sails

#endif /* _REQUEST_H_ */

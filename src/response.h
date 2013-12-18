#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "http.h"

namespace sails {

class Response {
public:
	Response();
	int set_header(const char* key, const char *value);
	int set_body(const char* body);
	int to_str();
public:
	struct message *raw_data;
	int connfd;
};

} // namespace sails

#endif /* _RESPONSE_H_ */















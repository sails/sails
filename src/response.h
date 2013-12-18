#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "http.h"

namespace sails {

class Response {
public:
	Response();
	char* to_str();
public:
	struct message *raw_data;
	int connfd;
};

} // namespace sails

#endif /* _RESPONSE_H_ */















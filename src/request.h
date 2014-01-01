#ifndef _REQUEST_H_
#define _REQUEST_H_

#include <map>
#include <string>
#include "http.h"

namespace sails {

class Request {
public:
	Request(struct message *raw_data);

	std::string getparam(std::string param_name);

	// new request for rpc client to send data 
	void set_http_proto(int http_major, int http_minor);
	void set_request_method(int method);
	void set_default_header();
	int set_header(const char* key, const char *value);
	int set_body(const char* body);
	int to_str();
	char* get_raw();
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

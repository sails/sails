#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "http.h"

namespace sails {

class Response {
public:
     Response();
     ~Response();

     void set_http_proto(int http_major, int http_minor);
     void set_response_status(int response_status);
     int set_header(const char* key, const char *value);
     int set_body(const char* body);
     int to_str();
     char* get_raw();
public:
     int connfd;
private:
     struct message *raw_data;
     void set_default_header();
};

} // namespace sails

#endif /* _RESPONSE_H_ */















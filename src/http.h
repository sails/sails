#ifndef _HTTP_H_
#define _HTTP_H_

#include <http_parser.h>
#include <string>

namespace sails {

#define MAX_HEADERS 13
#define MAX_ELEMENT_SIZE 2048


enum header_element { NONE=0, FIELD, VALUE };


#undef TRUE
#define TRUE 1
#undef FALSE
#define FALSE 0

struct message {
     char *raw;
     enum http_parser_type type;
     int method;
     int status_code;
     char request_path[MAX_ELEMENT_SIZE];
     char request_url[MAX_ELEMENT_SIZE];
     char fragment[MAX_ELEMENT_SIZE];
     char query_string[MAX_ELEMENT_SIZE];
     char body[MAX_ELEMENT_SIZE];
     size_t body_size;
     char *host;
     char *userinfo;
     uint16_t port;
     int num_headers;
     enum header_element last_header_element;
     char headers [MAX_HEADERS][2][MAX_ELEMENT_SIZE];
     int should_keep_alive;
	
     char *upgrade; // upgraded body
	
     unsigned short http_major;
     unsigned short http_minor;
	
     int message_begin_cb_called;
     int headers_complete_cb_called;
     int message_complete_cb_called;
     int message_complete_on_eof;
     int body_is_final;
};

int set_max_connfd(int max_connfd);
void reset_message_by_connfd(int connfd);
struct message* get_message_by_connfd(int connfd);

void message_init(message *msg);
void delete_message(message *msg);



class HttpHandle{		
public:

     static HttpHandle* instance() {
	  if(_instance == NULL) {
	       _instance = new HttpHandle();
	  }
	  return _instance;
     }

     ~HttpHandle();
	
     size_t parser_http(char *msg_buf, int connfd); 
     // http_parser call_back

     static int request_url_cb(http_parser *p, const char *buf, size_t len);
     static int status_complete_cb(http_parser *p);
     static int header_field_cb(http_parser *p, const char *buf, size_t len);
     static int header_value_cb(http_parser *p, const char *buf, size_t len);
     static void check_body_is_final(const http_parser *p);
     static int body_cb (http_parser *p, const char *buf, size_t len);
     int count_body_cb (http_parser *p, const char *buf, size_t len);
     static int message_begin_cb(http_parser *p);
     static int headers_complete_cb(http_parser *p);
     static int message_complete_cb(http_parser *p);

     
     // for request only
     void handle_request(http_parser* parser);

     static void printfmsg(struct message *m);

private:

     HttpHandle();//HTTP_REQUEST, HTTP_RESPONSE
     static HttpHandle* _instance;

     void parser_url(char *url, struct message *m);
     http_parser_settings settings;
};
	

}

#endif /* _HTTP_H_ */


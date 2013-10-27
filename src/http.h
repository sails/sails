#ifndef _HTTP_H_
#define _HTTP_H_

#include <http_parser.h>

namespace sails {

	const int MAX_ELEMENT_SIZE = 400;
	const int MAX_HEADERS = 100;
	
	struct message {
	  const char *name; // for debugging purposes
	  const char *raw;
	  enum http_parser_type type;
	  enum http_method method;
	  int status_code;
	  char request_path[MAX_ELEMENT_SIZE];
	  char request_url[MAX_ELEMENT_SIZE];
	  char fragment[MAX_ELEMENT_SIZE];
	  char query_string[MAX_ELEMENT_SIZE];
	  char body[MAX_ELEMENT_SIZE];
	  size_t body_size;
	  const char *host;
	  const char *userinfo;
	  uint16_t port;
	  int num_headers;
	  enum { NONE=0, FIELD, VALUE } last_header_element;
	  char headers [MAX_HEADERS][2][MAX_ELEMENT_SIZE];
	  int should_keep_alive;

	  const char *upgrade; // upgraded body
	  
	  unsigned short http_major;
	  unsigned short http_minor;
	  
	  int message_begin_cb_called;
	  int headers_complete_cb_called;
	  int message_complete_cb_called;
	  int message_complete_on_eof;
	  int body_is_final;
	  };
	
	
	struct message *parser_http_message(char *buf);
	
	http_parser *parser_http(char *buf);
}

#endif /* _HTTP_H_ */

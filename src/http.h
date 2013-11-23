#ifndef _HTTP_H_
#define _HTTP_H_

#include <http_parser.h>

namespace sails {

#define MAX_HEADERS 13
#define MAX_ELEMENT_SIZE 2048


enum header_element { NONE=0, FIELD, VALUE };


#undef TRUE
#define TRUE 1
#undef FALSE
#define FALSE 0

struct message {
	const char *name; // for debugging purposes
	const char *raw;
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
	
	const char *upgrade; // upgraded body
	
	unsigned short http_major;
	unsigned short http_minor;
	
	int message_begin_cb_called;
	int headers_complete_cb_called;
	int message_complete_cb_called;
	int message_complete_on_eof;
	int body_is_final;
};


class HttpHandle{		
public:

	HttpHandle();
	~HttpHandle();
	
	size_t parser_http(char *buf);

	// http_parser call_back

	int static request_url_cb(http_parser *p, const char *buf, size_t len);
	int static status_complete_cb(http_parser *p);
	int static header_field_cb(http_parser *p, const char *buf, size_t len);
	int static header_value_cb(http_parser *p, const char *buf, size_t len);
	void static check_body_is_final(const http_parser *p);
	int static body_cb (http_parser *p, const char *buf, size_t len);
	int count_body_cb (http_parser *p, const char *buf, size_t len);
	int static message_begin_cb(http_parser *p);
	int static headers_complete_cb(http_parser *p);
	int static message_complete_cb(http_parser *p);
	
// end http_parser call_back

	void handle_request(http_parser* parser);

	void printfmsg();

private:
	void parser_url(char *url);
	http_parser *parser;
	http_parser_settings settings;
public:

	struct message msg;
};
	

}

#endif /* _HTTP_H_ */









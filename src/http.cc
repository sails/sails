#include "http.h"
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include "util.h"



namespace sails {


void message_init(message *msg)
{
	if(msg != NULL) {
		msg->name = NULL;
		msg->raw = NULL;
		msg->method = 0;
		msg->status_code = 0;
		memset(msg->request_path, 0, MAX_ELEMENT_SIZE);
		memset(msg->request_url, 0, MAX_ELEMENT_SIZE);
		memset(msg->fragment, 0, MAX_ELEMENT_SIZE);
		memset(msg->query_string, 0, MAX_ELEMENT_SIZE);
		memset(msg->body, 0, MAX_ELEMENT_SIZE);
		msg->body_size = 0;
		msg->host = NULL;
		msg->userinfo = NULL;
		msg->port = 0;
		msg->num_headers = 0;
		for(int i = 0; i < MAX_HEADERS; i++) {
			memset(msg->headers[i][0], 0, MAX_ELEMENT_SIZE);
			memset(msg->headers[i][1], 0, MAX_ELEMENT_SIZE);
		}
		msg->last_header_element = NONE;
		msg->should_keep_alive = 1;
		msg->upgrade = NULL;
		msg->http_major = 1;
		msg->http_minor = 1;
	}
}

HttpHandle::HttpHandle() {

	settings.on_message_begin = message_begin_cb;
	settings.on_header_field = header_field_cb;
	settings.on_header_value = header_value_cb;
	settings.on_url = request_url_cb;
	settings.on_body = body_cb;
	settings.on_headers_complete = headers_complete_cb;
	settings.on_message_complete = message_complete_cb;
	
	message_init(&msg);

        parser = (http_parser *)malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);
	parser->data = this;

}

HttpHandle::~HttpHandle() {
	if(this->parser != NULL) {
		if(parser->data != NULL) {
			parser->data = NULL;
		}
		free(parser);
		parser = NULL;
	}
	if(this->msg.host != NULL) {
		free(this->msg.host);
		this->msg.host = NULL;
	}
	if(this->msg.userinfo != NULL) {
		free(this->msg.userinfo);
		this->msg.userinfo = NULL;
	}
}
	
size_t HttpHandle::parser_http(char *msg_buf) {

	size_t nparsed = http_parser_execute(parser, &settings, 
					     msg_buf, strlen(msg_buf));	
	
	return nparsed;
}

int
HttpHandle::request_url_cb (http_parser *p, const char *buf, size_t len)
{
	if(p->data == NULL) {
		printf("p->data == null\n");
	}
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	if(p != handle->parser) {
		printf("handle parser != p\n");
	}
	assert(p == handle->parser);
	strlncat(handle->msg.request_url,
		 sizeof(handle->msg.request_url),
		 buf,
		 len);

	return 0;
}

int HttpHandle::status_complete_cb (http_parser *p) {
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	return 0;
}

int HttpHandle::header_field_cb (http_parser *p, const char *buf, size_t len)
{
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);

	assert(p == handle->parser);
	struct message *m = &(handle->msg);

	if (m->last_header_element != FIELD)
		m->num_headers++;
	
	strlncat(m->headers[m->num_headers-1][0],
		 sizeof(m->headers[m->num_headers-1][0]),
		 buf,
		 len);

	m->last_header_element = FIELD;

	return 0;
}

int HttpHandle::header_value_cb (http_parser *p, const char *buf, size_t len)
{

	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	struct message *m = &(handle->msg);
	strlncat(m->headers[m->num_headers-1][1],
		 sizeof(m->headers[m->num_headers-1][1]),
		 buf,
		 len);
	m->last_header_element = VALUE;

	return 0;
}

void HttpHandle::check_body_is_final (const http_parser *p)
{
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	if (handle->msg.body_is_final) {
		fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
			"on last on_body callback call "
			"but it doesn't! ***\n\n");
		assert(0);
		abort();
	}
	handle->msg.body_is_final = http_body_is_final(p);
}

int HttpHandle::body_cb (http_parser *p, const char *buf, size_t len)
{
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	strlncat(handle->msg.body,
		 sizeof(handle->msg.body),
		 buf,
		 len);
	handle->msg.body_size += len;
	check_body_is_final(p);

	return 0;
}

int HttpHandle::count_body_cb (http_parser *p, const char *buf, size_t len)
{
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	assert(buf);
	handle->msg.body_size += len;
	check_body_is_final(p);

	return 0;
}

int HttpHandle::message_begin_cb (http_parser *p)
{
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	handle->msg.message_begin_cb_called = TRUE;
	return 0;
}

int HttpHandle::headers_complete_cb (http_parser *p)
{
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);

	assert(p == handle->parser);
	handle->msg.method = handle->parser->method;
	handle->msg.status_code = handle->parser->status_code;
	handle->msg.http_major = handle->parser->http_major;
	handle->msg.http_minor = handle->parser->http_minor;
	handle->msg.headers_complete_cb_called = TRUE;
	handle->msg.should_keep_alive = http_should_keep_alive(handle->parser);

	return 0;
}

int HttpHandle::message_complete_cb (http_parser *p)
{
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	if (handle->msg.should_keep_alive != http_should_keep_alive(handle->parser))
	{
		fprintf(stderr, "\n\n *** Error http_should_keep_alive() should have same "
			"value in both on_message_complete and on_headers_complete "
			"but it doesn't! ***\n\n");
		assert(0);
		abort();
	}

	if (handle->msg.body_size &&
	    http_body_is_final(p) &&
	    !handle->msg.body_is_final)
	{
		fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
			"on last on_body callback call "
			"but it doesn't! ***\n\n");
		assert(0);
		abort();
	}
	
	handle->msg.message_complete_cb_called = TRUE;

	handle->handle_request(p);
	return 0;
}


void HttpHandle::handle_request(http_parser* parser)
{
	char url[200];
	memset(url, 0, 200);
	strncat(url, "http://", strlen("http://"));
	for(int i = 0; i < this->msg.num_headers; i++) {
		if(strcmp("Host", this->msg.headers[i][0]) == 0) {
			strncat(url, this->msg.headers[i][1], strlen(msg.headers[i][1]));
			break;
		}
	}
	strncat(url, msg.request_url, strlen(msg.request_url));
	
	parser_url(url);
		
	this->printfmsg();
}	

void HttpHandle::parser_url(char *url)
{
	struct http_parser_url u;
	int url_result = 0;
	if((url_result = http_parser_parse_url(url, strlen(url), 0, &u)) == 0)
	{
		if(u.field_set & (1 << UF_PORT)) {
		        this->msg.port = u.port;
		}else {
			this->msg.port = 80;
		}
		if(this->msg.host) {
			free(this->msg.host);
		}
		if(u.field_set & (1 << UF_HOST)) {
			this->msg.host = (char*)malloc(u.field_data[UF_HOST].len+1);
			strncpy(this->msg.host, url+u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);  
			this->msg.host[u.field_data[UF_HOST].len] = 0;  
		}
		memset(this->msg.request_path, 0, strlen(this->msg.request_path));
		if(u.field_set & (1 << UF_PATH))  
		{  
			strncpy(this->msg.request_path, url+u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);  
			this->msg.request_path[u.field_data[UF_PATH].len] = 0;  
		}

	}else {
		printf("url parser error:%d\n", url_result);
	}
	
}


void HttpHandle::printfmsg()
{
	printf("message:\n");
	printf("request_url:%s\n", msg.request_url);
	printf("port:%d\n", msg.port);
	printf("request_path:%s\n", msg.request_path);
	printf("host:%s\n", msg.host);
	printf("should_keep_alive:%d\n", msg.should_keep_alive);

	for(int i = 0;i < msg.num_headers; i++) {
		printf("header %d:%s:%s\n", i, msg.headers[i][0], 
		       msg.headers[i][1]);
	}

	printf("body:%s\n", msg.body);

	
}

 
} // namespace sails

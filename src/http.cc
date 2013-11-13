#include "http.h"
#include <stdio.h>
#include <inttypes.h>
#include <iostream>
#include <string.h>
#include <string>
#include <strings.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include "util.h"

using namespace std;


namespace sails {
	
http_parser* HttpHandle::parser_http(char *buf) {
	static http_parser_settings settings;
	settings.on_message_begin = message_begin_cb;
	settings.on_header_field = header_field_cb;
	settings.on_header_value = header_value_cb;
	settings.on_url = request_url_cb;
	settings.on_body = body_cb;
	settings.on_headers_complete = headers_complete_cb;
	settings.on_message_complete = message_complete_cb;

        parser = (http_parser *)malloc(sizeof(http_parser));
	http_parser_init(parser, HTTP_REQUEST);
	parser->data = this;

	size_t nparsed = http_parser_execute(parser, &settings, buf, strlen(buf));	

	return parser;
}

int
HttpHandle::request_url_cb (http_parser *p, const char *buf, size_t len)
{
	cout << "request_url_cb" << endl;
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	strlncat(handle->msg.request_url,
		 sizeof(handle->msg.request_url),
		 buf,
		 len);


	struct http_parser_url u;
	int url_result = 0;
	if((url_result = http_parser_parse_url(buf, strlen(buf), 0, &u)) == 0)
	{
		cout << "u port:" << u.port << endl;
		if(u.field_set & (1 << UF_PORT)) {
		        handle->msg.port = u.port;
		}else {
			handle->msg.port = 80;
		}
		if(handle->msg.host) {
			free(handle->msg.host);
		}
		if(u.field_set & (1 << UF_HOST)) {
			handle->msg.host = (char*)malloc(u.field_data[UF_HOST].len+1);
			strncpy(handle->msg.host, buf+u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);  
			handle->msg.host[u.field_data[UF_HOST].len] = 0;  
		}
		memset(handle->msg.request_path, 0, strlen(handle->msg.request_path));
		if(u.field_set & (1 << UF_PATH))  
		{  
			strncpy(handle->msg.request_path, buf+u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);  
			handle->msg.request_path[u.field_data[UF_PATH].len] = 0;  
		}

	}else {
		cout << "url parser error:" << url_result << endl;
	}

	return 0;
}

int HttpHandle::status_complete_cb (http_parser *p) {
	cout << "status_complete_cb" << endl;
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
//	p->data++;
	return 0;
}

int HttpHandle::header_field_cb (http_parser *p, const char *buf, size_t len)
{
	cout << "header_field_cb" << endl;
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
	cout << "header_value_cb" << endl;
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
	cout << "check-body_is_final" << endl;
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
	cout << "body_cb" << endl;
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	strlncat(handle->msg.body,
		 sizeof(handle->msg.body),
		 buf,
		 len);
	handle->msg.body_size += len;
	check_body_is_final(p);
	// printf("body_cb: '%s'\n", requests[num_messages].body);
	return 0;
}

int HttpHandle::count_body_cb (http_parser *p, const char *buf, size_t len)
{
	cout << "count_body_cb" << endl;
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	assert(buf);
	handle->msg.body_size += len;
	check_body_is_final(p);
	return 0;
}

int HttpHandle::message_begin_cb (http_parser *p)
{	
	cout << "message_begin_cb" << endl;
	HttpHandle *handle = static_cast<HttpHandle*>(p->data);
	assert(p == handle->parser);
	handle->msg.message_begin_cb_called = TRUE;
	return 0;
}

int HttpHandle::headers_complete_cb (http_parser *p)
{
	cout << "headers_complete_cb" << endl;
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
	cout << "message_complete_cb" << endl;
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
	printf("handle request start \n");
	this->printfmsg();
}	

void HttpHandle::printfmsg()
{
	printf("message:\n");
	printf("name:%s\n", msg.name);
	printf("raw:%s\n", msg.raw);
//	printf("type:%\n", &msg.type);
	printf("request_url:%s\n", msg.request_url);
	printf("request_path:%s\n", msg.request_path);
	printf("host:%s\n", msg.host);
	printf("body:%s\n", msg.body);
//	printf("port:%" PRIu16 "\n", msg.port);
	cout << "port:" << msg.port << endl;
	for(int i = 0;i < msg.num_headers; i++) {
		cout << msg.headers[i][0] << ":" << msg.headers[i][1] <<  endl;
	}
//	cout << "message:";
//	cout << "name:" << msg.name << endl;
//	cout << "raw:" << msg.raw << endl;
//	cout << "type:" << msg.type << endl;
//	cout << "request url:" << msg.request_url << endl;
//	cout << "request path" << msg.request_path << endl;
//	cout << "host:" << msg.host << endl;
//	cout << "port:" << msg.port << endl;
	
}

 
}

















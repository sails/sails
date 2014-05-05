#include <common/net/http_connector.h>
#include <stdlib.h>
#include <string.h>
#include <common/base/string.h>

namespace sails {
namespace common {
namespace net {

struct http_parser_settings HttpConnector::settings = {
    on_message_begin : HttpConnector::message_begin_cb,
    on_url : HttpConnector::request_url_cb,
    on_status_complete : HttpConnector::status_complete_cb,
    on_header_field : HttpConnector::header_field_cb,
    on_header_value : HttpConnector::header_value_cb,
    on_headers_complete : HttpConnector::headers_complete_cb,
    on_body : HttpConnector::body_cb,
    on_message_complete : HttpConnector::message_complete_cb
};

HttpConnector::HttpConnector(int connect_fd)
    :Connector(connect_fd)
{
    parser.data = this;

    message = (struct http_message*)malloc(
	sizeof(struct http_message));
    http_message_init(message);

    http_parser_init(&parser, ::HTTP_BOTH);
}

HttpConnector::HttpConnector():Connector()
{
    parser.data = this;

    message = (struct http_message*)malloc(
	sizeof(struct http_message));
    http_message_init(message);

    http_parser_init(&parser, ::HTTP_BOTH);
}

HttpConnector::~HttpConnector()
{
    if(!req_list.empty()) {
	HttpRequest *item = req_list.front();
	req_list.pop_front();
	delete(item);
    }
    if(!rep_list.empty()) {
	HttpResponse *item = rep_list.front();
	rep_list.pop_front();
        delete(item);
    }
}


HttpRequest* HttpConnector::get_next_httprequest()
{
    if(!req_list.empty()) {
	HttpRequest *item = req_list.front();
	req_list.pop_front();
	return item;
    }
    return NULL;
}


HttpResponse* HttpConnector::get_next_httpresponse() {
    if(!rep_list.empty()) {
	HttpResponse *item = rep_list.front();
	rep_list.pop_front();
	return item;
    }
    return NULL;
}


void HttpConnector::httpparser() {
    char data[10000];
    strncpy(data, in_buf.peek(), in_buf.readable());
    //parser, if ok retrieve else ignore
    size_t nparsed = http_parser_execute(&parser, &settings, 
					  data, strlen(data));
    //
    in_buf.retrieve(nparsed);
}

int
HttpConnector::request_url_cb (http_parser *p, const char *buf, size_t len)
{
     if(p->data == NULL) {
	  printf("p->data == null\n");
     }
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     strlncat(m->request_url,
	      sizeof(m->request_url),
	      buf,
	      len);

     return 0;
}

int HttpConnector::status_complete_cb (http_parser *p) {
     HttpConnector* connector = (HttpConnector*)(p->data);
     if(connector) {
	  return 0;	  
     }
     return -1;
}

int HttpConnector::header_field_cb (http_parser *p, const char *buf, size_t len)
{
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     if (m->last_header_element != FIELD)
	  m->num_headers++;
	
     strlncat(m->headers[m->num_headers-1][0],
	      sizeof(m->headers[m->num_headers-1][0]),
	      buf,
	      len);

     m->last_header_element = FIELD;

     return 0;
}

int HttpConnector::header_value_cb (http_parser *p, const char *buf, size_t len)
{

     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     strlncat(m->headers[m->num_headers-1][1],
	      sizeof(m->headers[m->num_headers-1][1]),
	      buf,
	      len);
     m->last_header_element = VALUE;

     return 0;
}

void HttpConnector::check_body_is_final (const http_parser *p)
{
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     if (m->body_is_final) {
	  fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 on last on_body callback call "
		  "but it doesn't! ***\n\n");
	  assert(0);
	  abort();
     }

     m->body_is_final = http_body_is_final(p);
}

int HttpConnector::body_cb (http_parser *p, const char *buf, size_t len)
{
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     strlncat(m->body,
	      sizeof(m->body),
	      buf,
	      len);
     m->body_size += len;
     HttpConnector::check_body_is_final(p);

     return 0;
}

int HttpConnector::count_body_cb (http_parser *p, const char *buf, size_t len)
{
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     assert(buf);
     m->body_size += len;
     HttpConnector::check_body_is_final(p);

     return 0;
}

int HttpConnector::message_begin_cb (http_parser *p)
{
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;
     m->message_begin_cb_called = TRUE;
     return 0;
}

int HttpConnector::headers_complete_cb (http_parser *p)
{
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     m->method = p->method;
     m->status_code = p->status_code;
     m->http_major = p->http_major;
     m->http_minor = p->http_minor;
     m->headers_complete_cb_called = TRUE;
     m->should_keep_alive = http_should_keep_alive(p);

     return 0;
}

int HttpConnector::message_complete_cb (http_parser *p)
{
     HttpConnector* connector = (HttpConnector*)(p->data);
     struct http_message *m = connector->message;

     if (m->should_keep_alive != http_should_keep_alive(p))
     {
	  fprintf(stderr, "\n\n *** Error http_should_keep_alive() should have same "
		  "value in both on_message_complete and on_headers_complete "
		  "but it doesn't! ***\n\n");
	  assert(0);
	  abort();
     }

     if (m->body_size &&
	 http_body_is_final(p) &&
	 !m->body_is_final)
     {
	  fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
		  "on last on_body callback call "
		  "but it doesn't! ***\n\n");
	  assert(0);
	  abort();
     }
	
     m->message_complete_cb_called = TRUE;
     if(m->body_size == 0) {
	  m->message_complete_on_eof = TRUE;
     }
	
     if(p->type == HTTP_REQUEST) {
	 connector->handle_request();
	 HttpRequest* request = new HttpRequest(m);
	 connector->push_request_list(request);
	 
     }else {
	 HttpResponse* response = new HttpResponse(m);
	 connector->push_response_list(response);
     }
     struct http_message* message = (struct http_message*)malloc(
	     sizeof(struct http_message));
     http_message_init(message);
     connector->message = message;

     return 0;
}


void HttpConnector::handle_request()
{
     struct http_message *m = this->message;

     char url[200];
     memset(url, 0, 200);
     strncat(url, "http://", strlen("http://"));
     for(int i = 0; i < m->num_headers; i++) {
	  if(strcmp("Host", m->headers[i][0]) == 0) {
	       strncat(url, m->headers[i][1], strlen(m->headers[i][1]));
	       break;
	  }
     }
     strncat(url, m->request_url, strlen(m->request_url));
	
     parser_url(url);
}	

void HttpConnector::parser_url(char *url)
{
     struct http_parser_url u;
     int url_result = 0;
     struct http_message *m = this->message;
     if((url_result = http_parser_parse_url(url, strlen(url), 0, &u)) == 0)
     {
	  if(u.field_set & (1 << UF_PORT)) {
	       m->port = u.port;
	  }else {
	       m->port = 80;
	  }
	  if(m->host) {
	       free(m->host);
	  }
	  if(u.field_set & (1 << UF_HOST)) {
	       m->host = (char*)malloc(u.field_data[UF_HOST].len+1);
	       strncpy(m->host, url+u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);  
	       m->host[u.field_data[UF_HOST].len] = 0;  
	  }
	  memset(m->request_path, 0, strlen(m->request_path));
	  if(u.field_set & (1 << UF_PATH))  
	  {  
	       strncpy(m->request_path, url+u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);  
	       m->request_path[u.field_data[UF_PATH].len] = 0;  
	  }

     }else {
	  printf("url parser error:%d\n", url_result);
     }
	
}

void HttpConnector::push_request_list(HttpRequest *req) {
    if(req_list.size() <= 20) {
	req_list.push_back(req);
    }else {
	char msg[100];
	memset(msg, '\0', 100);
	sprintf(msg, "connect fd %d unhandle request list more than 20 and can't parser", connect_fd);
	perror(msg);
    }
}

void HttpConnector::push_response_list(HttpResponse *rep) {
    if(rep_list.size() <= 20) {
	rep_list.push_back(rep);
    }else {
	char msg[100];
	memset(msg, '\0', 100);
	sprintf(msg, "connect fd %d unhandle response list more than 20 and can't parser", connect_fd);
	perror(msg);
    }
}
} // namespace net
} // namespace common
} // namespace sails














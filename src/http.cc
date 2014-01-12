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
	  msg->body_is_final = 0;
     }
}


// max connnect fd for store request message and parser instance
int max_connfd = 2000;
int set_max_connfd(int max_fd) {
     max_connfd = max_fd;
     return max_connfd;
}
struct message** message_list = NULL;
struct message* get_message_by_connfd(int connfd) {
     // init array to store message for earch connfd
     if(message_list == NULL) {
	  message_list = (struct message**)malloc(
	       max_connfd*(sizeof(struct message*)));
	  for(int i = 0; i < max_connfd; i++) {
	       message_list[i] = NULL;
	  }
     }
     if(connfd > 0 && connfd < max_connfd) {
	  struct message* msg = message_list[connfd];
	  if(msg == NULL) {
	       msg = (struct message*)malloc(sizeof(struct message));
//	       message_init(msg);
	       message_list[connfd] = msg;
	  }
	  return msg;
     }else {
	  return NULL;
     }
}
void reset_message_by_connfd(int connfd) {
     if(message_list != NULL) {
	  if(connfd > 0 && connfd < max_connfd) {
	       message_list[connfd] = NULL;
	  }
     }
}

http_parser** parser_list = NULL;
http_parser *get_http_parser_by_connfd(int connfd) {
     // init array to store parser for earch connfd
     if(parser_list == NULL) {
	  parser_list = (http_parser**)malloc(
	       max_connfd*(sizeof(http_parser*)));
	  for(int i = 0; i < max_connfd; i++) {
	       parser_list[i] = NULL;
	  }
     }
     if(connfd > 0 && connfd < max_connfd) {
	  http_parser* parser = parser_list[connfd];
	  if(parser == NULL) {
	       parser = (http_parser*)malloc(sizeof(http_parser));
	       http_parser_init(parser, HTTP_BOTH);
	       parser_list[connfd] = parser;
	  }
	  return parser;
     }else {
	  return NULL;
     }
}

HttpHandle* HttpHandle::_instance = NULL;
HttpHandle::HttpHandle() {

     settings.on_message_begin = message_begin_cb;
     settings.on_header_field = header_field_cb;
     settings.on_header_value = header_value_cb;
     settings.on_url = request_url_cb;
     settings.on_body = body_cb;
     settings.on_headers_complete = headers_complete_cb;
     settings.on_message_complete = message_complete_cb;
     settings.on_status_complete = status_complete_cb;
     
}

HttpHandle::~HttpHandle() {
}

	
size_t HttpHandle::parser_http(char *msg_buf, int connfd) { 

     http_parser *parser = get_http_parser_by_connfd(connfd);
     struct message* msg = get_message_by_connfd(connfd);
     if(parser == NULL || msg == NULL) {
	  return -1; //out of max connfd
     }
     parser->data = &connfd;

     if(parser->state == 2 || parser->state == 3 || parser->state == 17) {
	  http_parser_init(parser, HTTP_BOTH);
	  message_init(msg);
     }

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
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);

     strlncat(m->request_url,
	      sizeof(m->request_url),
	      buf,
	      len);

     return 0;
}

int HttpHandle::status_complete_cb (http_parser *p) {
     
     int connfd = *(int*)(p->data);
     if(connfd > 0) {
	  return 0;	  
     }
     return -1;
}

int HttpHandle::header_field_cb (http_parser *p, const char *buf, size_t len)
{
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);

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

     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);

     strlncat(m->headers[m->num_headers-1][1],
	      sizeof(m->headers[m->num_headers-1][1]),
	      buf,
	      len);
     m->last_header_element = VALUE;

     return 0;
}

void HttpHandle::check_body_is_final (const http_parser *p)
{
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);
     if (m->body_is_final) {
	  fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 on last on_body callback call "
		  "but it doesn't! ***\n\n");
	  assert(0);
	  abort();
     }

     m->body_is_final = http_body_is_final(p);
}

int HttpHandle::body_cb (http_parser *p, const char *buf, size_t len)
{
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);
     strlncat(m->body,
	      sizeof(m->body),
	      buf,
	      len);
     m->body_size += len;
     HttpHandle::check_body_is_final(p);

     return 0;
}

int HttpHandle::count_body_cb (http_parser *p, const char *buf, size_t len)
{
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);
     assert(buf);
     m->body_size += len;
     HttpHandle::check_body_is_final(p);

     return 0;
}

int HttpHandle::message_begin_cb (http_parser *p)
{
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);
     m->message_begin_cb_called = TRUE;
     return 0;
}

int HttpHandle::headers_complete_cb (http_parser *p)
{
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);

     m->method = p->method;
     m->status_code = p->status_code;
     m->http_major = p->http_major;
     m->http_minor = p->http_minor;
     m->headers_complete_cb_called = TRUE;
     m->should_keep_alive = http_should_keep_alive(p);

     return 0;
}

int HttpHandle::message_complete_cb (http_parser *p)
{
     int connfd = *(int*)(p->data);
     struct message *m = get_message_by_connfd(connfd);
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
	  HttpHandle::instance()->handle_request(p);
     }

     return 0;
}


void HttpHandle::handle_request(http_parser* parser)
{
     int connfd = *(int*)(parser->data);
     struct message *m = get_message_by_connfd(connfd);

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
	
     parser_url(url, m);

     this->printfmsg(m);
}	

void HttpHandle::parser_url(char *url, struct message *m)
{
     struct http_parser_url u;
     int url_result = 0;
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


void HttpHandle::printfmsg(struct message *m)
{
     printf("message:\n");
     printf("request_url:%s\n", m->request_url);
     printf("port:%d\n", m->port);
     printf("request_path:%s\n", m->request_path);
     printf("host:%s\n", m->host);
     printf("should_keep_alive:%d\n", m->should_keep_alive);

     for(int i = 0;i < m->num_headers; i++) {
	  printf("header %d:%s:%s\n", i, m->headers[i][0], 
		 m->headers[i][1]);
     }

     printf("body:%s\n", m->body);

	
}

 
} // namespace sails

// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: http.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 14:31:46



#ifndef SAILS_NET_HTTP_H_
#define SAILS_NET_HTTP_H_

#include <stdio.h>
#include <map>
#include <string>
#include <list>
#include "sails/net/http_parser.h"


namespace sails {
namespace net {

///////////////////////////// message ///////////////////////////////////
#define MAX_HEADERS 13
#define MAX_PATH_SIZE 1024
#define MAX_ELEMENT_SIZE 2048


enum header_element { NONE = 0, FIELD, VALUE };

//enum http_parser_type { HTTP_REQUEST, HTTP_RESPONSE, HTTP_BOTH };


#undef TRUE
#define TRUE 1
#undef FALSE
#define FALSE 0

struct http_message {
  
  char *raw;
  enum http_parser_type type;
  int method;
  int status_code;
  char response_status[MAX_ELEMENT_SIZE];
  char request_path[MAX_PATH_SIZE];
  char request_url[MAX_PATH_SIZE];
  char fragment[MAX_ELEMENT_SIZE];
  char query_string[MAX_ELEMENT_SIZE];
  char body[MAX_ELEMENT_SIZE];
  size_t body_size;
  char *host;
  char *userinfo;
  uint16_t port;
  int num_headers;
  enum header_element last_header_element;
  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE];
  int should_keep_alive;

  char *upgrade;  // upgraded body

  int16_t http_major;
  int16_t http_minor;

  int message_begin_cb_called;
  int headers_complete_cb_called;
  int message_complete_cb_called;
  int message_complete_on_eof;
  int body_is_final;
};

typedef struct http_message http_message;

void http_message_init(http_message *msg);
void delete_http_message(http_message *msg);


//////////////////////////// http request ////////////////////////////////
class HttpRequest {
 public:
  explicit HttpRequest(struct http_message *raw_data);

  ~HttpRequest();

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
  struct http_message *raw_data;
 private:
  std::map<std::string, std::string> param;
};



/////////////////////////// http response //////////////////////////////
class HttpResponse {
 public:
  HttpResponse();
  explicit HttpResponse(struct http_message *raw_data);
  ~HttpResponse();

  void set_http_proto(int http_major, int http_minor);
  void set_response_status(int response_status);
  int set_header(const char* key, const char *value);
  int set_body(const char* body);
  char *get_body();
  int to_str();
  char* get_raw();
 public:
  int connfd;
 private:
  struct http_message *raw_data;
  void set_default_header();
};


class ParserFlag {
 public:
  ParserFlag() {flag = 0; message = NULL;}
  int flag;  // 解析状态(0成功,-1失败)
  std::list<http_message*> messageList;  // 用来缓存在解析过程中已经解析出来的消息
  http_message *message;  // 正在解析的消息
};


// 通过parser解析msg中的内容,将结果放到flag中,返回解析的字符数
// flag是上次解析后的结果,因为解析是有状态的
// 注意,parser的data属性在解析过程中会被重新设置,所以不要在data里保存数据
int sails_http_parser(
    http_parser* paser, const char* buf, ParserFlag* flag);

int message_begin_cb (http_parser *p);
int header_field_cb (http_parser *p, const char *buf, size_t len);
int header_value_cb (http_parser *p, const char *buf, size_t len);
int request_url_cb (http_parser *p, const char *buf, size_t len);
int response_status_cb (http_parser *p, const char *buf, size_t len);
int body_cb (http_parser *p, const char *buf, size_t len);
void check_body_is_final (const http_parser *p);
int headers_complete_cb (http_parser *p);
int message_complete_cb (http_parser *p);

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET _HTTP_H_



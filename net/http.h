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
#include <stdint.h>
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
#define MAX_BODY_SIZE 20480


enum header_element { NONE = 0, FIELD, VALUE };

// enum http_parser_type { HTTP_REQUEST, HTTP_RESPONSE, HTTP_BOTH };


#undef TRUE
#define TRUE 1
#undef FALSE
#define FALSE 0

struct http_message {
  char *raw;
  enum http_parser_type type;
  int method;
  int status_code;  // only for response
  char response_status[MAX_ELEMENT_SIZE];
  char request_path[MAX_PATH_SIZE];
  char request_url[MAX_PATH_SIZE];
  char fragment[MAX_ELEMENT_SIZE];
  char query_string[MAX_ELEMENT_SIZE];
  char body[MAX_BODY_SIZE];
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
int message_to_string(struct http_message *msg, char* data, int len);


//////////////////////////// http request //////////////////////////////
class HttpRequest {
 public:
  explicit HttpRequest(struct http_message *raw_data);

  ~HttpRequest();

  std::string GetRequestPath();
  std::string GetParam(std::string param_name);

  // new request for rpc client to send data
  void SetHttpProto(int http_major, int http_minor);
  void SetRequestMethod(int method);
  void SetDefaultHeader();
  int SetHeader(const char* key, const char *value);
  int SetBody(const char* body, int len);
  // 得到request的原始内容,传入一个char* data;方法会把原始值复制到data中
  // 返回值,0表示成功,否则失败.
  int ToString(char *data, int len);
 public:
  struct http_message *raw_data;
 private:
  std::map<std::string, std::string> param;
};



/////////////////////////// http response //////////////////////////////

class HttpResponse {
 public:
  // See RFC2616
  enum StatusCode {
    Status_Continue           = 100,
    Status_SwitchingProtocols = 101,

    Status_OK                          = 200,
    Status_Created                     = 201,
    Status_Accepted                    = 202,
    Status_NonAuthoritativeInformation = 203,
    Status_NoContent                   = 204,
    Status_ResetContent                = 205,
    Status_PartialContent              = 206,

    Status_MultipleChoices   = 300,
    Status_MovedPermanently  = 301,
    Status_Found             = 302,
    Status_SeeOther          = 303,
    Status_NotModified       = 304,
    Status_UseProxy          = 305,
    Status_TemporaryRedirect = 307,

    Status_BadRequest                   = 400,
    Status_Unauthorized                 = 401,
    Status_PaymentRequired              = 402,
    Status_Forbidden                    = 403,
    Status_NotFound                     = 404,
    Status_MethodNotAllowed             = 405,
    Status_NotAcceptable                = 406,
    Status_ProxyAuthRequired            = 407,
    Status_RequestTimeout               = 408,
    Status_Conflict                     = 409,
    Status_Gone                         = 410,
    Status_LengthRequired               = 411,
    Status_PreconditionFailed           = 412,
    Status_RequestEntityTooLarge        = 413,
    Status_RequestURITooLong            = 414,
    Status_UnsupportedMediaType         = 415,
    Status_RequestedRangeNotSatisfiable = 416,
    Status_ExpectationFailed            = 417,

    Status_InternalServerError     = 500,
    Status_NotImplemented          = 501,
    Status_BadGateway              = 502,
    Status_ServiceUnavailable      = 503,
    Status_GatewayTimeout          = 504,
    Status_HTTPVersionNotSupported = 505,
  };

 public:
  HttpResponse();
  explicit HttpResponse(struct http_message *raw_data);
  ~HttpResponse();

  static const char* StatusCodeToReasonPhrase(int status_code);

  void SetHttpProto(int http_major, int http_minor);
  void SetResponseStatus(int response_status);
  int SetHeader(const char* key, const char *value);
  // 可能有些body中含有'\0',所以不能直接通过strlen(body)来进行赋值
  int SetBody(const char* body, int len);
  char *GetBody();

  // 得到response的原始内容,传入一个char* data;方法会把原始值复制到data中
  // 返回值,0表示成功,否则失败.
  int ToString(char* data, int len);

 public:
  int connfd;

 private:
  struct http_message *raw_data;
  void SetDefaultHeader();
};



/////////////////////////// http parser ///////////////////////////////
class ParserFlag {
 public:
  ParserFlag() {flag = 0; message = NULL;}
  int flag;  // 解析状态(0成功,-1失败)
  // 用来缓存在解析过程中已经解析出来的消息
  std::list<http_message*> messageList;
  http_message *message;  // 正在解析的消息
};


// 通过parser解析msg中的内容,将结果放到flag中,返回解析的字符数
// flag是上次解析后的结果,因为解析是有状态的
// 注意,parser的data属性在解析过程中会被重新设置,所以不要在data里保存数据
int sails_http_parser(
    http_parser* paser, const char* buf, ParserFlag* flag);

int message_begin_cb(http_parser *p);
int header_field_cb(http_parser *p, const char *buf, size_t len);
int header_value_cb(http_parser *p, const char *buf, size_t len);
int request_url_cb(http_parser *p, const char *buf, size_t len);
int response_status_cb(http_parser *p, const char *buf, size_t len);
int body_cb(http_parser *p, const char *buf, size_t len);
void check_body_is_final(const http_parser *p);
int headers_complete_cb(http_parser *p);
int message_complete_cb(http_parser *p);
void parser_url(struct http_message* message);

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET _HTTP_H_









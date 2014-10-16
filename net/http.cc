// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: http.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 14:38:00



#include "sails/net/http.h"
#include <stdlib.h>
#include <string.h>
#include "sails/base/string.h"

namespace sails {
namespace net {


///////////////////////// http message ////////////////////////////////////
void http_message_init(struct http_message *msg) {
  if (msg != NULL) {
    // use once memset
    memset(msg, 0, sizeof(struct http_message));
    msg->raw = NULL;
    msg->method = 0;
    msg->status_code = 0;
    msg->body_size = 0;
    msg->host = NULL;
    msg->userinfo = NULL;
    msg->port = 0;
    msg->num_headers = 0;

    msg->last_header_element = NONE;
    msg->should_keep_alive = 1;
    msg->upgrade = NULL;
    msg->http_major = 1;
    msg->http_minor = 1;
    msg->body_is_final = 0;
  }
}

void delete_http_message(struct http_message *msg) {
  if (msg != NULL) {
    if (msg->host != NULL) {
      free(msg->host);
      msg->host = NULL;
    }
    if (msg->userinfo != NULL) {
      free(msg->userinfo);
      msg->userinfo = NULL;
    }
    if (msg->raw != NULL) {
      free(msg->raw);
    }
    free(msg);
  }
}


///////////////////////////// http request ////////////////////////////////
HttpRequest::HttpRequest(struct http_message *raw_data) {
  this->raw_data = raw_data;
}

HttpRequest::~HttpRequest() {
  if (this->raw_data != NULL) {
    delete_http_message(this->raw_data);
  }
}

std::string HttpRequest::getparam(std::string param_name) {
  std::map<std::string, std::string>::iterator iter
      = this->param.find(param_name);
  if (iter == param.end()) {
    if (param.empty() && this->raw_data != NULL) {
      for (int i = 0; i < raw_data->num_headers; i++) {
        std::string key(raw_data->headers[i][0]);
        std::string value(raw_data->headers[i][1]);
        param.insert(
            std::map<std::string, std::string>::value_type(key, value));
      }
      iter =  this->param.find(param_name);
    }
  }

  if (iter == param.end()) {
    return "";
  } else {
    return iter->second;
  }
}

void HttpRequest::set_default_header() {
  this->set_http_proto(1, 1);
  this->raw_data->method = 1;
  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE]
      = {{ "Accept",
           "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8"},  // NOLINT'
         { "Accept-Encoding", "gzip,deflate,sdch"},
         { "Accept-Language", "en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4"},
         { "Connection", "keep-alive"},
         { "Content-Length", "0"},
         { "Host", "" },
         { "User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/31.0.1650.63 Safari/537.36" }};  // NOLINT'

  for (int i = 0; i < MAX_HEADERS; i++) {
    this->set_header(headers[i][0], headers[i][1]);
  }
}
void HttpRequest::set_http_proto(int http_major, int http_minor) {
  this->raw_data->http_major = http_major;
  this->raw_data->http_minor = http_minor;
}

void HttpRequest::set_request_method(int method) {
  this->raw_data->method = method;
}
int HttpRequest::set_header(const char* key, const char *value) {
  if (key != NULL && value != NULL && strlen(key) > 0) {
    int exist = 0;
    for (int i = 0; i < raw_data->num_headers; i++) {
      if (strcasecmp(raw_data->headers[i][0], key) == 0) {
        memset(raw_data->headers[i][1],
               0, MAX_ELEMENT_SIZE);
        strcpy(raw_data->headers[i][1], value);  // NOLINT'
        exist = 1;
      }
    }
    if (!exist) {
      // add header
      raw_data->num_headers++;
      strcpy(raw_data->headers[raw_data->num_headers-1][0], key);  // NOLINT'
      strcpy(raw_data->headers[raw_data->num_headers-1][1],value);  // NOLINT'
    }
  }
  return 1;
}
int HttpRequest::set_body(const char* body) {
  if (body != NULL && strlen(body) > 0) {
    int body_size = strlen(body);
    memset(raw_data->body, 0, MAX_ELEMENT_SIZE);
    strncpy(raw_data->body, body, body_size);
    return 0;
  }
  return 1;
}
int HttpRequest::to_str() {
  if (this->raw_data != NULL) {
    if (raw_data->raw == NULL) {
      raw_data->raw = (char*)malloc(1024*10);
    }

    memset(raw_data->raw, 0, 1024*10);

    char method_str[10] = {0};
    if (raw_data->method == 1) {
      sprintf(method_str, "%s", "GET");
    } else {
      sprintf(method_str, "%s", "POST");
    }
    // status
    sprintf(raw_data->raw, "%s / HTTP/%d.%d\r\n",
            method_str,
            raw_data->http_major,
            raw_data->http_minor);

    // header
    for (int i = 0; i < raw_data->num_headers; i++) {
      strncpy(raw_data->raw+strlen(raw_data->raw),
              raw_data->headers[i][0],
              strlen(raw_data->headers[i][0]));
      raw_data->raw[strlen(raw_data->raw)] = ':';
      strncpy(raw_data->raw+strlen(raw_data->raw),
              raw_data->headers[i][1],
              strlen(raw_data->headers[i][1]));
      raw_data->raw[strlen(raw_data->raw)] = '\r';
      raw_data->raw[strlen(raw_data->raw)] = '\n';
    }

    // empty line
    raw_data->raw[strlen(raw_data->raw)] = '\r';
    raw_data->raw[strlen(raw_data->raw)] = '\n';

    // body
    strncpy(raw_data->raw+strlen(raw_data->raw),
            raw_data->body,
            strlen(raw_data->body));

    raw_data->raw[strlen(raw_data->raw)] = '\r';
    raw_data->raw[strlen(raw_data->raw)] = '\n';
    return 0;
  }
  return 1;
}
char* HttpRequest::get_raw() {
  return this->raw_data->raw;
}



///////////////////////////// http response ////////////////////////////
HttpResponse::HttpResponse() {
  this->raw_data = (struct http_message *)malloc(sizeof(struct http_message));
  http_message_init(this->raw_data);
  this->raw_data->type = HTTP_RESPONSE;
  this->set_default_header();
}

HttpResponse::HttpResponse(struct http_message *raw_data) {
  this->raw_data = raw_data;
}

HttpResponse::~HttpResponse() {
  if (this->raw_data != NULL) {
    delete_http_message(this->raw_data);
  }
}

void HttpResponse::set_http_proto(int http_major, int http_minor) {
  this->raw_data->http_major = http_major;
  this->raw_data->http_minor = http_minor;
}

void HttpResponse::set_response_status(int response_status) {
  this->raw_data->status_code = response_status;
}

int HttpResponse::set_header(const char *key, const char *value) {
  if (key != NULL && value != NULL && strlen(key) > 0) {
    int exist = 0;
    for (int i = 0; i < raw_data->num_headers; i++) {
      if (strcasecmp(raw_data->headers[i][0], key) == 0) {
        memset(raw_data->headers[i][1],
               0, MAX_ELEMENT_SIZE);
        strcpy(raw_data->headers[i][1], value);
        exist = 1;
      }
    }
    if (!exist) {
      // add header
      raw_data->num_headers++;
      strcpy(raw_data->headers[raw_data->num_headers-1][0],
             key);
      strcpy(raw_data->headers[raw_data->num_headers-1][1],
             value);
    }
  }
  return 1;
}

int HttpResponse::set_body(const char *body) {
  if (body != NULL && strlen(body) > 0) {
    int body_size = strlen(body);
    memset(raw_data->body, 0, MAX_ELEMENT_SIZE);
    strncpy(raw_data->body, body, body_size);
    char body_size_str[11];
    sprintf(body_size_str, "%d", body_size);
    this->set_header("Content-Length", body_size_str);
    return 0;
  }
  return 1;
}

char *HttpResponse::get_body() {
  return this->raw_data->body;
}

int HttpResponse::to_str() {
  if (this->raw_data != NULL) {
    if (raw_data->raw == NULL) {
      raw_data->raw = (char*)malloc(1024*10);
    }

    memset(raw_data->raw, 0, 1024*10);

    // status
    sprintf(raw_data->raw, "HTTP/%d.%d %d OK\r\n",
            raw_data->http_major,
            raw_data->http_minor,
            raw_data->status_code);

    // header
    for (int i = 0; i < raw_data->num_headers; i++) {
      strncpy(raw_data->raw+strlen(raw_data->raw),
              raw_data->headers[i][0],
              strlen(raw_data->headers[i][0]));
      raw_data->raw[strlen(raw_data->raw)] = ':';
      strncpy(raw_data->raw+strlen(raw_data->raw),
              raw_data->headers[i][1],
              strlen(raw_data->headers[i][1]));
      raw_data->raw[strlen(raw_data->raw)] = '\r';
      raw_data->raw[strlen(raw_data->raw)] = '\n';
    }

    // empty line
    raw_data->raw[strlen(raw_data->raw)] = '\r';
    raw_data->raw[strlen(raw_data->raw)] = '\n';

    // body
    strncpy(raw_data->raw+strlen(raw_data->raw),
            raw_data->body,
            strlen(raw_data->body));

    raw_data->raw[strlen(raw_data->raw)] = '\r';
    raw_data->raw[strlen(raw_data->raw)] = '\n';

    return 0;
  }
  return 1;
}

char* HttpResponse::get_raw() {
  return this->raw_data->raw;
}

void HttpResponse::set_default_header() {
  this->set_http_proto(1, 1);
  this->set_response_status(200);
  time_t timep;
  time(&timep);

  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE]
      = {{ "Location", "localhost/cust"},
         { "Content-Type", "text/html;charset=UTF-8"},
         { "Date", ""},
         { "Expires", "" },
         { "Connection", "keep-alive"},
         { "Cache-Control", "public, max-age=0" },
         { "Server", "sails server" },
         { "Content-Length", "0" }};

  for (int i = 0; i < MAX_HEADERS; i++) {
    this->set_header(headers[i][0], headers[i][1]);
  }
  // local time
  char *time_str = ctime(&timep);
  time_str[strlen(time_str)-1] = 0;  // delete '\n'
  this->set_header("Date", time_str);
  this->set_header("Expires", time_str);
}











static http_parser_settings settings =
{.on_message_begin = message_begin_cb
 ,.on_url = request_url_cb
 ,.on_status = response_status_cb
 ,.on_header_field = header_field_cb
 ,.on_header_value = header_value_cb
 ,.on_headers_complete = headers_complete_cb
 ,.on_body = body_cb
 ,.on_message_complete = message_complete_cb
  };


int message_begin_cb (http_parser *p)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  flag->message->message_begin_cb_called = TRUE;
  return 0;
}


int header_field_cb (http_parser *p, const char *buf, size_t len)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;

  if (m->last_header_element != FIELD)
    m->num_headers++;

  base::strlncat(m->headers[m->num_headers-1][0],
           sizeof(m->headers[m->num_headers-1][0]),
           buf,
           len);

  m->last_header_element = FIELD;

  return 0;
}

int header_value_cb (http_parser *p, const char *buf, size_t len)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;

  base::strlncat(m->headers[m->num_headers-1][1],
           sizeof(m->headers[m->num_headers-1][1]),
           buf,
           len);

  m->last_header_element = VALUE;

  return 0;
}

int request_url_cb (http_parser *p, const char *buf, size_t len)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;

  base::strlncat(m->request_url,
                 sizeof(m->request_url),
                 buf,
                 len);
  return 0;
}

int response_status_cb (http_parser *p, const char *buf, size_t len)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;
  base::strlncat(m->response_status,
           sizeof(m->response_status),
           buf,
           len);
  return 0;
}

int body_cb (http_parser *p, const char *buf, size_t len)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;
  
  base::strlncat(m->body,
           sizeof(m->body),
           buf,
           len);
  m->body_size += len;
  check_body_is_final(p);
 // printf("body_cb: '%s'\n", requests[num_messages].body);
  return 0;
}

void check_body_is_final (const http_parser *p)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;
  if (m->body_is_final) {
    fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
                    "on last on_body callback call "
                    "but it doesn't! ***\n\n");
    flag->flag = -1;
    return;
  }
  m->body_is_final = http_body_is_final(p);
}

int headers_complete_cb (http_parser *p)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;
  
  m->method = p->method;
  m->status_code = p->status_code;
  m->http_major = p->http_major;
  m->http_minor = p->http_minor;
  m->headers_complete_cb_called = TRUE;
  m->should_keep_alive = http_should_keep_alive(p);
  return 0;
}

int message_complete_cb (http_parser *p)
{
  ParserFlag* flag = (ParserFlag*)p->data;
  struct http_message *m = flag->message;
  if (m->should_keep_alive != http_should_keep_alive(p))
  {
    fprintf(stderr, "\n\n *** Error http_should_keep_alive() should have same "
                    "value in both on_message_complete and on_headers_complete "
                    "but it doesn't! ***\n\n");
    flag->flag = -1;
    return -1;
  }

  if (m->body_size &&
      http_body_is_final(p) &&
      !m->body_is_final)
  {
    fprintf(stderr, "\n\n *** Error http_body_is_final() should return 1 "
                    "on last on_body callback call "
                    "but it doesn't! ***\n\n");
    flag->flag = -1;
    return -1;
  }

  m->message_complete_cb_called = TRUE;
  m->message_complete_on_eof = TRUE;

  flag->messageList.push_back(flag->message);
  http_message* message = (http_message*)malloc(sizeof(http_message));
  http_message_init(message);
  flag->message = message;

  return 0;
}



int sails_http_parser(
    http_parser* parser, const char* buf,  ParserFlag* flag) {
  parser->data = flag;

  int parsed
      = http_parser_execute(parser, &settings, buf, strlen(buf));
  
  return parsed;
}

}  // namespace net
}  // namespace sails


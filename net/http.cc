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

int message_to_string(struct http_message *msg, char* data, int len) {
  if (msg != NULL) {
    if (msg->raw != NULL) {
      free(msg->raw);
      msg->raw = NULL;
    }
    // 计算长度
    // status line
    char statusLine[MAX_ELEMENT_SIZE+30] = {'\0'};
    if (msg->type == HTTP_REQUEST) {
      char method_str[10] = {0};
      if (msg->method == 1) {
        snprintf(method_str, sizeof(method_str), "%s", "GET");
      } else {
        snprintf(method_str, sizeof(method_str), "%s", "GET");
      }

      snprintf(statusLine, sizeof(statusLine), "%s %s HTTP/%d.%d",
              method_str,
              msg->request_url,
              msg->http_major,
              msg->http_minor);
    } else {
      snprintf(statusLine, sizeof(statusLine), "HTTP/%d.%d %d %s",
            msg->http_major,
            msg->http_minor,
            msg->status_code,
            HttpResponse::StatusCodeToReasonPhrase(msg->status_code));
    }
    uint32_t statusLen = strlen(statusLine)+2;  // status

    uint32_t headLen = 0;
    // header len
    for (int i = 0; i < msg->num_headers; i++) {
      headLen += strlen(msg->headers[i][0]);
      headLen += strlen(msg->headers[i][1]);
      headLen += 3;  // : '\r' '\n'
    }
    headLen = headLen + 2;  // head和body之间的空行
    // body len
    /*
    uint32_t bodyLen = strlen(msg->body)+2;

    int totalLen = statusLen + headLen + bodyLen + 1; // 加1,最后放str的结束符
    */
    int totalLen = statusLen + headLen + (msg->body_size+2) + 1;

    // 分配原始内容空间
    msg->raw = reinterpret_cast<char*>(malloc(totalLen));
    memset(msg->raw, 0, totalLen);

    // 开始to string

    // status line
    strncpy(msg->raw, statusLine, strlen(statusLine));
    msg->raw[strlen(msg->raw)] = '\r';
    msg->raw[strlen(msg->raw)] = '\n';

    // header
    for (int i = 0; i < msg->num_headers; i++) {
      strncpy(msg->raw+strlen(msg->raw),
              msg->headers[i][0],
              strlen(msg->headers[i][0]));
      msg->raw[strlen(msg->raw)] = ':';
      strncpy(msg->raw+strlen(msg->raw),
              msg->headers[i][1],
              strlen(msg->headers[i][1]));
      msg->raw[strlen(msg->raw)] = '\r';
      msg->raw[strlen(msg->raw)] = '\n';
    }

    // empty line
    msg->raw[strlen(msg->raw)] = '\r';
    msg->raw[strlen(msg->raw)] = '\n';

    // body
    /*
    strncpy(msg->raw+strlen(msg->raw),
            msg->body,
            strlen(msg->body));
    */
    memcpy(msg->raw+strlen(msg->raw),
           msg->body,
           msg->body_size);
    msg->raw[totalLen-3] = '\r';
    msg->raw[totalLen-2] = '\n';


    int cpyLen = totalLen > len ? len:totalLen;
    memcpy(data, msg->raw, cpyLen);

    return 0;
  }
  return 1;
}


///////////////////////////// http request ////////////////////////////////
HttpRequest::HttpRequest(struct http_message *raw_data) {
  this->raw_data = raw_data;
  this->raw_data->type = HTTP_REQUEST;
}

HttpRequest::~HttpRequest() {
  if (this->raw_data != NULL) {
    delete_http_message(this->raw_data);
  }
}

std::string HttpRequest::GetRequestPath() {
  if (this->raw_data == NULL) {
    return "";
  }
  return std::string(raw_data->request_path);
}

std::string HttpRequest::GetParam(std::string param_name) {
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

void HttpRequest::SetDefaultHeader() {
  this->SetHttpProto(1, 1);
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
    this->SetHeader(headers[i][0], headers[i][1]);
  }
}
void HttpRequest::SetHttpProto(int http_major, int http_minor) {
  this->raw_data->http_major = http_major;
  this->raw_data->http_minor = http_minor;
}

void HttpRequest::SetRequestMethod(int method) {
  this->raw_data->method = method;
}
int HttpRequest::SetHeader(const char* key, const char *value) {
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
int HttpRequest::SetBody(const char* body, int len) {
  if (len > MAX_BODY_SIZE) {
    return 1;
  }
  raw_data->body_size = len;
  memset(raw_data->body, 0, MAX_BODY_SIZE);
  memcpy(raw_data->body, body, raw_data->body_size);
  char body_size_str[11];
  snprintf(body_size_str, sizeof(body_size_str), "%ld", raw_data->body_size);
  this->SetHeader("Content-Length", body_size_str);
  return 0;
}
int HttpRequest::ToString(char* data, int len) {
  if (this->raw_data != NULL) {
    return message_to_string(this->raw_data, data, len);
  }
  return 1;
}




///////////////////////////// http response ////////////////////////////


static const struct {
  int status_code;
  const char* reason_phrase;
  const char* status_description;
} kResponseStatus[] = {
  { 100, "Continue", "Request received, please continue" },
  { 101, "Switching Protocols",
    "Switching to new protocol; obey Upgrade header" },
  { 200, "OK", "Request fulfilled, document follows" },
  { 201, "Created", "Document created, URL follows" },
  { 202, "Accepted", "Request accepted, processing continues off-line" },
  { 203, "Non-Authoritative Information", "Request fulfilled from cache" },
  { 204, "No Content", "Request fulfilled, nothing follows" },
  { 205, "Reset Content", "Clear input form for further input." },
  { 206, "Partial Content", "Partial content follows." },
  { 300, "Multiple Choices", "Object has several resources -- see URI list" },
  { 301, "Moved Permanently", "Object moved permanently -- see URI list" },
  { 302, "Found", "Object moved temporarily -- see URI list" },
  { 303, "See Other", "Object moved -- see Method and URL list" },
  { 304, "Not Modified", "Document has not changed since given time" },
  { 305, "Use Proxy",
    "You must use proxy specified in Location to access this resource." },
  { 307, "Temporary Redirect", "Object moved temporarily -- see URI list" },
  { 400, "Bad Request", "Bad request syntax or unsupported method" },
  { 401, "Unauthorized", "No permission -- see authorization schemes" },
  { 402, "Payment Required", "No payment -- see charging schemes" },
  { 403, "Forbidden", "Request forbidden -- authorization will not help" },
  { 404, "Not Found", "Nothing matches the given URI" },
  { 405, "Method Not Allowed",
    "Specified method is invalid for this resource." },
  { 406, "Not Acceptable", "URI not available in preferred format." },
  { 407, "Proxy Authentication Required",
    "You must authenticate with this proxy before proceeding." },
  { 408, "Request Timeout", "Request timed out; try again later." },
  { 409, "Conflict", "Request conflict." },
  { 410, "Gone", "URI no longer exists and has been permanently removed." },
  { 411, "Length Required", "Client must specify Content-Length." },
  { 412, "Precondition Failed", "Precondition in headers is false." },
  { 413, "Request Entity Too Large", "Entity is too large." },
  { 414, "Request-URI Too Long", "URI is too long." },
  { 415, "Unsupported Media Type", "Entity body in unsupported format." },
  { 416, "Requested Range Not Satisfiable", "Cannot satisfy request range." },
  { 417, "Expectation Failed", "Expect condition could not be satisfied." },
  { 500, "Internal Server Error", "Server got itself in trouble" },
  { 501, "Not Implemented", "Server does not support this operation" },
  { 502, "Bad Gateway", "Invalid responses from another server/proxy." },
  { 503, "Service Unavailable",
    "The server cannot process the request due to a high load" },
  { 504, "Gateway Timeout",
    "The gateway server did not receive a timely response" },
  { 505, "HTTP Version Not Supported", "Cannot fulfill request." },
  { -1, NULL, NULL },
};


HttpResponse::HttpResponse() {
  this->raw_data = (struct http_message *)malloc(sizeof(struct http_message));
  http_message_init(this->raw_data);
  this->raw_data->type = HTTP_RESPONSE;
  this->SetDefaultHeader();
}

HttpResponse::HttpResponse(struct http_message *raw_data) {
  this->raw_data = raw_data;
}

HttpResponse::~HttpResponse() {
  if (this->raw_data != NULL) {
    delete_http_message(this->raw_data);
  }
}

const char* HttpResponse::StatusCodeToReasonPhrase(int status_code) {
  for (int i = 0; kResponseStatus[i].status_code != -1; ++i) {
    if (kResponseStatus[i].status_code == status_code) {
      return kResponseStatus[i].reason_phrase;
    }
  }
  return NULL;
}

void HttpResponse::SetHttpProto(int http_major, int http_minor) {
  this->raw_data->http_major = http_major;
  this->raw_data->http_minor = http_minor;
}

void HttpResponse::SetResponseStatus(int response_status) {
  this->raw_data->status_code = response_status;
}

int HttpResponse::SetHeader(const char *key, const char *value) {
  if (key != NULL && value != NULL && strlen(key) > 0) {
    int exist = 0;
    for (int i = 0; i < raw_data->num_headers; i++) {
      if (strcasecmp(raw_data->headers[i][0], key) == 0) {
        memset(raw_data->headers[i][1],
               0, MAX_ELEMENT_SIZE);
        snprintf(raw_data->headers[i][1], sizeof(raw_data->headers[i][1]),
                 "%s", value);
        exist = 1;
      }
    }
    if (!exist) {
      // add header
      raw_data->num_headers++;
      snprintf(raw_data->headers[raw_data->num_headers-1][0],
               sizeof(raw_data->headers[raw_data->num_headers-1][0]),
               "%s", key);
      snprintf(raw_data->headers[raw_data->num_headers-1][1],
               sizeof(raw_data->headers[raw_data->num_headers-1][1]),
               "%s", value);
    }
  }
  return 1;
}

int HttpResponse::SetBody(const char *body, int len) {
  if (len > MAX_BODY_SIZE) {
    return 1;
  }
  raw_data->body_size = len;
  memset(raw_data->body, 0, MAX_BODY_SIZE);
  memcpy(raw_data->body, body, raw_data->body_size);
  char body_size_str[11];
  snprintf(body_size_str, sizeof(body_size_str), "%ld", raw_data->body_size);
  this->SetHeader("Content-Length", body_size_str);
  return 0;
}

char *HttpResponse::GetBody() {
  return this->raw_data->body;
}

int HttpResponse::ToString(char* data, int len) {
  if (this->raw_data != NULL) {
    return message_to_string(this->raw_data, data, len);
  }
  return 1;
}

void HttpResponse::SetDefaultHeader() {
  this->SetHttpProto(1, 1);
  this->SetResponseStatus(200);
  time_t timep;
  time(&timep);

  char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE]
      = {{ "Content-Type", "text/html;charset=UTF-8"},
         { "Date", ""},
         { "Expires", "" },
         { "Connection", "keep-alive"},
         { "Cache-Control", "public, max-age=0" },
         { "Server", "sails server" },
         { "Content-Length", "0" }};

  for (int i = 0; i < MAX_HEADERS; i++) {
    this->SetHeader(headers[i][0], headers[i][1]);
  }
  // local time
  char time_str[40] = {'\0'};
  ctime_r(&timep, time_str);
  time_str[strlen(time_str)-1] = 0;  // delete '\n'
  this->SetHeader("Date", time_str);
  this->SetHeader("Expires", time_str);
}

























///////////////////// http_message /////////////////////////////


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


int message_begin_cb(http_parser *p) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
  flag->message->message_begin_cb_called = TRUE;
  return 0;
}


int header_field_cb(http_parser *p, const char *buf, size_t len) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
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

int header_value_cb(http_parser *p, const char *buf, size_t len) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
  struct http_message *m = flag->message;

  base::strlncat(m->headers[m->num_headers-1][1],
                 sizeof(m->headers[m->num_headers-1][1]),
                 buf,
                 len);

  m->last_header_element = VALUE;

  return 0;
}

int request_url_cb(http_parser *p, const char *buf, size_t len) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
  struct http_message *m = flag->message;

  base::strlncat(m->request_url,
                 sizeof(m->request_url),
                 buf,
                 len);
  return 0;
}

int response_status_cb(http_parser *p, const char *buf, size_t len) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
  struct http_message *m = flag->message;
  base::strlncat(m->response_status,
                 sizeof(m->response_status),
                 buf,
                 len);
  return 0;
}

int body_cb(http_parser *p, const char *buf, size_t len) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
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

void check_body_is_final(const http_parser *p) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
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

int headers_complete_cb(http_parser *p) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
  struct http_message *m = flag->message;

  m->method = p->method;
  m->status_code = p->status_code;
  m->http_major = p->http_major;
  m->http_minor = p->http_minor;
  m->headers_complete_cb_called = TRUE;
  m->should_keep_alive = http_should_keep_alive(p);
  return 0;
}

int message_complete_cb(http_parser *p) {
  ParserFlag* flag = reinterpret_cast<ParserFlag*>(p->data);
  struct http_message *m = flag->message;
  if (m->should_keep_alive != http_should_keep_alive(p)) {
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
  parser_url(m);
  flag->messageList.push_back(flag->message);
  http_message* message = reinterpret_cast<http_message*>(
      malloc(sizeof(http_message)));
  http_message_init(message);
  flag->message = message;

  return 0;
}

void parser_url(struct http_message* m) {
  char url[MAX_PATH_SIZE];
  memset(url, 0, MAX_PATH_SIZE);
  strncat(url, "http://", strlen("http://"));
  for (int i = 0; i < m->num_headers; i++) {
    if (strcmp("Host", m->headers[i][0]) == 0) {
      strncat(url, m->headers[i][1], strlen(m->headers[i][1]));
      break;
    }
  }
  strncat(url, m->request_url, strlen(m->request_url));

  struct http_parser_url u;
  int url_result = 0;

  if ((url_result = http_parser_parse_url(url, strlen(url), 0, &u)) == 0) {
    if (u.field_set & (1 << UF_PORT)) {
      m->port = u.port;
    } else {
      m->port = 80;
    }
    if (m->host) {
      free(m->host);
    }
    if (u.field_set & (1 << UF_HOST)) {
      m->host = reinterpret_cast<char*>(
          malloc(u.field_data[UF_HOST].len+1));
      strncpy(
          m->host, url+u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
      m->host[u.field_data[UF_HOST].len] = 0;
    }
    memset(m->request_path, 0, strlen(m->request_path));
    if (u.field_set & (1 << UF_PATH)) {
      strncpy(m->request_path, url+u.field_data[UF_PATH].off,
              u.field_data[UF_PATH].len);
      m->request_path[u.field_data[UF_PATH].len] = 0;
    }
  } else {
    printf("url parser error:%d\n", url_result);
  }
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


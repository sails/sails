#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
namespace sails {

Request::Request(struct message *raw_data) {
     this->raw_data = raw_data;
}

Request::~Request() {
     if(this->raw_data != NULL) {
	  delete_message(this->raw_data);
     }
}

std::string Request::getparam(std::string param_name) {
     map<string, string>::iterator iter = this->param.find(param_name);
     if(iter == param.end()) {
	  if(param.empty() && this->raw_data != NULL) {
	       for(int i = 0; i < raw_data->num_headers; i++) {
		    string key(raw_data->headers[i][0]);
		    string value(raw_data->headers[i][1]);
		    param.insert(map<string,string>::value_type(key,value));
	       }
	       iter =  this->param.find(param_name);
	  }
     }

     if(iter == param.end()){
	  return "";
     }else {
	  return iter->second;
     }
}

void Request::set_default_header() {
     this->set_http_proto(1, 1);
     this->raw_data->method = 1;
     char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE] = 
	  {{ "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8"},
	   { "Accept-Encoding", "gzip,deflate,sdch"},
	   { "Accept-Language", "en-US,en;q=0.8,zh-CN;q=0.6,zh;q=0.4"},
	   { "Connection", "keep-alive"},
	   { "Content-Length", "0"},
	   { "Host", "" },
	   { "User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/31.0.1650.63 Safari/537.36" }};
	
     for(int i = 0; i < MAX_HEADERS; i++) {
	  this->set_header(headers[i][0], headers[i][1]);
     }
}
void Request::set_http_proto(int http_major, int http_minor) {
     this->raw_data->http_major = http_major;
     this->raw_data->http_minor = http_minor;
}

void Request::set_request_method(int method) {
     this->raw_data->method = method;
}
int Request::set_header(const char* key, const char *value) {
     if(key != NULL && value != NULL && strlen(key) > 0) {
	  int exist = 0;
	  for(int i = 0; i < raw_data->num_headers; i++) {
	       if(strcasecmp(raw_data->headers[i][0], key) == 0) {
		    memset(raw_data->headers[i][1], 
			   0, MAX_ELEMENT_SIZE);
		    strcpy(raw_data->headers[i][1], value);
		    exist = 1;
	       }
	  }
	  if(!exist) {
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
int Request::set_body(const char* body) {
     if(body != NULL && strlen(body) > 0) {
	  int body_size = strlen(body);
	  memset(raw_data->body, 0, MAX_ELEMENT_SIZE);
	  strncpy(raw_data->body, body, body_size);
	  return 0;
     }
     return 1;
}
int Request::to_str() {
     if(this->raw_data != NULL) {
	  if(raw_data->raw == NULL) {
	       raw_data->raw = (char*)malloc(1024*10);
	  }

	  memset(raw_data->raw, 0, 1024*10);

	  char method_str[10] = {0};
	  if(raw_data->method == 1) {
	       sprintf(method_str, "%s", "GET");
	  }else {
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
		
	  //body
	  strncpy(raw_data->raw+strlen(raw_data->raw), 
		  raw_data->body, 
		  strlen(raw_data->body));
			
	  raw_data->raw[strlen(raw_data->raw)] = '\r';
	  raw_data->raw[strlen(raw_data->raw)] = '\n';
	  return 0;
     }
     return 1;
}
char* Request::get_raw() {
     return this->raw_data->raw;
}



// protocol type
int get_request_protocol(Request* request) 
{
     if(request != NULL) {
	  char *body = request->raw_data->body;
	  if(strlen(body) > 0) {
	       if(strncasecmp(body, PROTOBUF, strlen(PROTOBUF)) == 0) {
		    return PROTOBUF_PROTOCOL;	
	       }

	  }
	  return NORMAL_PROTOCOL;
     }else {
	  return ERROR_PROTOCOL;
     }

}


} // namespace sails

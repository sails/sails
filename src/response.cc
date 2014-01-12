#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

namespace sails {

Response::Response() {
     this->raw_data = (struct message *)malloc(sizeof(struct message));
     message_init(this->raw_data);
     this->raw_data->type = HTTP_RESPONSE;
     this->set_default_header();
}

Response::~Response() {
     if(this->raw_data != NULL) {
	  delete_message(this->raw_data);
     }
}

void Response::set_http_proto(int http_major, int http_minor) {
     this->raw_data->http_major = http_major;
     this->raw_data->http_minor = http_minor;
}

void Response::set_response_status(int response_status) {
     this->raw_data->status_code = response_status;
}

int Response::set_header(const char *key, const char *value) {
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

int Response::set_body(const char *body) {
     if(body != NULL && strlen(body) > 0) {
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

int Response::to_str() {
     if(this->raw_data != NULL) {
	  if(raw_data->raw == NULL) {
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

char* Response::get_raw() {
     return this->raw_data->raw;
}

void Response::set_default_header() {
     this->set_http_proto(1, 1);
     this->set_response_status(200);
     time_t timep;
     time (&timep);

     char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE] = 
	  {{ "Location", "localhost/cust"},
	   { "Content-Type", "text/html;charset=UTF-8"},
	   { "Date", ""},
	   { "Expires", "" },
	   { "Connection", "keep-alive"},
	   { "Cache-Control", "public, max-age=0" },
	   { "Server", "sails server" },
	   { "Content-Length", "0" }};
	
     for(int i = 0; i < MAX_HEADERS; i++) {
	  this->set_header(headers[i][0], headers[i][1]);
     }
     // local time
     char *time_str = ctime(&timep);
     time_str[strlen(time_str)-1] = 0;// delete '\n'
     this->set_header("Date", time_str);
     this->set_header("Expires", time_str);
}

} // namespace sails












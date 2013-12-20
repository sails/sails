#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace sails {

Response::Response() {
	this->raw_data = (struct message *)malloc(sizeof(struct message));
	memset(raw_data, 0, sizeof(struct message));
	raw_data->raw = NULL;
}

Response::~Response() {
	if(this->raw_data != NULL) {
		free(this->raw_data);
		this->raw_data = NULL;
	}
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
			strcpy(raw_data->headers[raw_data->num_headers][0],
				key);
			strcpy(raw_data->headers[raw_data->num_headers][1],
				value);
						
		}
	}
	return 1;
}

int Response::set_body(const char *body) {
	if(body != NULL && strlen(body) > 0) {
		int body_size = strlen(body);
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

} // namespace sails












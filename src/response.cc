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

char* Response::to_str() {
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

			strncpy(raw_data->raw+strlen(raw_data->raw), 
				raw_data->headers[i][1], 
				strlen(raw_data->headers[i][1]));

		}

	}
	return NULL;
}

} // namespace sails












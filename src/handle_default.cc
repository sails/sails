#include "handle_default.h"
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

namespace sails {

void HandleDefault::do_handle(sails::Request *request, 
			      sails::Response *response, 
			      HandleChain<sails::Request *, sails::Response *> *chain)
{
	printf("default handle process\n");
	printf("connfd:%d\n", response->connfd);

	if(request != 0) {

		this->set_default_header(response);

		// open file and send content to client

		std::string request_path = strcmp(
			request->raw_data->request_path, "/")>0
			?request->raw_data->request_path:"index.html\0";
		std::cout << "request_path:" << request_path << std::endl;
		std::string content;
		std::string line;
		std::ifstream file(request_path);
		if (file.is_open())
		{
			while ( getline (file,line) )
			{
				content+=line;
			}
			content+="\r\n";
			
			response->set_body(content.c_str());

			file.close();
		}
		response->to_str();
		int n = write(response->connfd, response->raw_data->raw, 
			      strlen(response->raw_data->raw));
		if(request->raw_data->should_keep_alive != 1) {
			close(response->connfd);
		}else {
			
		}
	}

}

void HandleDefault::set_default_header(Response* response) {
	response->raw_data = (struct message*)malloc(
		sizeof(struct message));
	response->raw_data->name = "sails server";
	response->raw_data->type = HTTP_RESPONSE;
	response->raw_data->http_major = 1;
	response->raw_data->http_minor = 1;
	response->raw_data->status_code = 200;


	char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE] = 
		{{ "Location", "localhost/cust"},
		 { "Content-Type", "text/html;charset=UTF-8"},
		 { "Date", "un, 26 Apr 2013 11:11:49 GMT"},
		 { "Expires", "Tue, 26 May 2013 11:11:49 GMT" },
		 { "Connection", "keep-alive"},
		 { "Cache-Control", "public, max-age=2592000" },
		 { "Server", "sails server" },
		 { "Content-Length", "0" }};
	response->raw_data->num_headers = 8;
	
	for(int i = 0; i < MAX_HEADERS; i++) {
		strncpy(response->raw_data->headers[i][0], headers[i][0],MAX_ELEMENT_SIZE);
		strncpy(response->raw_data->headers[i][1], headers[i][1],MAX_ELEMENT_SIZE);
	}


}

} // namespace sails 

















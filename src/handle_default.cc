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
	if(request != 0) {
		// open file and send content to client
		std::string request_path = strcmp(
			request->raw_data->request_path, "/")>0
			?request->raw_data->request_path:"/index.html\0";
		std::cout << "request_path:" << request_path << std::endl;
		std::string content;
		std::string line;
		std::ifstream file(request_path.substr(1, request_path.length()-1));
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
	}
}

} // namespace sails 

















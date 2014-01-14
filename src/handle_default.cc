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
	  int proto_type = get_request_protocol(request);
	  printf("proto_type:%d\n", proto_type);
	  if(proto_type != NORMAL_PROTOCOL) {
	       chain->do_handle(request, response);
	       return;
	  }
		
	  // open file and send content to client
	  std::string request_path = this->get_request_path(request);
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
	  std::cout << "content:" << content << std::endl;

	  return;
     }
}


std::string HandleDefault::get_request_path(sails::Request *request) {
     std::string request_path = std::string(request->raw_data->request_path);
     std::string real_path = "../static"+request_path;
     if (real_path == "../static/") {
	  real_path+="index.html";
     }
     std::cout << "real_path:" << real_path << std::endl;
     return real_path;
}

} // namespace sails 



















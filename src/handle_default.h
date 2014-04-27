#ifndef _HANDLE_DEFAULT_H_
#define _HANDLE_DEFAULT_H_

#include <iostream>
#include "handle.h"
#include <common/net/http.h>

namespace sails {


class HandleDefault : public Handle<common::net::HttpRequest*, common::net::HttpResponse*>
{
public:
    void do_handle(common::net::HttpRequest* request, 
		   common::net::HttpResponse* response, 
		   HandleChain<common::net::HttpRequest*, common::net::HttpResponse*> *chain);

    std::string get_request_path(common::net::HttpRequest *request);
};


} //namespace sails

#endif /* _HANDLE_DEFAULT_H_ */
















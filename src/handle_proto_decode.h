#ifndef _HANDLE_PROTO_DECODE_H_
#define _HANDLE_PROTO_DECODE_H_

#include "handle.h"
#include <common/net/http.h>

namespace sails {

class HandleProtoDecode : public Handle<common::net::HttpRequest*, common::net::HttpResponse*>
{
public:
void do_handle(common::net::HttpRequest* request, 
		   common::net::HttpResponse* response, 
		   HandleChain<common::net::HttpRequest*, common::net::HttpResponse*> *chain);

private:
void decode_protobuf(common::net::HttpRequest* request, common::net::HttpResponse* response, 
			 HandleChain<common::net::HttpRequest*, common::net::HttpResponse*> *chain);
};

}



#endif /* _HANDLE_PROTO_DECODE_H_ */






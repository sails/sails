#ifndef _HANDLE_PROTO_DECODE_H_
#define _HANDLE_PROTO_DECODE_H_

#include "handle.h"
#include "request.h"
#include "response.h"

namespace sails {

class HandleProtoDecode : public Handle<Request*, Response*>
{
public:
	void do_handle(Request* request, Response* response, 
		       HandleChain<Request*, Response*> *chain);

private:
	void decode_protobuf(Request* request, Response* response, 
		       HandleChain<Request*, Response*> *chain);
};

}



#endif /* _HANDLE_PROTO_DECODE_H_ */


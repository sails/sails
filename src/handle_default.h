#ifndef _HANDLE_DEFAULT_H_
#define _HANDLE_DEFAULT_H_

#include "handle.h"
#include "request.h"
#include "response.h"

namespace sails {


class HandleDefault : public Handle<Request*, Response*>
{
public:
	void do_handle(Request* request, Response* response, 
		       HandleChain<Request*, Response*> *chain);
};


} //namespace sails

#endif /* _HANDLE_DEFAULT_H_ */
















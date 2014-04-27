#ifndef _FILTER_DEFAULT_H_
#define _FILTER_DEFAULT_H_

#include "filter.h"
#include <common/net/http.h>

namespace sails {

class FilterDefault : public Filter<common::net::HttpRequest*, common::net::HttpResponse*>
{
	void do_filter(common::net::HttpRequest* request, 
		       common::net::HttpResponse* response, 
		       FilterChain<common::net::HttpRequest*, common::net::HttpResponse*> *chain);
};

} // namespace sails

#endif /* _FILTER_DEFAULT_H_ */
















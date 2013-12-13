#ifndef _FILTER_DEFAULT_H_
#define _FILTER_DEFAULT_H_

#include "filter.h"
#include "request.h"
#include "response.h"

namespace sails {

class FilterDefault : public Filter<Request*, Response*>
{
	void do_filter(Request* request, Response* response, FilterChain<Request*, Response*> *chain);
};

} // namespace sails

#endif /* _FILTER_DEFAULT_H_ */
















#include "filter_default.h"

namespace sails {

void FilterDefault::do_filter(common::net::HttpRequest *request, 
			      common::net::HttpResponse *response, 
			      sails::FilterChain<common::net::HttpRequest*, common::net::HttpResponse*> 
			      *chain) {
//	printf("filter request url:%s\n", request->raw_data->request_url);
	
}

} // namespace sails

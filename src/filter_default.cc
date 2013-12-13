#include "filter_default.h"

namespace sails {

void FilterDefault::do_filter(sails::Request *request, sails::Response *response, sails::FilterChain<Request*, Response*> *chain) {
	printf("filter request url:%s\n", request->raw_data->request_url);
}

} // namespace sails

#include "handle_proto_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


namespace sails {



void HandleProtoDecode::do_handle(sails::Request *request, 
				sails::Response *response, 
				HandleChain<sails::Request *, sails::Response *> *chain)
{
	if(request != 0) {
		int proto_type = get_request_protocol(request);
		if(proto_type == PROTOBUF_PROTOCOL) {
			printf("protobuf protocol decode handle\n");
			decode_protobuf(request, response, chain);
		}
	}
	chain->do_handle(request, response);
	return;
}

void HandleProtoDecode::decode_protobuf(sails::Request *request, sails::Response *response, HandleChain<sails::Request *, sails::Response *> *chain)
{
	
}


} // namespace sails


















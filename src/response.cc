#include "response.h"
#include <stdlib.h>

namespace sails {

Response::Response() {
	this->raw_data = (struct message *)malloc(sizeof(struct message));
}

} // namespace sails

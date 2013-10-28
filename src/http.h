#ifndef _HTTP_H_
#define _HTTP_H_

#include <http_parser.h>

namespace sails {
	
class HttpConnection {		
public:
	http_parser *parser_http(char *buf);
};
	

}

#endif /* _HTTP_H_ */

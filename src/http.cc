#include "http.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

namespace sails {
	
	http_parser *parser_http(char *buf) {
		static http_parser_settings settings_null;
		settings_null.on_message_begin = 0;
		settings_null.on_header_field = 0;
		settings_null.on_header_value = 0;
		settings_null.on_url = 0;
		settings_null.on_body = 0;
		settings_null.on_headers_complete = 0;
		settings_null.on_message_complete = 0;

		http_parser *parser = (http_parser *)malloc(sizeof(http_parser));
		http_parser_init(parser, HTTP_REQUEST);
		size_t nparsed = http_parser_execute(parser, &settings_null, buf, strlen(buf));
		return parser;
	}
}

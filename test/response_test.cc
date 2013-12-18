#include <gtest/gtest.h>
#include <stdio.h>
#include "response.h"

using namespace sails;


void set_default_header(Response* response) {
	response->raw_data = (struct message*)malloc(
		sizeof(struct message));
	response->raw_data->name = "sails server";
	response->raw_data->type = HTTP_RESPONSE;
	response->raw_data->http_major = 1;
	response->raw_data->http_minor = 1;
	response->raw_data->status_code = 200;

	char headers[MAX_HEADERS][2][MAX_ELEMENT_SIZE] = 
		{{"Location", "localhost/cust"},
		 {"Content-Type", "text/html;charset=UTF-8"},
		 {"Date", "Thu, 26 Apr 2013 11:11:49 GMT"},
		 { "Expires", "Tue, 26 May 2013 11:11:49 GMT" },
		 { "Cache-Control", "public, max-age=2592000" },
		 { "Server", "gws" },
		 { "Content-Length", "219" }};
	response->raw_data->num_headers = 7;
	for(int i = 0; i < MAX_HEADERS; i++) {
		strncpy(response->raw_data->headers[i][0], headers[i][0],MAX_ELEMENT_SIZE);
		strncpy(response->raw_data->headers[i][1], headers[i][1],MAX_ELEMENT_SIZE);
	}


}


TEST(response_test, to_str_test)
{
	Response response;
	set_default_header(&response);
	strncpy(response.raw_data->body, "test html content", 17);
	response.to_str();
	printf("%s\n", response.raw_data->raw);
}

int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

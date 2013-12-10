#include <gtest/gtest.h>
#include "filter.h"

using namespace sails;

typedef struct request {
	int i = 0;
} Request;

typedef struct response {
	int i = 0;
}Response;

class TestFilter: public Filter<Request, Response>
{
public:
	void do_filter(Request req, Response resp, FilterChain<Request, Response> *chain) {
		printf("request:%d response:%d \n", req.i, resp.i);
		chain->do_filter(req, resp);
	}
};

TEST(filter_test, do_filter_test)
{
	FilterChain<Request, Response> chain;
	Request request;
	Response response;
	TestFilter *test_filter = new TestFilter();
	chain.add_filter(test_filter);
	chain.do_filter(request, response);
}

int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

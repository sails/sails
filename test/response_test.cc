#include <gtest/gtest.h>
#include <stdio.h>
#include <common/net/http.h>

using namespace sails::common::net;

TEST(response_test, to_str_test)
{
	HttpResponse response;
	response.to_str();
	printf("%s\n", response.get_raw());
}

int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

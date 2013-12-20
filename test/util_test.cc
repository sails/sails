#include <gtest/gtest.h>
#include <stdio.h>
#include "util.h"

using namespace sails; 

TEST(util_test, strlncat) {
	char buf[2048];
	memset(buf, 0, 2048);
	const char *src = "max-age=0\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\nUser-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/31.0.1650.63 Sa";
        strlncat(buf,2048, src, 9);
	printf("buf:%s", buf);
}


int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}











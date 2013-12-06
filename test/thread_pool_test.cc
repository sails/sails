#include <gtest/gtest.h>
#include "thread_pool.h"

using namespace sails;

TEST(thread_pool, new_thread_pool)
{
	ThreadPool *pool = new ThreadPool(2, 10);
	EXPECT_EQ(pool->get_thread_num(), 2);
	delete pool;
	pool = NULL;
}

int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}












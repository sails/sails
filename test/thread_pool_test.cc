#include <gtest/gtest.h>
#include <common/base/thread_pool.h>

using namespace sails;

TEST(thread_pool, new_thread_pool)
{
        common::ThreadPool *pool = new common::ThreadPool(10);
	EXPECT_EQ(pool->get_thread_num(), 2);
	delete pool;
	pool = NULL;
}

void task_test(void *arg) {
	int num = *(int *)arg;
	printf("task test end\n");
}

TEST(thread_pool, add_task)
{
	common::ThreadPool *pool = new common::ThreadPool(100);
	common::ThreadPoolTask task;
	task.fun = task_test;
	int num = 100;
	task.argument = &num;
	for(int i = 0; i < 5; i++){
		pool->add_task(task);
	}
	
	std::this_thread::sleep_for(std::chrono::seconds(1));//wait for task all executed

	delete pool;
	pool = NULL;
}

int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}












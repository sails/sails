#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace sails {


class ThreadPoolTask {
public:
	void (*fun)(void *);
public:
	void *argument;
};

class ThreadPool {
public:
	// set num of threads equal to processor
	ThreadPool(int queue_size);

	// set num of threads for customize
	ThreadPool(int thread_num, int queue_size);

	~ThreadPool();
	
	int add_task(ThreadPoolTask task);
	
private:
	int thread_num;
	std::mutex lock; // for add task and threads to get task crital section
	std::condition_variable notify; // wake up wait thread
	int shutdown;
	std::queue<ThreadPoolTask> task_queue;
	std::vector<std::thread> thread_list;
	// real function passed to thread when created
	static void *threadpool_thread(void *threadpool);
};

typedef enum {
	immediate_shutdown = 1,
	graceful_shutdown = 2
} threadpool_shutdown_t;
	
}



#endif /* _THREAD_POOL_H_ */











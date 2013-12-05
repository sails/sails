#include "thread_pool.h"
#include <sys/sysinfo.h>

namespace sails {


ThreadPool::ThreadPool(int queue_size) {
	int processor_num = get_nprocs();
	if(processor_num < 0) {
		printf("error get num of porcessor .\n");
		exit(0);
	}else {
		ThreadPool(processor_num, queue_size);
	}
}

ThreadPool::ThreadPool(int thread_num, int queue_size) {
	this->thread_num = thread_num;
	this->shutdown = 0;
	
	// start worker threads
	for(int i = 0; i < thread_num; i++) {
		thread_list.push_back(std::thread(ThreadPool::threadpool_thread,this));
		this->thread_num++;
	}
}

ThreadPool::~ThreadPool() {

}

int ThreadPool::add_task(sails::ThreadPoolTask task) {
	this->lock.lock();
	this->task_queue.push(task);
	this->notify.notify_one(); // because mutex, at the same time only a thread wait for condition variable
	this->lock.unlock();
}

void* ThreadPool::threadpool_thread(void *threadpool) {
	ThreadPool *pool = (ThreadPool *)threadpool;
	ThreadPoolTask task;
	std::unique_lock<std::mutex> locker(pool->lock);
	for(;;) {
		// lock must be taken to wait on conditional varible
		pool->lock.lock();

		// wait on condition variable, check for spurious wakeups.
		// if task_queue is not empty, even thougth spurious wakeups
		// we also would execute.
		// when returning from wait, we own the lock.
		while((pool->task_queue.size() == 0) && (!pool->shutdown)) {
			pool->notify.wait(locker); // wait would release lock, and when return it can get lock
		}
		if((pool->shutdown == immediate_shutdown) ||
		   ((pool->shutdown == graceful_shutdown) &&
		    (pool->task_queue.size() == 0))) {
			   break;
		}
		
		// grab our task
		task = pool->task_queue.front();
		pool->task_queue.pop();
		
		// unlock
		pool->lock.unlock();
		
		// work
		(*(task.fun))(task.argument);
	}
	pool->lock.unlock();
	return(NULL);
}
	
}

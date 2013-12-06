#include <stdio.h>
#include "thread_pool.h"
#include <sys/sysinfo.h>

namespace sails {


ThreadPool::ThreadPool(int queue_size) {
	int processor_num = get_nprocs();
	if(processor_num < 0) {
		printf("error get num of porcessor .\n");
		exit(0);
	}else {
		this->thread_num = processor_num;
		this->task_queue_available_size = queue_size;
		this->shutdown = 0;
		this->start_run();
	}
}

ThreadPool::ThreadPool(int thread_num, int queue_size) {
	this->thread_num = thread_num;
	this->task_queue_available_size = queue_size;
	this->shutdown = 0;
	this->start_run();
}

void ThreadPool::start_run()
{
	// start worker threads
	for(int i = 0; i < thread_num; i++) {
		thread_list.push_back(std::thread(ThreadPool::threadpool_thread,this));
	}
}

ThreadPool::~ThreadPool() {
	this->shutdown = immediate_shutdown;
	this->notify.notify_all();
	for(int i = 0; i < this->thread_list.size(); i++) {
		thread_list[i].join();
	}
}

int ThreadPool::get_thread_num()
{
	return this->thread_num;
}

int ThreadPool::get_task_queue_size()
{
	return this->task_queue.size();
}

int ThreadPool::add_task(sails::ThreadPoolTask task) {
	int result = 0;
	this->lock.lock();
	if(this->task_queue_available_size > 0) {
		this->task_queue.push(task);
		this->task_queue_available_size--;
		this->notify.notify_one(); // because mutex, at the same time only a thread wait for condition variable
		result = 0;
	}else {
		result = -1;
	}
	this->lock.unlock();
	return result;
}

void* ThreadPool::threadpool_thread(void *threadpool) {
	ThreadPool *pool = (ThreadPool *)threadpool;
	ThreadPoolTask task;

	for(;;) {
		// lock must be taken to wait on conditional varible
		std::unique_lock<std::mutex> locker(pool->lock);

		// wait on condition variable, check for spurious wakeups.
		// when returning from wait, we own the lock.

		while(!pool->shutdown) {
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
		pool->task_queue_available_size++;

		// unlock
		pool->lock.unlock();
		
		// work
		(*(task.fun))(task.argument);
	}
	pool->lock.unlock();
	return(NULL);
}
	
}

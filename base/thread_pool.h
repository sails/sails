// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: thread_pool.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:31:30



#ifndef SAILS_BASE_THREAD_POOL_H_
#define  SAILS_BASE_THREAD_POOL_H_

#include <vector>
#include <queue>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace sails {
namespace base {

class ThreadPoolTask {
 public:
  void (*fun)(void *);
 public:
  void *argument;
};

class ThreadPool {
 public:
  // set num of threads equal to processor
  explicit ThreadPool(int queue_size);

  // set num of threads for customize
  ThreadPool(int thread_num, int queue_size);

  ~ThreadPool();

  int get_thread_num();

  int get_task_queue_size();

  int add_task(ThreadPoolTask task);

 private:
  void start_run();

  int thread_num;
  int task_queue_available_size;
  std::mutex lock;  // for add task and threads to get task crital section
  std::condition_variable notify;  // wake up wait thread
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

}  // namespace base
}  // namespace sails



#endif  // SAILS_BASE_THREAD_POOL_H_











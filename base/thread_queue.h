// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: thread_queue.h
// Description: 线程安全的队列
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:57:22



#ifndef SAILS_BASE_THREAD_QUEUE_H_
#define SAILS_BASE_THREAD_QUEUE_H_

#include <assert.h>
#include <deque>
#include <thread>  // NOLINT
#include <condition_variable>  // NOLINT
#include <mutex>  // NOLINT

namespace sails {
namespace base {

template<typename T, typename D = std::deque<T> >
class ThreadQueue {
 public:
  ThreadQueue() {
    _size = 0;
    _maxSize = 100000;
    isTerminate = false;
  }

 public:
  typedef D queue_type;
  // 从头部获取数据, 没有数据则等待.
  // @param millsecond   阻塞等待时间(ms)
  //                     0 表示不阻塞  -1 永久等待
  bool pop_front(T& t, int millsecond = 0);  // NOLINT'

  // 通知等待在队列上面的线程都醒过来
  void notifyT();

  // 放数据到队列后端.如果失败,调用端要决定如何处理
  bool push_back(const T& t);

  // 放数据到队列后端,返回push了多少个元素
  int push_back(const queue_type &qt);

  // 放数据到队列前端.
  void push_front(const T& t);

  // 放数据到队列前端.
  void push_front(const queue_type &qt);

  // 等到有数据才交换.
  // millsecond  阻塞等待时间(ms)
  //             0 表示不阻塞 -1 为则永久等待
  // 有数据返回true, 无数据返回false
  bool swap(queue_type &q, int millisecond = 0);

  // 队列大小.
  size_t size();

  // 设置队列最大大小
  void setMaxSize(size_t maxSize);

  size_t MaxSize() {
    return _maxSize;
  }

  // 清空队列
  void clear();

  // 是否数据为空.
  bool empty();

  // 等待
  void wait(std::unique_lock<std::mutex>& locker);  // NOLINT'

  bool timedWait(int millisecond, std::unique_lock<std::mutex>& locker);  // NOLINT

 private:
  // 队列
  queue_type          _queue;

  size_t              _maxSize;
  // 队列大小
  size_t              _size;
  // 互斥量
  std::mutex          queue_mutex;

  std::condition_variable     notify;
  bool                isTerminate;
};



template<typename T, typename D>
void ThreadQueue<T, D>::wait(std::unique_lock<std::mutex>& locker) {  // NOLINT'
  while ((_queue.size() == 0)  && (!isTerminate)) {
    notify.wait(locker);
  }
}

template<typename T, typename D>
bool ThreadQueue<T, D>::timedWait(int millisecond,
                                  std::unique_lock<std::mutex>& locker) {  // NOLINT
  notify.wait_for(locker, std::chrono::milliseconds(millisecond));
  if (_queue.size() > 0) {
    return true;
  }
  return false;
}


template<typename T, typename D>
bool ThreadQueue<T, D>::pop_front(T& t, int millisecond) { // NOLINT'
  std::unique_lock<std::mutex> locker(queue_mutex);
  if (_queue.empty()) {
    if (millisecond == 0) {
      return false;
    }
    if (millisecond > 3600*24) {
      wait(locker);
    } else {
      timedWait(millisecond, locker);
    }
  }

  if (_queue.empty()) {
    return false;
  }

  t = _queue.front();
  _queue.pop_front();
  assert(_size > 0);
  --_size;


  return true;
}

template<typename T, typename D>
void ThreadQueue<T, D>::notifyT() {
  std::unique_lock<std::mutex> locker(queue_mutex);
  this->notify.notify_all();
}

template<typename T, typename D>
bool ThreadQueue<T, D>::push_back(const T& t) {
  if (_size >= _maxSize) {
    return false;
  }
  std::unique_lock<std::mutex> locker(queue_mutex);
  this->notify.notify_one();

  _queue.push_back(t);
  ++_size;
  return true;
}

template<typename T, typename D>
int ThreadQueue<T, D>::push_back(const queue_type &qt) {
  std::unique_lock<std::mutex> locker(queue_mutex);
  typename queue_type::const_iterator it = qt.begin();
  typename queue_type::const_iterator itEnd = qt.end();
  int push_num = 0;
  while (it != itEnd) {
    if (_size >= _maxSize) {
      return push_num;
    }
    _queue.push_back(*it);
    push_num++;
    ++it;
    ++_size;
    this->notify.notify_one();
  }
  return push_num;
}

template<typename T, typename D>
void ThreadQueue<T, D>::push_front(const T& t) {
  std::unique_lock<std::mutex> locker(queue_mutex);

  this->notify.notify_one();

  _queue.push_front(t);

  ++_size;
}

template<typename T, typename D>
void ThreadQueue<T, D>::push_front(const queue_type &qt) {
  std::unique_lock<std::mutex> locker(queue_mutex);

  typename queue_type::const_iterator it = qt.begin();
  typename queue_type::const_iterator itEnd = qt.end();
  while (it != itEnd) {
    _queue.push_front(*it);
    ++it;
    ++_size;

    this->notify.notify_one();
  }
}

template<typename T, typename D>
bool ThreadQueue<T, D>::swap(queue_type &q, int millisecond) {  // NOLINT'
  std::unique_lock<std::mutex> locker(queue_mutex);

  if (_queue.empty()) {
    if (millisecond == 0) {
      return false;
    }
    if (millisecond == (size_t)-1) {
      wait(locker);
    } else {
      //超时了
      if (!timedWait(millisecond, locker)) {
        return false;
      }
    }
  }

  if (_queue.empty()) {
    return false;
  }

  q.swap(_queue);
  _size = q.size();

  return true;
}

template<typename T, typename D> size_t ThreadQueue<T, D>::size() {
  std::unique_lock<std::mutex> locker(queue_mutex);
  // return _queue.size();
  return _size;
}


template<typename T, typename D>
void ThreadQueue<T, D>::setMaxSize(size_t maxSize) {
  _maxSize = maxSize;
}

template<typename T, typename D>
void ThreadQueue<T, D>::clear() {
  std::unique_lock<std::mutex> locker(queue_mutex);
  _queue.clear();
  _size = 0;
}

template<typename T, typename D>
bool ThreadQueue<T, D>::empty() {
  std::unique_lock<std::mutex> locker(queue_mutex);
  return _queue.empty();
}




}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_THREAD_QUEUE_H_

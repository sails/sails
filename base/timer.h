// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: timer.h
// Description: 定时器
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:43:32



#ifndef BASE_TIMER_H_
#define BASE_TIMER_H_

#include <thread>  // NOLINT
#include "sails/base/event_loop.h"

namespace sails {
namespace base {

typedef void (*ExpiryAction)(void *data);

class Timer {
 public:
  // set tick to zero, the timer expires just once
  explicit Timer(EventLoop *ev_loop,  int tick = 1);
  explicit Timer(int tick);
  ~Timer();
  // set when to zero disarms the timer
  bool init(ExpiryAction action, void *data, int when);
  bool disarms();

 public:
  static void read_timerfd_data(base::event*, int revents, void* owner);
  void pertick_processing();

 private:
  int tick;
  int timerfd;
  #ifdef __APPLE__
  // kqueue的timerfd可以随意指定，这里用static是为了防止多个timer时重复
  static int next_timerfd;
  #endif

  EventLoop *ev_loop;
  int self_evloop;
  ExpiryAction action;
  void *data;
  #ifdef __linux__
  char temp_data[50];
  #endif
  std::thread* iothread;  // 当没有event_loop时需要
};


}  // namespace base
}  // namespace sails


#endif  // BASE_TIMER_H_





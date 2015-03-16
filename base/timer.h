// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: timer.h
// Description: 定时器
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:43:32



#ifndef SAILS_BASE_TIMER_H_
#define SAILS_BASE_TIMER_H_
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

  EventLoop *ev_loop;
  int self_evloop;
  ExpiryAction action;
  void *data;
  char temp_data[50];
};


}  // namespace base
}  // namespace sails


#endif  // SAILS_BASE_TIMER_H_

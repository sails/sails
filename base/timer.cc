// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: timer.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:47:09



#include "sails/base/timer.h"
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "sails/base/event_loop.h"

namespace sails {
namespace base {

Timer::Timer(EventLoop *ev_loop, int tick) {
  this->tick = tick;
  this->ev_loop = ev_loop;
  this->data = NULL;
  self_evloop = 0;
}

Timer::Timer(int tick) {
  this->tick = tick;
  this->data = NULL;
  self_evloop = 0;
}

bool Timer::init(ExpiryAction action, void *data, int when = 1) {
  timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);

  new_value = (struct itimerspec*)malloc(sizeof(struct itimerspec));
  new_value->it_interval.tv_sec = tick;
  new_value->it_interval.tv_nsec = 0;
  new_value->it_value.tv_sec = when;
  new_value->it_value.tv_nsec = 0;
  timerfd_settime(timerfd, 0, new_value, NULL);

  sails::base::event ev;
  emptyEvent(&ev);
  ev.fd = timerfd;
  ev.events = sails::base::EventLoop::Event_READ;
  ev.cb = sails::base::Timer::read_timerfd_data;
  ev.data.ptr = this;
  ev.stop_cb = NULL;
  ev.next = NULL;

  if (ev_loop == NULL) {
    ev_loop = new EventLoop(this);
    ev_loop->init();
    self_evloop = 1;
  }

  if (!ev_loop->event_ctl(base::EventLoop::EVENT_CTL_ADD, &ev)) {
    printf("timerfd:%d\n", timerfd);
    close(timerfd);
    timerfd = 0;
    return false;
  }
  this->action = action;
  this->data = data;
  return true;
}

bool Timer::disarms()  {
  if (timerfd > 0) {
    close(timerfd);
    timerfd = 0;
  }
  return true;
}

void Timer::read_timerfd_data(base::event* ev, int revents, void* owner) {
  if (revents != EventLoop::Event_READ || owner == NULL) {
    return;
  }
  Timer *timer = reinterpret_cast<Timer*>(ev->data.ptr);
  if (timer != NULL) {
    memset(timer->temp_data, '\0', 50);
    int n = read(ev->fd, timer->temp_data, sizeof(uint64_t));
    if (n != sizeof(uint64_t)) {
      return;
    }

    timer->pertick_processing();
  }
}


Timer::~Timer() {
  if (timerfd > 0) {
    close(timerfd);
    timerfd = 0;
  }
  if (self_evloop > 0) {
    ev_loop->stop_loop();
    delete ev_loop;
  }
  ev_loop = NULL;
  action = NULL;
  if (new_value != NULL) {
    free(new_value);
  }
}

void Timer::pertick_processing() {
  (*action)(this->data);
}



}  // namespace base
}  // namespace sails

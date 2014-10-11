// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: event_loop.h
// Description: 在关注的fd上绑定事件和回调,当事件发生时,调用相应的回调函数
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-10 15:59:50



#ifndef SAILS_BASE_EVENT_LOOP_H_
#define SAILS_BASE_EVENT_LOOP_H_
#include <sys/epoll.h>
#include "sails/base/uncopyable.h"

namespace sails {
namespace base {

typedef void (*event_cb)(struct event*, int revents, void *owner);
typedef void (*STOPEVENT_CB)(struct event*, void *owner);

typedef union Event_Data {
  void *ptr;
  uint32_t u32;
  uint64_t u64;
} Event_Data;

struct event {
  int fd;
  int events;
  event_cb cb;
  Event_Data data;
  struct event* next;
  STOPEVENT_CB stop_cb;
};

void emptyEvent(struct event* ev);

struct ANFD {
  int isused;
  int fd;
  int events;
  struct event* next;
};


class EventLoop : private Uncopyable{
 public:
  static const int INIT_EVENTS = 1000;
  enum OperatorType {
    EVENT_CTL_ADD = 1,
    EVENT_CTL_DEL = 2,
    EVENT_CTL_MOD = 3
  };
  enum Events {
    Event_READ = 1,
    Event_WRITE = 2
  };


  explicit EventLoop(void *owner);
  ~EventLoop();

  void init();
  bool event_ctl(OperatorType op, const struct event*);
  bool event_stop(int fd);
  void start_loop();
  void stop_loop();
  void delete_all_event();

 private:
  void *owner;
  bool add_event(const struct event*, bool ctl_epoll = true);
  bool delete_event(const struct event*, bool ctl_epoll = true);
  bool mod_event(const struct event*, bool ctl_epoll = true);
  void process_event(int fd, int events);
  bool array_needsize(int need_cnt);
  void init_events(int start, int count);
  int epollfd;
  struct epoll_event *events;
  struct ANFD *anfds;
  int max_events;

  bool stop;
  int shutdownfd = 0;
};

}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_EVENT_LOOP_H_

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
#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif
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
  int64_t edata;  // event可能需要，如kqueue中的timer要传一个时间进kqueue中
  event_cb cb;  // 事件发生时回调
  Event_Data data;  // 用于回调时传递数据
  struct event* next;  // private
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
    EVENT_CTL_ADD = 1
    , EVENT_CTL_DEL = 2
    , EVENT_CTL_MOD = 3
#ifdef __APPLE__
    , EVENT_CTL_DISABLE = 4
    , EVENT_CTR_ENABLE = 5
#endif
  };
  enum Events {
    Event_READ = 1
    , Event_WRITE = 2
#ifdef __APPLE__
    , Event_TIMER = 4
#endif
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
  bool add_event(const struct event*, bool ctl_poll = true);
  bool delete_event(const struct event*, bool ctl_epoll = true);
  bool mod_event(const struct event*, bool ctl_poll = true);
  void process_event(int fd, int events);
  bool array_needsize(int need_cnt);
  void init_events(int start, int count);

#ifdef __linux__
  int epollfd;
  // 因为epoll中使用的是ET模式，它只通知一次，所以要在这里保证足够的events.
  // 虽然中会在epoll_wait中使用，但是由于要动态增长，
  // 所以不能是start_loop的局部变量
  struct epoll_event* events;
#elif __APPLE__
  int kqfd;
  struct kevent* events;
#endif
  struct ANFD *anfds;
  int max_events;

  bool stop;
  // 用于通知结束wait事件循环
  int shutdownfd = 0;
};

}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_EVENT_LOOP_H_

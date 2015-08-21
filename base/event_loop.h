// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: event_loop.h
// Description: 在关注的fd上绑定事件和回调,当事件发生时,调用相应的回调函数
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-10 15:59:50



#ifndef BASE_EVENT_LOOP_H_
#define BASE_EVENT_LOOP_H_
#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#endif
#include <mutex>  // NOLINT
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

  // 为了提升性能，只修改linux的epoll和kevent，而不修改anfds
  // 这样也可以不用加锁
  bool mod_ev_only(const struct event*);
 private:
  void *owner;
  bool add_event(const struct event*, bool ctl_poll = true);
  bool delete_event(const struct event*, bool ctl_epoll = true);
  bool mod_event(const struct event*, bool ctl_poll = true);
  void process_event(int fd, int events);
  bool array_needsize(int need_cnt);
  void init_events(int start, int count);


  // 最大同时就绪的事件
  int max_events_ready;
#ifdef __linux__
  int epollfd;
  // 因为epoll中使用的是ET模式，它只通知一次，但是如果wait中没有
  // 足够的events长度用于存放它，那么下次wait中它也会被返回，也
  // 就是说通知一次是指它被wait返回之后才不再有效
  // 不过为了效率，最好还是能一次返回所有的就绪事件，所以event在
  // 这里也要动态增长
  // 注意，它不能和anfds一样修改，因为如果和anfds的增长可能是另
  // 一个net线程向这个线程增加fd导致的。 那时它可能会阻塞在当
  // 前线程的epoll_wait中,所以它的在epoll中的events的地址还是上次的
  // 如果realloc在返回的地址不相同，那就出问题，所以它的增加要放到
  // wait之后。
  struct epoll_event* events;
#elif __APPLE__
  int kqfd;
  struct kevent* events;
#endif
  struct ANFD *anfds;
  // 保存fd的列表的长度
  int max_events;

  bool stop;
  // 用于通知结束wait事件循环
  int shutdownfd = 0;

  // 在event_ctl会在其它线程中访问，它会去修改anfds的数据
  std::recursive_mutex eventMutex;
};

}  // namespace base
}  // namespace sails

#endif  // BASE_EVENT_LOOP_H_

// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: event_loop.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-10 16:07:18



#include "sails/base/event_loop.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <mutex>

namespace sails {
namespace base {

// const int EventLoop::INIT_EVENTS = 1000;

// 在event_ctl会在其它线程中访问，它会去修改anfds的数据
std::recursive_mutex eventMutex;

void emptyEvent(struct event* ev) {
  if (ev == NULL) {
    return;
  }
  memset(ev, 0, sizeof(struct event));
  ev->fd = 0;
  ev->events = 0;
  ev->cb = NULL;
  ev->edata = 0;
  ev->data.ptr = 0;
  ev->next = NULL;
}

EventLoop::EventLoop(void* owner) {
#ifdef __linux__
  events = (struct epoll_event*)malloc(sizeof(struct epoll_event)
                                       *INIT_EVENTS);
#elif __APPLE__
  events = (struct kevent*)malloc(sizeof(struct kevent)
                                       *INIT_EVENTS);
#endif
  anfds = (struct ANFD*)malloc(sizeof(struct ANFD)
                               *INIT_EVENTS);
  memset(anfds, 0, 1000*sizeof(struct ANFD));
  max_events = INIT_EVENTS;
  stop = false;
#ifdef __linux__
  shutdownfd = socket(AF_INET, SOCK_STREAM, 0);
#elif __APPLE__
  shutdownfd = open("/tmp/temp_sails_event_poll.txt", O_CREAT | O_RDWR, 0644);
#endif
  assert(shutdownfd > 0);
  this->owner = owner;
}

EventLoop::~EventLoop() {
  if (events != NULL) {
    for (int i = 0; i < max_events; i++) {
      event_stop(i);
    }
    free(events);
    events = NULL;
  }
  if (anfds != NULL) {
    free(anfds);
  }
}

void EventLoop::init() {
#ifdef __linux__
  epollfd = epoll_create(10);
  assert(epollfd > 0);
#elif __APPLE__
  kqfd = kqueue();
  assert(kqfd > 0);
#endif
  for (int i = 0; i < INIT_EVENTS; i++) {
    anfds[i].isused = 0;
    anfds[i].fd = 0;
    anfds[i].events = 0;
    anfds[i].next = NULL;
  }

  // shutdown fd
  
#ifdef __linux__
  struct epoll_event ev;
  memset(&ev, 0, sizeof(ev));
  ev.events = 0; ev.data.fd = 0;
  
  // 一开始设置监听成可读而不是可写事件，这样在调用epoll_ctl之前，就不会触发一次
  // 通过mod触发shutdownfd发出writeable事件的前提必须是已经加入了
  ev.events = EPOLLIN | EPOLLET; 
  ev.data.fd = shutdownfd;
  assert(epoll_ctl(epollfd, EPOLL_CTL_ADD, shutdownfd, &ev) == 0);
#elif __APPLE__
  // kqueue可以设置默认是水平触发，也可以设置边缘触发EV_CLEAR
  // kqueue与epoll不同在于epoll可以通过ctl_mod来触发epoll_wait事件返回
  // 而kqueue没有mod操作，不过它的ADD会把之前的覆盖，并且重新ADD之后，也会触发
  // 由于kqueue创建的是一个可写的fd，所以这里不用提前add
  /*
    struct kevent changes[1];
  EV_SET(&changes[0], shutdownfd, EVFILT_WRITE | EV_CLEAR,
         EV_ADD | EV_DISABLE, 0, 0, NULL);
  assert(kevent(kqfd, changes, 1, NULL, 0, NULL) != -1);
  */
#endif
}

// 把事件加到已有事件列表的最后
bool EventLoop::add_event(const struct event*ev, bool ctl_poll) {
  if (ev->fd >= max_events) {    
    // malloc new events and anfds
    if (!array_needsize(ev->fd+1)) {
      return false;
    }
  }

  struct event *e = (struct event*)malloc(sizeof(struct event));
  emptyEvent(e);
  e->fd = ev->fd;
  e->events = ev->events;
  e->edata = ev->edata;
  e->cb = ev->cb;
  e->data = ev->data;
  e->next = NULL;

  int fd = e->fd;

  // 把事件加到事件列表之后
  int need_add_to_epoll = false;
  
  if (anfds[fd].isused == 1) {
    if ((anfds[fd].events | e->events) != anfds[fd].events) {
      need_add_to_epoll = true;
      struct event *t_e = anfds[fd].next;
      while (t_e->next != NULL) {
        t_e = t_e->next;
      }
      t_e->next = e;
    }
  } else {
    anfds[fd].isused = 1;
    need_add_to_epoll = true;
    anfds[fd].next = e;
  }
  // 修改epoll或kevent监听事件
  if (need_add_to_epoll && ctl_poll) {
#ifdef __linux__
    struct epoll_event epoll_ev;
    memset(&epoll_ev, 0, sizeof(epoll_ev));
    unsigned int events = 0;
    epoll_ev.data.fd = ev->fd;
    if (ev->events & Event_READ) {
      events = events | EPOLLIN | EPOLLET;
    }
    if (ev->events & Event_WRITE) {
      events = events | EPOLLOUT | EPOLLET;
    }
    epoll_ev.events = events;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev->fd, &epoll_ev) == -1) {
      perror("epoll_ctl");
      return false;
    }
#elif __APPLE__
    struct kevent changes[1];
    int16_t filter = 0;
    // 注意,kqueue同一次只能增加一种事件
    if (ev->events & Event_READ) {
      filter = filter | EVFILT_READ;
    }
    if (ev->events & Event_WRITE) {
      filter = filter | EVFILT_WRITE;
    }
    if (ev->events & Event_TIMER) {
      filter = filter | EVFILT_TIMER;
    }
    EV_SET(&changes[0], ev->fd, filter, EV_ADD | EV_CLEAR, 0, ev->edata, NULL);
    if (kevent(kqfd, changes, 1, NULL, 0, NULL) == -1) {
      perror("kevent");
      return false;
    }
#endif
  }

  // 更新fd的已经监听事件flag
  struct event *temp = anfds[fd].next;
  int new_events = 0;
  while (temp != NULL) {
    new_events |= temp->events;
    temp = temp->next;
  }
  anfds[fd].events = new_events;

  return true;
}


void EventLoop::delete_all_event() {
  for (int i = 0; i < max_events; i++) {
    event_stop(i);
  }
}

bool EventLoop::delete_event(const struct event* ev, bool ctl_poll) {
  int fd = ev->fd;
  if (fd < 0 || fd > max_events) {
    return false;
  }
  if (anfds[fd].isused == 1) {
    if ((anfds[fd].events & ev->events) == 0) {
      return true;  // not contain ev.events
    }
    // 从事件列表中删除
    struct event* pre = anfds[fd].next;
    struct event* cur = pre->next;
    while (cur != NULL) {
      int isdelete = 0;
      if ((cur->events & ev->events) > 0) {
        cur->events = cur->events^ev->events;
        if (cur->events == 0) {
          // can delete it from the list
          isdelete = 1;
          cur = cur->next;
          free(pre->next);
          pre->next = cur;
        }
      }
      if (!isdelete) {
        pre = cur;
        cur = cur->next;
      }
    }

    // 更新已经注册的事件flag
    if (anfds[fd].events & ev->events) {
      anfds[fd].events = anfds[fd].events^ev->events;
      if (anfds[fd].events == 0) {
        // can delete it from the list
        struct event* temp = anfds[fd].next->next;
        free(anfds[fd].next);
        anfds[fd].next = temp;
      }
    }

    // 修改epoll或者kqueue
    if (ctl_poll) {
#ifdef __linux__
      // delete from epoll
      struct epoll_event epoll_ev;
      unsigned int events = 0;
      epoll_ev.data.fd = ev->fd;
      if (ev->events & Event_READ) {
        events = events | EPOLLIN | EPOLLET;
      }
      if (ev->events & Event_WRITE) {
        events = events | EPOLLOUT | EPOLLET;
      }
      epoll_ev.events = events;
      if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, &epoll_ev) == -1) {
        perror("delete event from epoll");
      }
#elif __APPLE__
      struct kevent changes[1];
      int16_t filter = 0;
      if (ev->events & Event_READ) {
        filter = filter | EVFILT_READ;
      }
      if (ev->events & Event_WRITE) {
        filter = filter | EVFILT_WRITE;
      }
      if (ev->events & Event_TIMER) {
        filter = filter | EVFILT_TIMER;
      }
      EV_SET(&changes[0], ev->fd, filter, EV_DELETE, 0, 0, NULL);
      if (kevent(kqfd, changes, 1, NULL, 0, NULL) == -1) {
        perror("epoll_ctl");
        return false;
      }
#endif
    }
  }
  return true;
}

bool EventLoop::mod_ev_only(const struct event* ev) {
  int fd = ev->fd;
  if (fd < 0 || fd > max_events) {
    return false;
  }

#ifdef __linux__
  if (anfds[fd].isused == 1) {
    // 修改epoll
    struct epoll_event epoll_ev;
    memset(&epoll_ev, 0, sizeof(epoll_ev));
    epoll_ev.events = 0;
    unsigned int events = 0;
    epoll_ev.data.fd = ev->fd;
    if (ev->events & Event_READ) {
      events = events | EPOLLIN | EPOLLET;
    }
    if (ev->events & Event_WRITE) {
      events = events | EPOLLOUT | EPOLLET;
    }
    epoll_ev.events = events;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &epoll_ev) == -1) {
      perror("in mod_event call epoll_ctl with EPOLL_CTL_MOD");
    } else {
      return true;
    }
  }
#elif __APPLE__
  if (true) {  // kqueue可以直接mod
    // 增加,对于相同的fd会覆盖
    struct kevent changes[1];
    int16_t filter = 0;
    if (ev->events & Event_READ) {
      filter = filter | EVFILT_READ;
    }
    if (ev->events & Event_WRITE) {
      filter = filter | EVFILT_WRITE;
    }
    if (ev->events & Event_TIMER) {
      filter = filter | EVFILT_TIMER;
    }
    EV_SET(&changes[0], ev->fd, filter, EV_ADD | EV_ENABLE | EV_CLEAR,
           0, ev->edata, NULL);
    if (kevent(kqfd, changes, 1, NULL, 0, NULL) == -1) {
      perror("epoll_ctl");
      return false;
    }
  }
#endif

  return false;
}


bool EventLoop::mod_event(const struct event*ev, bool ctl_poll) {
  // 删除fd上绑定的所有struct event事件,然后再绑定新的事件,修改epoll event
  int fd = ev->fd;
  if (fd < 0 || fd > max_events) {
    return false;
  }

#ifdef __linux__
  if (anfds[fd].isused == 1) {
#elif __APPLE__
  if (true) {  // kqueue可以直接mod
#endif
    anfds[fd].isused = 0;
    // detele event list
    struct event* cur = anfds[fd].next;
    anfds[fd].next = NULL;

    // 删除fd上的所有事件
    while (cur != NULL) {
      struct event* pre = NULL;
      pre = cur;
      cur = cur->next;
      pre->next = NULL;
      pre->data.ptr = NULL;
      free(pre);
      pre = NULL;
    }

    // epoll 在mod操作,可以直接修改，而kqueue没有，所以直接覆盖
    // 增加新事件
    if (add_event(ev, false)) {
      if (ctl_poll) {
        return mod_ev_only(ev);
      }
    } else {
      perror("in mod event call add event");
    }
  } else {
    perror("mod event, but fd isused equal 0");
  }
  return false;
}


bool EventLoop::event_stop(int fd) {
  if (anfds[fd].isused == 1) {
    anfds[fd].isused = 0;
    // detele event list
    struct event* cur = anfds[fd].next;

    while (cur != NULL) {
      struct event* pre = NULL;
      pre = cur;
      cur = cur->next;
      free(pre);
      pre = NULL;
    }

#ifdef __linux__
    // delete all epoll event
    if (epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL)) {
      printf("fd:%d\n", fd);
      perror("event stop and delete fd from epoll");
    }
#elif __APPLE__
    struct kevent deletechange[1];
    // 因为kqueue是通过《indent,filter》来标志一个过滤器，所以只能
    // 一个一个的删除
    int16_t filter =  EVFILT_READ;
    EV_SET(&deletechange[0], fd, filter, EV_DELETE, 0, 0, NULL);
    kevent(kqfd, deletechange, 1, NULL, 0, NULL);
    filter =  EVFILT_WRITE;
    EV_SET(&deletechange[0], fd, filter, EV_DELETE, 0, 0, NULL);
    kevent(kqfd, deletechange, 1, NULL, 0, NULL);
    filter =  EVFILT_TIMER;
    EV_SET(&deletechange[0], fd, filter, EV_DELETE, 0, 0, NULL);
    kevent(kqfd, deletechange, 1, NULL, 0, NULL);
#endif
  }

  return true;
}

bool EventLoop::event_ctl(OperatorType op, const struct event* ev) {
  std::unique_lock<std::recursive_mutex> locker(eventMutex);
  if (op == EventLoop::EVENT_CTL_ADD) {
    return this->add_event(ev);
  } else if (op == EventLoop::EVENT_CTL_DEL) {
    return this->delete_event(ev);
  } else if (op == EventLoop::EVENT_CTL_MOD) {
    return this->mod_event(ev);
  }
  else {
    perror("event_ctl can't know the value of op");
  }

  return false;
}

void EventLoop::start_loop() {
  while (!stop) {
#ifdef __linux__
    int nfds = epoll_wait(epollfd, events, max_events, -1);
    #elif __APPLE__
    int nfds = kevent(kqfd, NULL, 0, events, max_events, NULL);
    #endif
    if (nfds == -1) {
#ifdef __linux__
      perror("epoll wait");
#elif __APPLE__
      perror("kqueue wait");
#endif
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    //对于没有
    for (int n = 0; n < nfds; ++n) {
#ifdef __linux__
      int fd = events[n].data.fd;
#elif __APPLE__
      int fd = events[n].ident;
#endif
      // 因为fd未连接,所以会发生一个epollhup事件
      // 因为fd是et模式,所以只会触发一次,由于没有设置stop flag,所以没有影响
      if (shutdownfd == fd) {
        printf("shutdwon event\n");
        continue;
      }
      if (anfds[fd].isused == 1) {
        // find events for fd and callback
        int ev = 0;
#ifdef __linux__
        if (events[n].events & EPOLLIN) {
          ev |= Event_READ;
        }
        if (events[n].events & EPOLLOUT) {
          ev |= Event_WRITE;
        }
#elif __APPLE__
        // 一次只会有一个事件，所以可以直接用==
        if (events[n].filter == EVFILT_READ) {
          ev = Event_READ;
        } else if (events[n].filter == EVFILT_WRITE) {
          ev = Event_WRITE;
        } else if (events[n].filter == EVFILT_TIMER) {
          ev = Event_TIMER;
        }
#endif
        process_event(fd, ev);
      }
    }
  }
}

void EventLoop::stop_loop() {
  stop = true;
  if (shutdownfd > 0) {
#ifdef __linux__
    struct epoll_event epoll_ev;
    memset(&epoll_ev, 0, sizeof(epoll_ev));
    // 通知epoll wait醒来,只要fd可写,都会触发一次,
    // 也可以通过这种方法来通知发送队列中还有数据可写,
    // 达到类似条件变量的效果,但是优点是基于事件机制,不用线程阻塞.
    unsigned int events = EPOLLOUT;

    epoll_ev.data.fd = shutdownfd;
    epoll_ev.events = events;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, this->shutdownfd, &epoll_ev);
#elif __APPLE__
    struct kevent changes[1];
    int16_t filter = EVFILT_WRITE;
    EV_SET(&changes[0], shutdownfd, filter, EV_ADD | EV_ENABLE | EV_CLEAR,
           0, 0, NULL);
    kevent(kqfd, changes, 1, NULL, 0, NULL);
#endif
  }
}

void EventLoop::process_event(int fd, int events) {
  if (fd < 0 || fd > max_events) {
    return;
  }

  std::unique_lock<std::recursive_mutex> locker(eventMutex);
  if (anfds[fd].isused != 1) {
    return;
  }
  if (anfds[fd].events & events) {
    struct event* io_w = anfds[fd].next;
    while (io_w != NULL && io_w->cb != NULL) {
      // events是大于0的1,2,4，所以要以用&来断定是否有相同的
      if (io_w->events & events && io_w->fd == fd) {
        io_w->cb(io_w, io_w->events, owner);
      }
      if (anfds[fd].isused) {  // call back may be delete
        io_w = io_w->next;
      } else {
        break;
      }
    }
  }
}

bool EventLoop::array_needsize(int need_cnt) {
  int cur = max_events;
  if (need_cnt > cur) {
    int newcnt = cur;
    do {
      newcnt += (newcnt >> 1) + 16;
    }while(need_cnt > newcnt);

    struct ANFD* tempadfds =
        (struct ANFD*)realloc(anfds, sizeof(struct ANFD)*newcnt);
    if (tempadfds == NULL) {
      return false;
    }
    anfds = tempadfds;
#ifdef __linux__
    struct epoll_event* tempevents =
        (struct epoll_event*)realloc(events, sizeof(struct epoll_event)*newcnt);
#elif __APPLE__
    struct kevent* tempevents =
        (struct kevent*)realloc(events, sizeof(struct kevent)*newcnt);
#endif
    if (tempevents == NULL) {
      return false;
    }

    events = tempevents;

    /*
    fprintf(stdin, "relloc %ld for epoll_event array\n",
            sizeof(struct epoll_event)*newcnt);
    */
    init_events(cur, newcnt-cur);
    max_events = newcnt;
  }
  return true;
}

void EventLoop::init_events(int start, int count) {
  for (int i = 0; i < count; i++) {
    anfds[i+start].isused = 0;
    anfds[i+start].fd = 0;
    anfds[i+start].events = 0;
    anfds[i+start].next = NULL;
  }
}

}  // namespace base
}  // namespace sails


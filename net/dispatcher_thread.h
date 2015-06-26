// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: dispatcher_thread.h
// Description: 分发已经解析到的消息到处理线程队列中
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:56:35



#ifndef SAILS_NET_DISPATCHER_THREAD_H_
#define SAILS_NET_DISPATCHER_THREAD_H_
#include <unistd.h>
#include <thread>
#include "sails/net/net_thread.h"

namespace sails {
namespace net {


template <typename T>
class DispatcherThread {
 public:
  explicit DispatcherThread(EpollServer<T>* server);

  // 网络线程运行状态
  enum RunStatus {
    RUNING,
    PAUSE,
    STOPING
  };

  void run();
  void terminate();
  void join();

  static void dispatch(DispatcherThread<T>* dispacher);

 private:
  EpollServer<T>* server;
  int status;
  std::thread *thread;
  bool continueRun;
};




template <typename T>
DispatcherThread<T>::DispatcherThread(EpollServer<T>* server) {
  this->server = server;
  status = DispatcherThread<T>::STOPING;
  thread = NULL;
  continueRun = false;
}

template <typename T>
void DispatcherThread<T>::run() {
  continueRun = true;
  thread = new std::thread(dispatch, this);
  status = DispatcherThread<T>::RUNING;
}


template <typename T>
void DispatcherThread<T>::dispatch(DispatcherThread<T>* dispacher) {
  while (dispacher->continueRun) {
    TagRecvData<T>* data = NULL;
    if (dispacher->server->use_dispatch_thread) {
      data = dispacher->server->GetRecvPacket();
      if (data != NULL) {
        // 开始分发消息，这里考虑到有些消息要按照严格的先后顺序来处理
        // 为了达到这个效果，让他在一个处理线程中最好，不然的话，可能
        // 有多个线程同步处理，然后后来的消息反而先完成
        // 如果没有这个要求，就可以不用分发线程了，而是handle直接从
        // 网络线程那里拿数据处理，也不用再为handle接收一个接收队列
        // 这样少经过一个线程队列，速度也会提高不少
        int fd = data->fd;
        int handleNum = dispacher->server->GetHandleNum();
        int selectedHandle = fd % handleNum;
        dispacher->server->AddHandleData(data, selectedHandle);
      }
    } else {
      sleep(2);
    }
  }
}

template <typename T>
void DispatcherThread<T>::terminate() {
  continueRun = false;
}

template <typename T>
void DispatcherThread<T>::join() {
  if (thread != NULL) {
    thread->join();
    status = DispatcherThread<T>::STOPING;
    delete thread;
    thread = NULL;
  }
}

}  // namespace net
}  // namespace sails



#endif  // SAILS_NET_DISPATCHER_THREAD_H_











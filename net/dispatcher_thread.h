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


template<typename T>
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




template<typename T>
DispatcherThread<T>::DispatcherThread(EpollServer<T>* server) {
  this->server = server;
  status = DispatcherThread<T>::STOPING;
  thread = NULL;
  continueRun = false;
}

template<typename T>
void DispatcherThread<T>::run() {
  continueRun = true;
  thread = new std::thread(dispatch, this);
  status = DispatcherThread<T>::RUNING;
}


template<typename T>
void DispatcherThread<T>::dispatch(DispatcherThread<T>* dispacher) {
  while (dispacher->continueRun) {
    dispacher->server->DipacherWait();

    int recvQueueNum = dispacher->server->GetRecvQueueNum();
    for (int i = 0; i < recvQueueNum; i++) {
      TagRecvData<T>* data = NULL;
      do {
        data = dispacher->server->GetRecvPacket(i);
        if (data != NULL) {
          // 开始分发消息
          int fd = data->fd;
          int handleNum = dispacher->server->GetHandleNum();
          int selectedHandle = fd % handleNum;
          dispacher->server->AddHandleData(data, selectedHandle);
        }
      } while (data != NULL);
    }
  }
}

template<typename T>
void DispatcherThread<T>::terminate() {
  continueRun = false;
  server->NotifyDispacher();
}

template<typename T>
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











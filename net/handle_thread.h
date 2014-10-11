// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: handle_thread.h
// Description: 处理线程,分发线程会把消息放到处理线程的队列中,
//              消息处理完成后,调用server的send
//              来发送消息给客户端
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:57:59



#ifndef SAILS_NET_HANDLE_THREAD_H_
#define SAILS_NET_HANDLE_THREAD_H_

#include <string>
#include "sails/net/net_thread.h"

namespace sails {
namespace net {


template <typename T> class EpollServer;

template <typename T>
class HandleThread {
 public:
  explicit HandleThread(EpollServer<T> *server);

  virtual ~HandleThread();


  // 处理线程运行状态
  enum RunStatus {
    RUNING,
    STOPING
  };

  // 获取服务
  EpollServer<T>* getEpollServer();

  // 线程处理方法
  virtual void run();

  void terminate();

  void join();

 public:
  // 对象初始化
  virtual void initialize() {}

  // dispatcher线程分发
  void addForHandle(TagRecvData<T> *data);

  /*
  // 发送数据
  void sendResponse(unsigned int uid,
                    const std::string &sSendBuffer,
                    const std::string &ip, int port, int fd);

  // 关闭链接
  void close(unsigned int uid, int fd);
  */
 protected:
  static void runThread(HandleThread<T>* handle);
  // 具体的处理逻辑
  virtual void handleImp();

  // 处理函数
  // @param stRecvData: 接收到的数据
  virtual void handle(const TagRecvData<T> &recvData) = 0;

  // 处理连接关闭通知，包括
  // 1.close by peer
  // 2.recv/send fail
  // 3.close by timeout or overload
  // @param stRecvData:
  // virtual void handleClose(const TagRecvData<T> &recvData) {}

  // 心跳(每处理完一个请求或者等待请求超时都会调用一次)
  virtual void heartbeat() {}

  // 线程已经启动, 进入具体处理前调用stopHandle
  virtual void startHandle() {}

  // 线程马上要退出时调用
  virtual void stopHandle() {}

 protected:
  EpollServer<T>  *server;

  // 将要处理的数据队列
  recv_queue<T> handlelist;

  bool continueHanle;
  // 等待时间
  uint32_t  _iWaitTime;

  std::thread *thread;
  int status;
};








template <typename T>
HandleThread<T>::HandleThread(EpollServer<T> *server) {
  this->server = server;
  this->status = HandleThread<T>::STOPING;
  continueHanle = true;
}

template <typename T>
HandleThread<T>::~HandleThread() {
  if (status != HandleThread<T>::STOPING) {
    terminate();
    join();
    delete thread;
    thread = NULL;
  }

  // 删除handlelist中的数据
  bool hashandleData = false;
  do {
    hashandleData= false;
    TagRecvData<T>* data = NULL;
    handlelist.pop_front(data, 100);
    if (data != NULL) {
      hashandleData = true;
      T* t = data->data;
      if (t != NULL) {
        server->Tdeleter(t);
        t = NULL;
      }
      delete data;
      data = NULL;
    }
  } while (hashandleData);
}

template <typename T>
EpollServer<T>* HandleThread<T>::getEpollServer() {
  return server;
}

template <typename T>
void HandleThread<T>::run() {
  initialize();
  startHandle();
  thread = new std::thread(runThread, this);
  this->status = HandleThread<T>::RUNING;
}

template <typename T>
void HandleThread<T>::runThread(HandleThread<T>* handle) {
  if (handle != NULL) {
    handle->handleImp();
  }
}

template <typename T>
void HandleThread<T>::terminate() {
  continueHanle = false;
  stopHandle();
}

template <typename T>
void HandleThread<T>::join() {
  thread->join();
  status = HandleThread<T>::STOPING;
  delete thread;
  thread = NULL;
}


template <typename T>
void HandleThread<T>::addForHandle(TagRecvData<T> *data) {
  if (!handlelist.push_back(data)) {
    // 删除它
    T* t = data->data;
    if (t != NULL) {
      server->Tdeleter(t);
      data->data = NULL;
    }
    delete data;
  }
}


template <typename T>
void HandleThread<T>::handleImp() {
  // 从接收队列中得到数据,然后调用handle()处理
  while (continueHanle) {
    TagRecvData<T>* data = NULL;
    handlelist.pop_front(data, 100);
    if (data != NULL) {
      TagRecvData<T>& recvData = *data;
      handle(recvData);
      heartbeat();
      if (data->data != NULL) {
        server->Tdeleter(data->data);
        data->data = NULL;
      }
      delete data;
    } else {
    }
  }
}


}  // namespace net
}  // namespace sails



#endif  // SAILS_NET_HANDLE_THREAD_H_

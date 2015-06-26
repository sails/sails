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
#include <exception>
#include "sails/net/net_thread.h"

namespace sails {
namespace net {

// T 指接收和发送的数据结构
// U 在这儿是不需要的，但是因为EpollServer需要
template <typename T>
class HandleThread {
 public:
  // 处理线程运行状态
  enum RunStatus {
    RUNING,
    STOPING
  };

  // 状态
  struct HandleThreadStatus {
    RunStatus status;
    uint64_t handle_times;
    uint32_t handle_queue_capacity;
    uint32_t handle_queue_size;
  };

  explicit HandleThread(EpollServer<T> *server);

  virtual ~HandleThread();

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
  virtual void handle(const TagRecvData<T> &recvData);

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

 public:
  size_t id;
  // 统计相关
  HandleThreadStatus GetStatus();

 protected:
  EpollServer<T>  *server;

  // 将要处理的数据队列
  recv_queue<T> handlelist;

  bool continueHanle;
  // 等待时间
  uint32_t  _iWaitTime;

  std::thread *hthread;
  RunStatus status;
  uint64_t handle_times;
};








template <typename T>
HandleThread<T>::HandleThread(EpollServer<T> *server) {
  this->server = server;
  this->status = HandleThread<T>::STOPING;
  continueHanle = true;
  handle_times = 0;
  id = 0;
}

template <typename T>
HandleThread<T>::~HandleThread() {
  if (status != HandleThread<T>::STOPING) {
    terminate();
    join();
    delete hthread;
    hthread = NULL;
  }

  // 删除handlelist中的数据
  bool hashandleData = false;
  do {
    hashandleData = false;
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
  hthread = new std::thread(runThread, this);
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
  hthread->join();
  status = HandleThread<T>::STOPING;
  delete hthread;
  hthread = NULL;
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
    if (server->useMemoryPool) {
      data->~TagRecvData<T>();
      this->server->memory_pool.deallocate(
          reinterpret_cast<char*>(data), sizeof(TagRecvData<T>));
    } else {
      delete data;
    }
    data = NULL;
  }
}


template <typename T>
void HandleThread<T>::handleImp() {
  // 从接收队列中得到数据,然后调用handle()处理
  while (continueHanle) {
    TagRecvData<T>* data = NULL;
    if (!server->use_dispatch_thread) {
      data = server->GetRecvPacket();
    } else {
      handlelist.pop_front(data, 100);
    }

    // 为了实现所有主逻辑的单线程化，在每次循环中，给业务处理自有消息的机会
    this->server->handleCustomMessage(this);
    if (data != NULL) {
      TagRecvData<T>& recvData = *data;
      handle(recvData);
      handle_times++;
      if (handle_times > INT64_MAX-10) {
        handle_times = 0;
      }
      heartbeat();
      if (data->data != NULL) {
        server->Tdeleter(data->data);
        data->data = NULL;
      }
      if (server->useMemoryPool) {
        data->~TagRecvData<T>();
        this->server->memory_pool.deallocate(
            reinterpret_cast<char*>(data), sizeof(TagRecvData<T>));
      } else {
        delete data;
      }
      data = NULL;
    } else {
    }
  }
}

template <typename T>
void HandleThread<T>::handle(const TagRecvData<T> &recvData) {
  try {
    this->server->handle(recvData);
  } catch (std::exception &ex) {
    // 增加异常处理机制,防止客户逻辑错误让整个程序崩溃
    log::LoggerFactory::getLog("server")->error("handle %s", ex.what());
  } catch (...) {
    log::LoggerFactory::getLog("server")->error("handle unknown error");
  }
}


template <typename T>
typename HandleThread<T>::HandleThreadStatus HandleThread<T>::GetStatus() {
  HandleThreadStatus stat;
  stat.status = this->status;
  stat.handle_times = this->handle_times;
  stat.handle_queue_capacity = handlelist.MaxSize();
  stat.handle_queue_size = handlelist.size();
  return stat;
}

}  // namespace net
}  // namespace sails



#endif  // SAILS_NET_HANDLE_THREAD_H_

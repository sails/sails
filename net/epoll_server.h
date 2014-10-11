// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: epoll_server.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:57:33



#ifndef SAILS_NET_EPOLL_SERVER_H_
#define SAILS_NET_EPOLL_SERVER_H_

#include <signal.h>
#include <string>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include "sails/base/thread_queue.h"
#include "sails/net/handle_thread.h"
#include "sails/net/dispatcher_thread.h"
#include "sails/net/net_thread.h"


namespace sails {

extern sails::log::Logger serverlog;

}

namespace sails {
namespace net {

template<typename T>
class EpollServer {
  
 public:
  // 构造函数
  explicit EpollServer(unsigned int NetThreadNum = 1);
  // 析构函数
  virtual ~EpollServer();

  // 创建epoll
  void createEpoll();

  // 设置空连接超时时间
  void setEmptyConnTimeout(int timeout) { connectorTimeout = timeout;}

  int getEmptyConnTimeout() { return connectorTimeout;}

  // 绑定一个网络线程去监听端口
  void bind(int port);

  // 开始运行网络线程
  bool startNetThread();

  // 终止网络线程
  bool stopNetThread();

  bool add_handle(HandleThread<T> *handle);

  // 开始运行处理线程
  bool startHandleThread();
  // 终止处理线程
  bool stopHandleThread();


  // T 的删除器
  virtual void Tdeleter(T *data);

  virtual void create_connector_cb(std::shared_ptr<net::Connector> connector);

  // 循环调用parser
  virtual void parseImp(std::shared_ptr<net::Connector> connector);
  // 解析数据包
  virtual T* parse(std::shared_ptr<net::Connector> connector) {
    printf("need implement parser method in subclass, connector:%d\n",
           connector->get_connector_fd());
    return NULL;
  }

  // 当epoll读到0个数据时，客户端主动close
  virtual void closed_connect_cb(std::shared_ptr<net::Connector> connector);

  // 当连接超时时,提供应用层处理机会
  virtual void connector_timeout_cb(net::Connector* connector);

  // 增加连接
  void addConnector(std::shared_ptr<net::Connector> connector, int fd);

  // 选择网络线程
  NetThread<T>* getNetThreadOfFd(int fd) {
    return netThreads[fd % netThreads.size()];
  }

  // 分发线程等待数据
  void dipacher_wait();

  // 通知分发线程有数据
  void notify_dispacher();

  // 得到接收队列个数,用于dispacher线程循环的从队列中得到数据
  int getRecvQueueNum();

  // 得到接收到的数据数
  size_t getRecvDataNum();

  // 从io线程队列中得到数据包,用于dispacher线程
  // index指io线程的标志
  TagRecvData<T>* getRecvPacket(uint32_t index);

  // 处理线程数
  int getHandleNum() {
    return handleThreads.size();
  }

  // 向处理线程中加入消息
  void addHandleData(TagRecvData<T>*data, int handleIndex);

  // 发送数据
  void send(const std::string &s, const std::string &ip,
            uint16_t port, int uid, int fd);

  // 关闭连接
  void close_connector(const std::string &ip, uint16_t port, int uid, int fd);

  // sigpipe信号处理函数
  static void handle_sigpipe(int sig);

  friend class HandleThread<T>;

 private:
  // 网络线程
  std::vector<NetThread<T>*> netThreads;
  // 逻辑处理线程
  std::vector<HandleThread<T>*> handleThreads;
  // 消息分发线程
  DispatcherThread<T>* dispacher_thread;

  // 网络线程数目
  unsigned int netThreadNum;

  // io线程将数据入队时通过dispacher线程分发
  std::mutex dispacher_mutex;
  std::condition_variable dispacher_notify;

  // 服务是否停止
  bool bTerminate;

  // 业务线程是否启动
  bool handleStarted;

  // 空链超时时间
  int connectorTimeout;

  // 防止当连接断开时的瞬间,服务器还在向它写数据时的SIGPIPE错误
  // 没有日志,也没有core文件,很难发现
  struct sigaction sigpipe_action;
};







template<typename T>
EpollServer<T>::EpollServer(unsigned int netThreadNum) {
  this->netThreadNum = netThreadNum;
  if (this->netThreadNum < 0) {
    this->netThreadNum = 1;
  } else if (this->netThreadNum > 15) {
    this->netThreadNum = 15;
  }
  for (size_t i = 0; i < this->netThreadNum; i++) {
    NetThread<T> *netThread = new NetThread<T>(this);
    netThreads.push_back(netThread);
  }
  bTerminate = true;
  connectorTimeout = 0;

  sigpipe_action.sa_handler = EpollServer<T>::handle_sigpipe;
  sigemptyset(&sigpipe_action.sa_mask);
  sigpipe_action.sa_flags = 0;
  sigaction(SIGPIPE, &sigpipe_action, NULL);
}


template<typename T>
EpollServer<T>::~EpollServer() {
  printf("delete server\n");
  for (size_t i = 0; i < this->netThreadNum; i++) {
    delete netThreads[i];
  }
}


template<typename T>
void EpollServer<T>::handle_sigpipe(int sig) {
  serverlog.warn("sigpipe %d", sig);
}

template<typename T>
void EpollServer<T>::createEpoll() {
  for (size_t i = 0; i < this->netThreadNum; i++) {
    netThreads[i]->create_event_loop();
  }
}


template<typename T>
void EpollServer<T>::bind(int port) {
  netThreads[0]->bind(port);
}

// 开始运行网络线程
template<typename T>
bool EpollServer<T>::startNetThread() {
  for (size_t i = 0; i < netThreadNum; i++) {
    printf("start net thread i:%lu\n", i);
    netThreads[i]->run();
  }
  return true;
}

// 终止网络线程
template<typename T>
bool EpollServer<T>::stopNetThread() {
  for (size_t i = 0; i < netThreadNum; i++) {
    netThreads[i]->terminate();
    netThreads[i]->join();
  }
  return true;
}


template<typename T>
void EpollServer<T>::Tdeleter(T *data) {
  free(data);
}

template<typename T>
void EpollServer<T>::addConnector(
    std::shared_ptr<net::Connector> connector, int fd) {
  NetThread<T>* netThread = getNetThreadOfFd(fd);
  netThread->add_connector(connector);
}


template<typename T>
void EpollServer<T>::parseImp(std::shared_ptr<net::Connector> connector) {
  T* packet = NULL;
  while ((packet = this->parse(connector)) != NULL) {
    TagRecvData<T>* data = new TagRecvData<T>();
    data->uid = connector->getId();
    data->data = packet;
    data->ip = connector->getIp();
    data->port= connector->getPort();
    data->fd = connector->get_connector_fd();

    NetThread<T>* netThread = getNetThreadOfFd(connector->get_connector_fd());
    netThread->addRecvList(data);
  }
}

template<typename T>
void EpollServer<T>::create_connector_cb(
    std::shared_ptr<net::Connector> connector) {
  printf("create connector cb %d\n", connector->get_connector_fd());
}

template<typename T>
bool EpollServer<T>::add_handle(HandleThread<T> *handle) {
  handleThreads.push_back(handle);
  return true;
}

template<typename T>
bool EpollServer<T>::startHandleThread() {
  int i = 0;
  for (HandleThread<T> *handle : handleThreads) {
    printf("start handle thread i:%d\n", i);
    handle->run();
  }
  dispacher_thread = new DispatcherThread<T>(this);
  dispacher_thread->run();
  return true;
}


template<typename T>
bool EpollServer<T>::stopHandleThread() {
  printf("stop dispacher thread\n");
  dispacher_thread->terminate();
  dispacher_thread->join();
  delete dispacher_thread;
  dispacher_thread = NULL;
  printf(" end stop dispacher\n");

  printf("stop handle thread\n");
  for (uint32_t i = 0; i < handleThreads.size(); i++) {
    handleThreads[i]->terminate();
    handleThreads[i]->join();
    handleThreads[i] = NULL;
  }
  return true;
}

template<typename T>
void EpollServer<T>::addHandleData(TagRecvData<T>*data, int handleIndex) {
  handleThreads[handleIndex]->addForHandle(data);
}


template<typename T>
void EpollServer<T>::dipacher_wait() {
  std::unique_lock<std::mutex> locker(dispacher_mutex);
  dispacher_notify.wait(locker);
}


template<typename T>
void EpollServer<T>::notify_dispacher() {
  std::unique_lock<std::mutex> locker(dispacher_mutex);
  dispacher_notify.notify_one();
}

template<typename T>
int EpollServer<T>::getRecvQueueNum() {
  return netThreads.size();
}


template<typename T>
size_t EpollServer<T>::getRecvDataNum() {
  size_t num = 0;
  for (int i = 0; i < netThreads.size(); i++) {
    num = num + netThreads[i]->get_recvqueue_size();
  }
  return num;
}

template<typename T>
TagRecvData<T>* EpollServer<T>::getRecvPacket(uint32_t index) {
  TagRecvData<T> *data = NULL;
  if (netThreads.size() >= index) {
    netThreads[index]->getRecvData(data, 0);  //不阻塞
  }
  return data;
}

template<typename T>
void EpollServer<T>::send(const std::string &s,
                          const std::string &ip,
                          uint16_t port, int uid, int fd) {
  NetThread<T>* netThread = getNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->send(ip, port, uid, s);
  }
}

template<typename T>
void EpollServer<T>::close_connector(
    const std::string &ip, uint16_t port, int uid, int fd) {
  NetThread<T>* netThread = getNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->close_connector(ip, port, uid, fd);
  }
}

template<typename T>
void EpollServer<T>::closed_connect_cb(
    std::shared_ptr<net::Connector> connector) {
  if (!connector->isClosed()) {
    close_connector(connector->getIp(),
                    connector->getPort(),
                    connector->getId(), connector->get_connector_fd());
  }
}

template<typename T>
void EpollServer<T>::connector_timeout_cb(net::Connector* connector) {
  printf("connector %d timeout, perhaps need to do something",
         connector->get_connector_fd());
}

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET_EPOLL_SERVER_H_
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
namespace net {

template<typename T, typename U >
class EpollServer {
  
 public:
  // 构造函数
  explicit EpollServer(unsigned int NetThreadNum = 1);
  // 析构函数
  virtual ~EpollServer();

  // 创建epoll
  void CreateEpoll();

  // 设置空连接超时时间
  void SetEmptyConnTimeout(int timeout) { connectorTimeout = timeout;}

  int GetEmptyConnTimeout() { return connectorTimeout;}

  // 绑定一个网络线程去监听端口
  void Bind(int port);

  // 开始运行网络线程
  bool StartNetThread();

  // 增加网络线程
  bool AddHandle(HandleThread<T> *handle);

  // 开始运行处理线程
  bool StartHandleThread();
  // 终止处理线程
  bool StopHandleThread();

  // 终止网络线程
  bool StopNetThread();

 public:
  // T 的删除器,用于用户处理接收到的消息后,框架层删除它
  // 默认是用户free,如果T是通过new建立,一定重载它
  virtual void Tdeleter(T *data);
  
  // 创建连接后调用,可能用户层需要在连接建立后创建一些用户相关信息
  virtual void CreateConnectorCB(std::shared_ptr<net::Connector> connector);

  // 循环调用parser, 子类可能要在得到parse的结果后,为TagRecvData设置一些属性
  virtual void ParseImp(std::shared_ptr<net::Connector> connector);
  // 解析数据包,ParseImp会循环调用,所以一次只用解析出一个包
  virtual T* Parse(std::shared_ptr<net::Connector> connector) {
    printf("need implement parser method in subclass, connector:%d\n",
           connector->get_connector_fd());
    return NULL;
  }

  // 当epoll读到0个数据时，客户端主动close.
  // 在调用函数之后,netThread会主动调用connect close
  // 如果要处理额外情况,在子类中重新实现它
  virtual void ClosedConnectCB(std::shared_ptr<net::Connector> connector);

  // 当连接超时时,提供应用层处理机会
  // 在调用函数之后,netThread会自己关闭连接,所有子类中不用再调用close connector
  virtual void ConnectorTimeoutCB(net::Connector* connector);

 public:
  // 增加连接
  void AddConnector(std::shared_ptr<net::Connector> connector, int fd);

  // 选择网络线程
  NetThread<T>* GetNetThreadOfFd(int fd) {
    return netThreads[fd % netThreads.size()];
  }

  // 分发线程等待数据
  void DipacherWait();

  // 通知分发线程有数据
  void NotifyDispacher();

  // 得到接收队列个数,用于dispacher线程循环的从队列中得到数据
  int GetRecvQueueNum();

  // 得到接收到的数据数
  size_t GetRecvDataNum();

  // 从io线程队列中得到数据包,用于dispacher线程
  // index指io线程的标志
  TagRecvData<T>* GetRecvPacket(uint32_t index);

  // 处理线程数
  int GetHandleNum() {
    return handleThreads.size();
  }

  // 向处理线程中加入消息
  void AddHandleData(TagRecvData<T>*data, int handleIndex);

  // 发送数据
  void send(const std::string &s, const std::string &ip,
            uint16_t port, int uid, int fd);

  // 关闭连接
  void CloseConnector(const std::string &ip, uint16_t port, int uid, int fd);

  // sigpipe信号处理函数
  static void HandleSigpipe(int sig);

  friend class HandleThread<T>;

 public:
  int ListenPort() { return listenPort;}
 private:
  int listenPort;
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







template<typename T, typename U>
EpollServer<T, U>::EpollServer(unsigned int netThreadNum) {
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
  listenPort = 0;
  bTerminate = true;
  connectorTimeout = 0;

  sigpipe_action.sa_handler = EpollServer<T>::HandleSigpipe;
  sigemptyset(&sigpipe_action.sa_mask);
  sigpipe_action.sa_flags = 0;
  sigaction(SIGPIPE, &sigpipe_action, NULL);
}


template<typename T, typename U>
EpollServer<T, U>::~EpollServer() {
  printf("delete server\n");
  for (size_t i = 0; i < this->netThreadNum; i++) {
    if (netThreads[i] != NULL) {
          delete netThreads[i];
    }
  }
}


template<typename T, typename U>
void EpollServer<T, U>::HandleSigpipe(int sig) {
  log::LoggerFactory::getLogD("server")->warn("sigpipe %d", sig);
}

template<typename T, typename U>
void EpollServer<T, U>::CreateEpoll() {
  for (size_t i = 0; i < this->netThreadNum; i++) {
    netThreads[i]->create_event_loop();
  }
}


template<typename T, typename U>
void EpollServer<T, U>::Bind(int port) {
  listenPort = port;
  netThreads[0]->bind(port);
}

// 开始运行网络线程
template<typename T, typename U>
bool EpollServer<T, U>::StartNetThread() {
  for (size_t i = 0; i < netThreadNum; i++) {
    printf("start net thread i:%lu\n", i);
    netThreads[i]->run();
  }
  return true;
}

// 终止网络线程
template<typename T, typename U>
bool EpollServer<T, U>::StopNetThread() {
  for (size_t i = 0; i < netThreadNum; i++) {
    netThreads[i]->terminate();
    netThreads[i]->join();
<<<<<<< HEAD
    // 因为当网络线程结束时,要进行一些后续处理
    // 比如关闭连接,删除用户数据等,所以不能等到server析构时再来delete
    // 这里是要删除全部网络线程,所以不用把后面的线程往前移
    // 如果只是删除某一个,则要移动,否则数据得不到处理.
=======
>>>>>>> 86a971fbba5eb2f1a20bc3864e0ba9d3c9d69a54
    delete netThreads[i];
    netThreads[i] = NULL;
  }
  return true;
}


template<typename T, typename U>
void EpollServer<T, U>::Tdeleter(T *data) {
  free(data);
}

template<typename T, typename U>
void EpollServer<T, U>::AddConnector(
    std::shared_ptr<net::Connector> connector, int fd) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  netThread->add_connector(connector);
}


template<typename T, typename U>
void EpollServer<T, U>::ParseImp(
    std::shared_ptr<net::Connector> connector) {
  T* packet = NULL;
  while ((packet = this->Parse(connector)) != NULL) {
    TagRecvData<T>* data = new TagRecvData<T>();
    data->uid = connector->getId();
    data->data = packet;
    data->ip = connector->getIp();
    data->port= connector->getPort();
    data->fd = connector->get_connector_fd();

    NetThread<T>* netThread = GetNetThreadOfFd(connector->get_connector_fd());
    netThread->addRecvList(data);
  }
}

template<typename T, typename U>
void EpollServer<T, U>::CreateConnectorCB(
    std::shared_ptr<net::Connector> connector) {
  printf("create connector cb %d\n", connector->get_connector_fd());
}

template<typename T, typename U>
bool EpollServer<T, U>::AddHandle(HandleThread<T> *handle) {
  handleThreads.push_back(handle);
  return true;
}

template<typename T, typename U>
bool EpollServer<T, U>::StartHandleThread() {
  int i = 0;
  for (HandleThread<T> *handle : handleThreads) {
    printf("start handle thread i:%d\n", i);
    handle->run();
  }
  dispacher_thread = new DispatcherThread<T>(this);
  dispacher_thread->run();
  return true;
}


template<typename T, typename U>
bool EpollServer<T, U>::StopHandleThread() {
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

template<typename T, typename U>
void EpollServer<T, U>::AddHandleData(TagRecvData<T>*data, int handleIndex) {
  handleThreads[handleIndex]->addForHandle(data);
}


template<typename T, typename U>
void EpollServer<T, U>::DipacherWait() {
  std::unique_lock<std::mutex> locker(dispacher_mutex);
  dispacher_notify.wait(locker);
}


template<typename T, typename U>
void EpollServer<T, U>::NotifyDispacher() {
  std::unique_lock<std::mutex> locker(dispacher_mutex);
  dispacher_notify.notify_one();
}

template<typename T, typename U>
int EpollServer<T, U>::GetRecvQueueNum() {
  return netThreads.size();
}


template<typename T, typename U>
size_t EpollServer<T, U>::GetRecvDataNum() {
  size_t num = 0;
  for (int i = 0; i < netThreads.size(); i++) {
    num = num + netThreads[i]->get_recvqueue_size();
  }
  return num;
}

template<typename T, typename U>
TagRecvData<T>* EpollServer<T, U>::GetRecvPacket(uint32_t index) {
  TagRecvData<T> *data = NULL;
  if (netThreads.size() >= index) {
    if (netThreads[index] != NULL) {
      netThreads[index]->getRecvData(data, 0);  //不阻塞
    }
  }
  return data;
}

template<typename T, typename U>
void EpollServer<T, U>::send(const std::string &s,
                          const std::string &ip,
                          uint16_t port, int uid, int fd) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->send(ip, port, uid, s);
  }
}

template<typename T, typename U>
void EpollServer<T, U>::CloseConnector(
    const std::string &ip, uint16_t port, int uid, int fd) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->close_connector(ip, port, uid, fd);
  }
}

template<typename T, typename U>
void EpollServer<T, U>::ClosedConnectCB(
    std::shared_ptr<net::Connector> connector) {
  log::LoggerFactory::getLogD("server")->debug("connetor %ld will be closed", connector->get_connector_fd());
}

template<typename T, typename U>
void EpollServer<T, U>::ConnectorTimeoutCB(net::Connector* connector) {
  log::LoggerFactory::getLogD("server")->debug("connector %d timeout, perhaps need to do something",
                                               connector->get_connector_fd());
}

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET_EPOLL_SERVER_H_

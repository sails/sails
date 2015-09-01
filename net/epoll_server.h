// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: epoll_server.h
//           server有两种工作模式:
//           第一种是把所有接收到的包加入epoll_server的队列中;
//           第二种是把包分发到不同的handle_thread的队列中;
//           当使用第二种模式时，由于锁冲突会减少，所以性能更高.
//           性能测试当server中8个网络线程+8个处理线程时，客户端30个同步线程
//           第一种模式达到17w tps,而第二种可以达到48w tps
//           由于第二种模式时网络线程中连接和处理线程中请求包的分派都是通过
//           socketfd取模，所以为了能让两个网络线程不会在入队请求包到同一个处
//           理线程的队列中而使锁冲突增加，处理线程的数据最好是网络线程的整数倍
//
//           还有一种情况是必须使用第二种模式的，当我们我们有多个处理线程
//           而且要保证一个异步请求的客户端的处理先后顺序时，如果使用第一
//           种模式，那可能会造成后来的包先处理
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:57:33



#ifndef NET_EPOLL_SERVER_H_
#define NET_EPOLL_SERVER_H_

#include <signal.h>
#include <string>
#include <vector>
#include <thread>  // NOLINT
#include <condition_variable>  // NOLINT
#include <mutex>  // NOLINT
#include "sails/base/thread_queue.h"
#include "sails/base/memory_pool.h"
#include "sails/net/handle_thread.h"
#include "sails/net/net_thread.h"


namespace sails {
namespace net {
// T 指接收和发送的数据结构
template<typename T>
class EpollServer {
 public:
  // 构造函数
  EpollServer();
  // 析构函数
  virtual ~EpollServer();

  // 初始化
  void Init(int port,
            int netThreadNum = 1,
            int timeout = 10,
            int handleThreadNum = 1,
            bool useMemoryPool = false,
            int runMode = 1);

  // 停止服务器
  void Stop();

 protected:
  // 创建epoll
  void CreateEpoll();

  // 设置空连接超时时间
  void SetEmptyConnTimeout(int timeout) { connectorTimeout = timeout;}

  // 绑定一个网络线程去监听端口
  int Bind(int port);

  // 开始运行网络线程
  bool StartNetThread();

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
  // 注意,如果在这里分配了内存,那么记得在关闭时释放
  virtual void CreateConnectorCB(std::shared_ptr<net::Connector> connector);

  // 循环调用parser, 子类可能要在得到parse的结果后,为TagRecvData设置一些属性
  virtual void ParseImp(std::shared_ptr<net::Connector> connector);
  // 解析数据包,ParseImp会循环调用,所以一次只用解析出一个包
  virtual T* Parse(std::shared_ptr<net::Connector> connector) {
    printf("need implement parser method in subclass, connector:%d\n",
           connector->get_connector_fd());
    return NULL;
  }

  // 处理函数
  virtual void handle(const TagRecvData<T> &recvData) {
    printf("need implement handle packet(uid:%d) in server class(will be call handle thread)\n", recvData.uid);  // NOLINT
  }

  // 每次handle被唤醒后都会调用，实现在主线程中处理自有数据的功能
  // 比如定时器任务或自有网络的异步响应;
  // 一般它需要配合业务自有的队列使用，先把要处理的custome消息放到队列中
  // 在这个函数中从队列中取出来处理.
  virtual void handleCustomMessage(HandleThread<T>*) {
  }

  // 在NetThread关闭连接之前调用，用来删除用户数据
  // 比如在连接时创建了player，在此时可以进行删除
  // 注意，它是在NetThread中调用，所以要区别资源是在哪里删除，
  // 有些可以直接在NetThread中删除,有些只能通过消息让handle线程删除
  virtual void ClosedConnectCB(std::shared_ptr<net::Connector> connector);

 public:
  int GetEmptyConnTimeout() { return connectorTimeout;}

  // ip过滤
  virtual bool isIpAllow(const std::string&) { return true; }

  // 增加连接
  void AddConnector(std::shared_ptr<net::Connector> connector, int fd);

  // 选择网络线程
  NetThread<T>* GetNetThreadOfFd(int fd) {
    return netThreads[fd % netThreads.size()];
  }

  // 插入网络线程接收到的数据
  bool InsertRecvData(TagRecvData<T>* data);

  // 得到接收到的数据数
  size_t GetRecvDataNum();

  // 从接收队列中得到数据包,用于dispacher线程
  TagRecvData<T>* GetRecvPacket();

  // 处理线程数
  int GetHandleNum() {
    return handleThreads.size();
  }

  // 发送数据
  void send(const std::string &s, const std::string &ip,
            uint16_t port, uint32_t uid, int fd);

  void send(char* message, int len, const std::string &ip,
            uint16_t port, uint32_t uid, int fd);

  // 提供给处理调用,它通过向io线程发送命令来达到目的,所以是多线程安全的
  // 关闭连接,关闭连接有三种情况,1:客户端主动关闭,2:服务器超时,
  // 3:服务器业务中关闭,不管哪种情况,最后都会通过调用这个函数来
  // 向netThread线程中发送命令来操作
  // 如果在创建连接时,绑定了资源, 为了能达到三种情况下都能统一删除资源，
  // 在NetThread删除connector时，会调用ClosedConnectCB，
  // 所以千万不要在其它地方去删除，否则可能造成资源泄漏
  void CloseConnector(
      const std::string &ip, uint16_t port, uint32_t uid, int fd);

  // 设置connector数据，它最终会在netThread中设置
  // 如果执行逻辑在NetThread中，可以直接修改而不通过这个接口
  void SetConnectorData(
      const std::string &ip, uint16_t port, uint32_t uid, int fd,
                        ExtData data);

  uint32_t GetTick() {
    return tick;
  }

  // sigpipe信号处理函数
  static void HandleSigpipe(int sig);

  friend class NetThread<T>;
  friend class HandleThread<T>;

 public:
  // 统计相关
  int ListenPort() { return listenPort;}
  int RunMode() { return runMode;}
  unsigned int NetThreadNum() {
    return netThreadNum;
  }
  unsigned int HandleThreadNum() {
    return handleThreadNum;
  }
  typename NetThread<T>::NetThreadStatus GetNetThreadStatus(
      int threadNum) {
    if (netThreads[threadNum] != NULL) {
      return netThreads[threadNum]->GetStatus();
    }
    typename NetThread<T>::NetThreadStatus status;
    return status;
  }
  typename HandleThread<T>::HandleThreadStatus GetHandleThreadStatus(
      int threadNum) {
    if (handleThreads[threadNum] != NULL) {
      return handleThreads[threadNum]->GetStatus();
    }
    typename HandleThread<T>::HandleThreadStatus status;
    return status;
  }

  // 内存池
  bool useMemoryPool;
  sails::base::MemoryPoll memory_pool;

 protected:
  int listenPort;

  // 运行模式，1：所以请求入同一个队列，2：请求分派到处理线程的队列中
  int runMode;

  // 网络线程
  std::vector<NetThread<T>*> netThreads;
  // 逻辑处理线程
  std::vector<HandleThread<T>*> handleThreads;

  // 网络线程数目
  unsigned int netThreadNum;

  uint32_t handleThreadNum;

  uint32_t tick;

  // 服务是否停止
  bool bTerminate;

  // 业务线程是否启动
  bool handleStarted;

  // 空链超时时间
  int connectorTimeout;

  // 接收的数据队列，处理线程直接从这里拿数据处理
  recv_queue<T> recvlist;

  // 防止当连接断开时的瞬间,服务器还在向它写数据时的SIGPIPE错误
  // 没有日志,也没有core文件,很难发现
  struct sigaction sigpipe_action;
};







template<typename T>
EpollServer<T>::EpollServer() {
  listenPort = 0;
  bTerminate = false;
  connectorTimeout = 0;
  useMemoryPool = false;
  tick = 0;

  sigpipe_action.sa_handler = EpollServer<T>::HandleSigpipe;
  sigemptyset(&sigpipe_action.sa_mask);
  sigpipe_action.sa_flags = 0;
  sigaction(SIGPIPE, &sigpipe_action, NULL);
}


template<typename T>
EpollServer<T>::~EpollServer() {
  printf("delete server\n");
  for (size_t i = 0; i < this->netThreadNum; i++) {
    if (netThreads[i] != NULL) {
          delete netThreads[i];
    }
  }
}


template<typename T>
void EpollServer<T>::Init(int port,
                          int netThreadNum,
                          int timeout,
                          int handleThreadNum,
                          bool useMemoryPool,
                          int runMode) {
  this->runMode = runMode;
  this->useMemoryPool = useMemoryPool;
  // 新建网络线程
  this->netThreadNum = netThreadNum;
  if (this->netThreadNum < 0) {
    this->netThreadNum = 1;
  } else if (this->netThreadNum > 15) {
    this->netThreadNum = 15;
  }
  for (size_t i = 0; i < this->netThreadNum; i++) {
    NetThread<T> *netThread = new NetThread<T>(this, i);
    netThreads.push_back(netThread);
  }
  CreateEpoll();
  SetEmptyConnTimeout(timeout);
  Bind(port);

  // 新建处理线程
  if (handleThreadNum < 0) {
    this->handleThreadNum = 1;
  } else if (handleThreadNum > 15) {
    this-> handleThreadNum = 15;
  } else {
    this->handleThreadNum = handleThreadNum;
  }
  for (size_t i = 0; i < this->handleThreadNum; i++) {
    HandleThread<T>* handleThread = new HandleThread<T>(this);
    handleThread->id = i;
    handleThreads.push_back(handleThread);
  }

  // 开始网络线程
  StartNetThread();
  // 开始处理线程
  StartHandleThread();
}



template<typename T>
void EpollServer<T>::HandleSigpipe(int sig) {
  WARN_LOG("server", "sigpipe %d", sig);
}

template<typename T>
void EpollServer<T>::CreateEpoll() {
  for (size_t i = 0; i < this->netThreadNum; i++) {
    netThreads[i]->create_event_loop();
  }
}


template<typename T>
int EpollServer<T>::Bind(int port) {
  listenPort = port;
  return netThreads[0]->bind(port);
}

// 开始运行网络线程
template<typename T>
bool EpollServer<T>::StartNetThread() {
  for (size_t i = 0; i < netThreadNum; i++) {
    netThreads[i]->run();
  }
  printf("start %u net threads\n", netThreadNum);
  return true;
}

// 终止网络线程
template<typename T>
bool EpollServer<T>::StopNetThread() {
  printf("stop NetThread\n");
  for (size_t i = 0; i < netThreadNum; i++) {
    // 因为当网络线程结束时,要进行一些后续处理
    // 比如关闭连接,删除用户数据等,所以不能等到server析构时再来delete
    // 这里是要删除全部网络线程,所以不用把后面的线程往前移
    // 如果只是删除某一个,则要移动,否则数据得不到处理.
    delete netThreads[i];
    netThreads[i] = NULL;
  }
  return true;
}

template<typename T>
void EpollServer<T>::Tdeleter(T *data) {
  free(data);
}

template<typename T>
void EpollServer<T>::AddConnector(
    std::shared_ptr<net::Connector> connector, int fd) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  netThread->add_connector(connector);
}


template<typename T>
void EpollServer<T>::ParseImp(
    std::shared_ptr<net::Connector> connector) {
  T* packet = NULL;
  while ((packet = this->Parse(connector)) != NULL) {
    TagRecvData<T>* data = NULL;
    if (useMemoryPool) {
      // Use memory pool
      data = (TagRecvData<T>*)memory_pool.allocate(sizeof(TagRecvData<T>));
      new(data) TagRecvData<T>();
    } else {
      data = new TagRecvData<T>();
    }


    data->uid = connector->getId();
    data->data = packet;
    data->ip = connector->getIp();
    data->port = connector->getPort();
    data->fd = connector->get_connector_fd();
    data->extdata = connector->data;

    // 插入
    if (!InsertRecvData(data)) {
      // 删除它
      T* t = data->data;
      if (t != NULL) {
        Tdeleter(t);
        data->data = NULL;
      }
      delete data;
    }
  }
}

template<typename T>
void EpollServer<T>::CreateConnectorCB(
    std::shared_ptr<net::Connector> connector) {
  DEBUG_LOG("server", "create connector cb %d\n",
            connector->get_connector_fd());
}

template<typename T>
bool EpollServer<T>::StartHandleThread() {
  int i = 0;
  for (HandleThread<T> *handle : handleThreads) {
    // printf("start handle thread i:%d\n", i);
    handle->run();
    i++;
  }
  printf("start %lu handle threads\n", handleThreads.size());
  return true;
}


template<typename T>
void EpollServer<T>::Stop() {
  this->StopHandleThread();
  this->StopNetThread();
  bTerminate = true;
}


template<typename T>
bool EpollServer<T>::StopHandleThread() {
  printf("stop handle thread\n");
  for (uint32_t i = 0; i < handleThreads.size(); i++) {
    handleThreads[i]->terminate();
    handleThreads[i]->join();
    delete handleThreads[i];
    handleThreads[i] = NULL;
  }
  return true;
}

// 插入网络线程接收到的数据
template<typename T>
bool EpollServer<T>::InsertRecvData(TagRecvData<T>* data) {
  if (runMode == 1) {
    return recvlist.push_back(data);
  } else {
    int handleNum = GetHandleNum();
    int selectedHandle = data->fd % handleNum;
    return handleThreads[selectedHandle]->addForHandle(data);
  }
}

template<typename T>
size_t EpollServer<T>::GetRecvDataNum() {
  return recvlist.size();
}

template<typename T>
TagRecvData<T>* EpollServer<T>::GetRecvPacket() {
  TagRecvData<T> *data = NULL;
  recvlist.pop_front(data, 100);  // 100ms
  return data;
}

template<typename T>
void EpollServer<T>::send(const std::string &s,
                          const std::string &ip,
                          uint16_t port, uint32_t uid, int fd) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->send(ip, port, uid, s);
  }
}

template<typename T>
void EpollServer<T>::send(char* message,
                          int len,
                          const std::string &ip,
                          uint16_t port, uint32_t uid, int fd) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->send(ip, port, uid, message, len);
  }
}

template<typename T>
void EpollServer<T>::CloseConnector(
    const std::string &ip, uint16_t port, uint32_t uid, int fd) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->close_connector(ip, port, uid, fd);
  }
}

template<typename T>
void EpollServer<T>::SetConnectorData(const std::string &ip,
                      uint16_t port,
                      uint32_t uid,
                      int fd,
                      ExtData data) {
  NetThread<T>* netThread = GetNetThreadOfFd(fd);
  if (netThread != NULL) {
    netThread->SetConnectorData(ip, port, uid, fd, data);
  }
}

template<typename T>
void EpollServer<T>::ClosedConnectCB(
    std::shared_ptr<net::Connector> connector) {
  DEBUG_LOG("server", "connetor %ld will be closed",
            connector->get_connector_fd());
}

}  // namespace net
}  // namespace sails

#endif  // NET_EPOLL_SERVER_H_

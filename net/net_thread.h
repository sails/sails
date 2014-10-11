// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: net_thread.h
// Description: 网络线程,在一个线程中同时处理接收和发送数据
//              可以同时开启多个网络线程
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 12:00:01



#ifndef SAILS_NET_NET_THREAD_H_
#define SAILS_NET_NET_THREAD_H_

#include <string>
#include <deque>
#include <list>
#include <memory>
#include <thread>
#include "sails/base/util.h"
#include "sails/base/thread_queue.h"
#include "sails/base/event_loop.h"
#include "sails/net/connector_list.h"
#include "sails/log/logging.h"

namespace sails {

extern sails::log::Logger serverlog;

}


namespace sails {
namespace net {


template <typename T> class EpollServer;


// 定义数据队列中的结构
template <typename T>
struct TagRecvData {
  uint32_t        uid;           // 连接标示
  T*              data;          // 接收到的内容
  std::string     ip;            // 远程连接的ip
  uint16_t        port;          // 远程连接的端口
  int             fd;
  uint32_t        extId;         // 补充参数
};

struct TagSendData {
  char            cmd;            // 命令:'c',关闭fd; 's',有数据需要发送
  uint32_t        uid;            // 连接标示
  std::string     buffer;         // 需要发送的内容
  std::string     ip;             // 远程连接的ip
  uint16_t        port;           // 远程连接的端口
};

template <typename T>
using recv_queue = base::ThreadQueue<TagRecvData<T>*,
                                     std::deque<TagRecvData<T>*>>;
typedef base::ThreadQueue<TagSendData*,
                          std::deque<TagSendData*>> send_queue;


template <typename T>
class NetThread {
 public:
  // 链接状态
  struct ConnStatus {
    std::string     ip;
    int32_t         uid;
    uint16_t        port;
  };

  // 网络线程运行状态
  enum RunStatus {
    RUNING,
    PAUSE,
    STOPING
  };

  // 状态
  struct ThreadStatus {
    int thread_num;
    RunStatus status;
    long run_time;
    int listen_port;	// only for recv accept
    int connector_num;	
  };

  // 监听端口信息
  struct BindSocketInfo {
    int listen_port;
    long accept_times;
  };

  explicit NetThread(EpollServer<T> *server);
  virtual ~NetThread();

  // 创建一个epoll的事件循环
  void create_event_loop();

  base::EventLoop* get_event_loop() { return this->ev_loop; }

  // 监听端口
  int bind(int port);

  // 创建一个connector超时管理器
  void setEmptyConnTimeout(int connector_read_timeout = 10);

  static void timeoutCb(net::Connector* connector);

  static void startEvLoop(NetThread<T>* netThread);

  void run();

  void terminate();

  bool isTerminate() { return status == STOPING;}

  void join();

  // 接收连接请求
  static void accept_socket_cb(base::event* e, int revents, void* owner);
  // 增加connector
  void add_connector(std::shared_ptr<net::Connector> connector);
  // 接收连接数据
  static void read_data_cb(base::event* e, int revents, void* owner);

  static void read_pipe_cb(base::event* e, int revents, void* owner);

  // 获取连接数
  size_t get_connector_count() { return connector_list.size();}


  // 接收队列大小
  size_t get_recvqueue_size();

  // 发送队列大小
  size_t get_sendqueue_size();

  // 用于io线程自身解析完之后
  void addRecvList(TagRecvData<T> *data);

  // 用于dispacher线程
  void getRecvData(TagRecvData<T>* &data, int millisecond);  // NOLINT'

  // 发送数据,把data放入一个send list中,然后再触发epoll的可写事件
  void send(const std::string &ip,
            uint16_t port, int uid,
            const std::string &data);

  // 关闭连接
  void close_connector(const std::string &ip, uint16_t port, int uid, int fd);

  // 获取服务
  EpollServer<T>* getServer();

 protected:
  void accept_socket(base::event* e, int revents);

  void read_data(base::event* e, int revents);

 private:
  EpollServer<T> *server;
  // 接收的数据队列
  recv_queue<T> recvlist;

  // 发送的数据队列
  send_queue sendlist;

  int status;
  std::thread *thread;
  int listenfd;
  int listen_port;

  base::EventLoop *ev_loop;  // 事件循环

  ConnectorTimeout* connect_timer;  // 连接超时器
  std::list<base::Timer*> timerList;  // 定时器

  ConnectorList connector_list;

  int shutdown;  // 管道(用于关闭服务)
  int notify;  // 管道(用于通知有数据要发送)
};




template <typename T>
NetThread<T>::NetThread(EpollServer<T> *server) {
  this->server = server;
  status = NetThread::STOPING;
  thread = NULL;
  listenfd = 0;
  listen_port = 0;
  ev_loop = NULL;
  connector_list.init(10000);
  connect_timer = NULL;
  notify = socket(AF_INET, SOCK_STREAM, 0);
}


template <typename T>
NetThread<T>::~NetThread() {
  if (status != NetThread::STOPING) {
    this->terminate();
    this->join();
    delete thread;
    thread = NULL;
  }
  if (connect_timer != NULL) {
    delete connect_timer;
    connect_timer = NULL;
  }

  if (ev_loop != NULL) {
    delete ev_loop;
    ev_loop = NULL;
  }

  // 删除sendlist中没有处理的数据
  bool hasData = false;
  do {
    hasData = false;
    TagSendData* data = NULL;
    sendlist.pop_front(data, 0);
    if (data != NULL) {
      hasData = true;
      delete data;
    }
  }while(hasData);

  // 删除recvlist中的数据
  bool hasRecvData = false;
  do {
    hasRecvData = false;
    TagRecvData<T>* data = NULL;
    getRecvData(data, 0);
    if (data != NULL) {
      hasRecvData = true;
      T* t = data->data;
      if (t != NULL) {
        server->Tdeleter(t);
        t = NULL;
      }
      delete data;
      data = NULL;
    }
  }while(hasRecvData);
}


template <typename T>
void NetThread<T>::create_event_loop() {
  ev_loop = new base::EventLoop(this);
  ev_loop->init();

  // 创建 notify 事件
  sails::base::event notify_ev;
  emptyEvent(&notify_ev);
  notify_ev.fd = notify;
  notify_ev.events = sails::base::EventLoop::Event_READ;
  notify_ev.cb = NetThread<T>::read_pipe_cb;

  assert(ev_loop->event_ctl(base::EventLoop::EVENT_CTL_ADD,
                            &notify_ev));
}

template <typename T>
int NetThread<T>::bind(int port) {
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("create listen socket");
    exit(EXIT_FAILURE);
  }
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  int flag = 1, len = sizeof(int);  // for can restart right now
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  ::bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
  sails::base::setnonblocking(listenfd);

  if (listen(listenfd, 10) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  sails::base::event listen_ev;
  emptyEvent(&listen_ev);
  listen_ev.fd = listenfd;
  listen_ev.events = sails::base::EventLoop::Event_READ;
  listen_ev.cb = NetThread<T>::accept_socket_cb;

  if (!ev_loop->event_ctl(sails::base::EventLoop::EVENT_CTL_ADD,
                         &listen_ev)) {
    fprintf(stderr, "add listen fd to event loop fail");
    exit(EXIT_FAILURE);
  }

  return listenfd;
}

template <typename T>
void NetThread<T>::accept_socket_cb(base::event* e, int revents, void* owner) {
  if (owner != NULL) {
    NetThread<T>* net_thread = (NetThread<T>*)owner;
    net_thread->accept_socket(e, revents);
  }
}


template <typename T>
void NetThread<T>::accept_socket(base::event* e, int revents) {
  if (revents & base::EventLoop::Event_READ) {
    struct sockaddr_in local;
    int addrlen = sizeof(struct sockaddr_in);

    // 循环accept,因为使用的是epoll,当多个连接同时连上时,只通知一次
    for (;;) {
      memset(&local, 0, addrlen);

      int connfd = -1;
      do {
        connfd = accept(e->fd,
                        (struct sockaddr*)&local,
                        (socklen_t*)&addrlen);  // NOLINT'
      } while ((connfd < 0) && (errno == EINTR));  // 被中断

      if (connfd > 0) {
        sails::base::setnonblocking(connfd);

        // 新建connector
        std::shared_ptr<net::Connector> connector(new net::Connector(connfd));
        uint32_t uid = connector_list.getUniqId();
        connector->setId(uid);
        int port = ntohs(local.sin_port);
        connector->setPort(port);
        char sAddr[20] = {'\0'};
        inet_ntop(AF_INET, &(local.sin_addr), sAddr, 20);
        std::string ip(sAddr);
        connector->setIp(ip);
        connector->setTimeoutCB(NetThread<T>::timeoutCb);

        server->addConnector(connector, connfd);

        server->create_connector_cb(connector);

      } else {
        break;
      }
    }
  }
}



// 增加connector
template <typename T>
void NetThread<T>::add_connector(std::shared_ptr<net::Connector> connector) {
  connector->owner = this;
  connector_list.add(connector);
  connect_timer->update_connector_time(connector);

  // 加入event poll中
  sails::base::event ev;
  emptyEvent(&ev);
  ev.fd = connector->get_connector_fd();
  ev.events = sails::base::EventLoop::Event_READ;
  ev.cb = NetThread<T>::read_data_cb;
  ev.data.u32 = connector->getId();

  if (!ev_loop->event_ctl(base::EventLoop::EVENT_CTL_ADD, &ev)) {
    connector_list.del(connector->getId());
  }
}


template <typename T>
void NetThread<T>::read_data_cb(base::event* e, int revents, void* owner) {
  if (owner != NULL && (revents & base::EventLoop::Event_READ)) {
    NetThread<T>* net_thread = (NetThread<T>*)owner;
    net_thread->read_data(e, revents);
  }
}

template<typename T>
size_t NetThread<T>::get_recvqueue_size() {
  return this->recvlist.size();
}


template<typename T>
size_t NetThread<T>::get_sendqueue_size() {
  return this->sendlist.size();
}

template <typename T>
void NetThread<T>::read_data(base::event* ev, int revents) {
  if (ev == NULL || ev->fd < 0 || revents != base::EventLoop::Event_READ) {
    return;
  }

  uint32_t uid = ev->data.u32;

  std::shared_ptr<net::Connector> connector = connector_list.get(uid);


  if (connector == NULL || connector.use_count() <= 0) {
    return;
  }

  bool readerror = false;

  // read nonblock connfd
  int totalNum = 0;
  while (true) {
    int lasterror = 0;
    int n = 0;
    do {
      n = connector->read();
      lasterror = errno;
    } while ((n == -1) && (lasterror == EINTR));  // read 调用被信号中断

    if (n > 0) {
      totalNum+=n;
      if (totalNum >= 4096) {  // 大于4k就开始解析,防止数据过多
        this->server->parseImp(connector);
      }
      if (n < READBYTES) {  // no data
        break;
      } else {
        continue;
      }
    } else if (n == 0) {
      // client close or shutdown send, and there is no error,
      // errno will not reset, so don't print errno
      readerror = true;
      char errormsg[100];
      memset(errormsg, '\0', 100);
      sprintf(errormsg, "read connfd %d, return:%d",  // NOLINT'
              connector->get_connector_fd(), n);
      serverlog.warn(errormsg);
      perror(errormsg);
      break;
    } else if (n == -1) {
      if (lasterror == EAGAIN || lasterror == EWOULDBLOCK) {  // 没有数据
        break;
      } else {  // read fault
        readerror = true;
        char errormsg[100];
        memset(errormsg, '\0', 100);


        sprintf(errormsg, "read connfd %d, return:%d, errno:%d",  // NOLINT'
                connector->get_connector_fd(), n, lasterror);
        serverlog.warn(errormsg);
        perror(errormsg);
        break;
      }
    }
  }

  if (readerror) {
    // 客户端主动close
    this->server->closed_connect_cb(connector);
  } else {
    connect_timer->update_connector_time(connector);  // update timeout
    this->server->parseImp(connector);
  }
}



template <typename T>
void NetThread<T>::read_pipe_cb(base::event* e, int revents, void* owner) {
  NetThread<T>* net_thread = NULL;
  if (e != NULL && owner != NULL && revents == base::EventLoop::Event_READ) {
    net_thread = (NetThread<T>*)owner;
    if (net_thread == NULL) {
      return;
    }
  } else {
    return;
  }

  TagSendData* data = NULL;
  bool read_more = true;
  do {
    read_more = false;
    data = NULL;
    net_thread->sendlist.pop_front(data, 0);

    if (data != NULL) {
      read_more = true;
      int uid = data->uid;
      std::string ip = data->ip;
      uint16_t port = data->port;

      char cmd = data->cmd;
      if (cmd == 's') {  // 发送数据
        std::shared_ptr<Connector> connector =
            net_thread->connector_list.get(uid);
        if (connector != NULL) {
          // 判断是否是正确的连接
          if (connector->getPort() == port && connector->getIp() == ip) {
            connector->write(data->buffer.c_str(), data->buffer.length());
            connector->send();
          }
        }
      } else if (cmd == 'c') {  // 关闭连接
        std::shared_ptr<Connector> connector
            = net_thread->connector_list.get(uid);
        if (connector != NULL) {
          // 从event loop中删除
          if (connector->getPort() == port && connector->getIp() == ip) {
            net_thread->ev_loop->event_stop(connector->get_connector_fd());
            connector->close();
            connector->data.u64 = 0;
            net_thread->connector_list.del(uid);
            // printf("connect use count :%d\n", connector.use_count());
          }
        }
      }
    }
    if (data != NULL) {
      delete data;
    }
  } while (read_more);
}


template <typename T>
void NetThread<T>::setEmptyConnTimeout(int connector_read_timeout) {
  int timeout = connector_read_timeout > 0?connector_read_timeout:10;
  connect_timer = new ConnectorTimeout(timeout);
  connect_timer->init(ev_loop);
}

template <typename T>
void NetThread<T>::timeoutCb(net::Connector* connector) {
  if (connector != NULL && connector->owner != NULL) {
    NetThread<T>* netThread = (NetThread<T>*)connector->owner;
    netThread->server->connector_timeout_cb(connector);
    netThread->close_connector(connector->getIp(),
                               connector->getPort(),
                               connector->getId(),
                               connector->get_connector_fd());
  }
}

template <typename T>
EpollServer<T>* NetThread<T>::getServer() {
  return this->server;
}

template <typename T>
void NetThread<T>::startEvLoop(NetThread<T>* netThread) {
  netThread->setEmptyConnTimeout();
  netThread->ev_loop->start_loop();
}

template <typename T>
void NetThread<T>::run() {
  thread = new std::thread(startEvLoop, this);
  status = NetThread::RUNING;
}

template <typename T>
void NetThread<T>::terminate() {
  if (thread != NULL) {
    // 向epoll管理的0号连接发一个终止事件,让epoll wait结束,然后再退出
    ev_loop->stop_loop();
  }
}


template <typename T>
void NetThread<T>::addRecvList(TagRecvData<T> *data) {
  if (!recvlist.push_back(data)) {
    // 删除它
    T* t = data->data;
    if (t != NULL) {
      server->Tdeleter(t);
      data->data = NULL;
    }
    delete data;
  }
  server->notify_dispacher();
}

template <typename T>
void NetThread<T>::getRecvData(TagRecvData<T>* &data,  // NOLINT'
                               int millisecond) {
  recvlist.pop_front(data, millisecond);
}

template <typename T>
void NetThread<T>::send(const std::string &ip,
                        uint16_t port, int uid, const std::string &s) {
  TagSendData* data = new TagSendData();
  data->cmd = 's';
  data->uid = uid;
  data->buffer = s;
  data->ip = ip;
  data->port = port;
  if (!sendlist.push_back(data)) {
    delete data;
  }

  // 通知epoll_wait
  sails::base::event notify_ev;
  emptyEvent(&notify_ev);
  notify_ev.fd = notify;
  notify_ev.events = sails::base::EventLoop::Event_WRITE;
  notify_ev.cb = NetThread<T>::read_pipe_cb;
  ev_loop->event_ctl(sails::base::EventLoop::EVENT_CTL_MOD, &notify_ev);
}

template <typename T>
void NetThread<T>::close_connector(const std::string &ip,
                                   uint16_t port, int uid, int fd) {
  //    printf("call close connector\n");
  if (ip.length() == 0 || port <= 0 || uid <= 0 || fd <= 0) {
    return;
  }
  serverlog.debug("call close connector\n");
  TagSendData* data = new TagSendData();
  data->cmd = 'c';
  data->uid = uid;
  data->ip = ip;
  data->port = port;
  while (!sendlist.push_back(data)) {
    // 要保证一定正确加入
    usleep(10000);  // 10ms
  }

  // 通知epoll_wait
  sails::base::event notify_ev;
  emptyEvent(&notify_ev);
  notify_ev.fd = notify;
  notify_ev.events = sails::base::EventLoop::Event_WRITE;
  notify_ev.cb = NetThread<T>::read_pipe_cb;
  ev_loop->event_ctl(sails::base::EventLoop::EVENT_CTL_MOD, &notify_ev);
}





template <typename T>
void NetThread<T>::join() {
  if (thread != NULL) {
    thread->join();
    status = NetThread::STOPING;
    delete thread;
    thread = NULL;
  }
}



}  // namespace net
}  // namespace sails



#endif  // SAILS_NET_NET_THREAD_H_

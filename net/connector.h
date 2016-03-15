// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: connector.h
// Description: 封装socket connector
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:33:36



#ifndef SAILS_NET_CONNECTOR_H_
#define SAILS_NET_CONNECTOR_H_

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string>
#include <thread>
#include <vector>
#include <list>
#include <memory>
#include <condition_variable>
#include <mutex>
#include "sails/base/buffer.h"
#include "sails/base/uncopyable.h"
#include "sails/base/timer.h"


namespace sails {
namespace net {



#define READBYTES 512

class Connector;

typedef void (*TimeoutCB)(Connector* connector);

typedef union ExtData {
  void *ptr;
  uint32_t u32;
  uint64_t u64;
} ExtData;


// 链接状态
struct ConnStatus {
  std::string ip;
  int32_t uid;
  uint16_t port;
  int timeout;
  int iLastRefreshTime;
};

class Connector {
 public:
  explicit Connector(int connect_fd);
  Connector();  // after create, must call connect yourself
  virtual ~Connector();

 private:
  Connector(const Connector&);
  Connector& operator=(const Connector&);

 public:
  bool connect(const char* host, uint16_t port, bool keepalive);
  ssize_t read();
  const char* peek();
  void retrieve(int len);
  uint32_t readable();

  int write(const char* data, int len);
  int send();

  void close();
  bool isClosed();

  void setId(uint32_t id);
  uint32_t getId();

  std::string getIp();
  void setIp(std::string ip);

  int getPort();
  void setPort(int port);

  int get_listen_fd();
  void set_listen_fd(int listen_fd);
  int get_connector_fd();

  void setLastRefreshTime(int time) {
    lastRefreshTime = time;
  }
  int LastRefreshTime() {
    return lastRefreshTime;
  }

  // Lan ip
  static std::string GetLocalAddress();

  // 设置超时，它会调用已经设置的超时回调函数
  void set_timeout();
  bool IsTimeout();

  void setTimeoutCB(TimeoutCB cb);
  bool haveSetTimer();

  // 设置超时时间
  void setTimeout(int timeout) {
    this->timeout = timeout;
  }

  int getTimeout() {
    return timeout;
  }

  // noDelay
  void SetDefaultOpt();

  void *owner;  // 为了当回调时能找到对应的拥有者
  ExtData data;

 protected:
  sails::base::Buffer in_buf;
  sails::base::Buffer out_buf;
  int listen_fd;
  int connect_fd;  // 连接fd
  std::mutex mutex;

 private:
  uint32_t id;
  std::string ip;  // 远程连接的ip
  uint16_t port;  // 远程连接的端口
  // 0:表示客户端主动关闭；1:服务端主动关闭;2:连接超时服务端主动关闭
  int  closeType;
  bool is_closed;  // 是否已经关闭
  bool is_timeout;
  int lastRefreshTime;
  TimeoutCB timeoutCB;
  int timeout;
};

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET_CONNECTOR_H_

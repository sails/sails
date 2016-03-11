// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: connector.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:43:02



#include "sails/net/connector.h"
#include <netinet/tcp.h>
#include <err.h>
#include <netdb.h>
#include "sails/base/string.h"

namespace sails {
namespace net {


Connector::Connector(int conn_fd) {
  id = 0;
  connect_fd = conn_fd;

  this->port = 0;
  this->ip = "";
  this->listen_fd = 0;
  has_set_timer = false;
  is_closed = false;
  is_timeout = false;
  timeoutCB = NULL;
  owner = NULL;
  SetDefaultOpt();
}

Connector::Connector() {
  id = 0;
  this->port = 0;
  this->ip = "";
  listen_fd = 0;
  connect_fd = 0;
  has_set_timer = false;
  is_closed = false;
  is_timeout = false;
  timeoutCB = NULL;
  owner = NULL;
}

Connector::~Connector() {
  if (!is_closed) {
    ::close(connect_fd);
  }
}

bool Connector::connect(const char *host, uint16_t port, bool keepalive) {
  char delim[2] = {'.'};
  std::vector<std::string> hostitem = base::split(host, delim);
  if (hostitem.size() == 4) {  // 简单判断为ip
    struct sockaddr_in serveraddr;
    connect_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (connect_fd == -1) {
      printf("new connect_fd error\n");
      return false;
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(host);
    serveraddr.sin_port = htons(port);


    int ret = ::connect(connect_fd,
                        (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (ret == -1) {
      printf("connect failed\n");
      return false;
    }
    SetDefaultOpt();
    this->ip = std::string(host);
    this->port = port;
    if (keepalive) {
      // new thread to send ping
      // std::thread();
    }
    is_closed = false;
  } else {
    struct addrinfo hints, *res, *res0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char servname[20] = {'\0'};
    snprintf(servname, sizeof(servname), "%d", port);
    int error = getaddrinfo(host, servname, &hints, &res0);
    if (error) {
      errx(1, "getaddrinfo %s", gai_strerror(error));
      /*NOTREACHED*/
    }
    int s = -1;
    const char *cause = NULL;
    for (res = res0; res; res = res->ai_next) {
      s = socket(res->ai_family, res->ai_socktype,
                 res->ai_protocol);
      if (s < 0) {
        cause = "socket";
        continue;
      }
      if (::connect(s, res->ai_addr, res->ai_addrlen) < 0) {
        cause = "connect";
        ::close(s);
        s = -1;
        continue;
      }
      connect_fd = s;
      struct sockaddr_in *serveraddr = (struct sockaddr_in *)res->ai_addr;
      char buf[1000] = {'\0'};
      const char *addr =
          inet_ntop(AF_INET, &serveraddr->sin_addr, buf, INET_ADDRSTRLEN);
      ip = std::string(addr?addr:"unknow");
      this->port = ntohs(serveraddr->sin_port);
      break;  /* okay we got one */
    }
    if (s < 0) {
      err(1, "connect_host:%s", cause);
      return false;
    }
    freeaddrinfo(res0);
    SetDefaultOpt();
    if (keepalive) {
      // new thread to send ping
      // std::thread();
    }
    is_closed = false;
  }
  return true;
}

void Connector::set_timeout() {
  this->is_timeout = true;
  if (timeoutCB != NULL) {
    timeoutCB(this);
  }
}

bool Connector::timeout() {
  return is_timeout;
}

void Connector::setTimeoutCB(TimeoutCB cb) {
  this->timeoutCB = cb;
}

void Connector::setId(uint32_t id) {
  this->id = id;
}
uint32_t Connector::getId() {
  return this->id;
}

std::string Connector::getIp() {
  return ip;
}
void Connector::setIp(std::string ip) {
  this->ip = ip;
}

int Connector::getPort() {
  return port;
}
void Connector::setPort(int port) {
  this->port = port;
}


int Connector::get_listen_fd() {
  return this->listen_fd;
}

void Connector::set_listen_fd(int listen_fd) {
  this->listen_fd = listen_fd;
}

int Connector::get_connector_fd() {
  return this->connect_fd;
}

void Connector::close() {
  if (!is_closed && this->connect_fd > 0) {
    is_closed = true;
    std::unique_lock<std::mutex> locker(this->mutex);
    ::close(this->connect_fd);
  }
}
bool Connector::isClosed() {
  return is_closed;
}

void Connector::setTimerEntry(std::weak_ptr<ConnectorTimerEntry> entry) {
  this->timer_entry = entry;
  this->has_set_timer = true;
}

std::weak_ptr<ConnectorTimerEntry> Connector::getTimerEntry() {
  return this->timer_entry;
}

bool Connector::haveSetTimer() {
  return this->has_set_timer;
}

ssize_t Connector::read() {
  ssize_t read_count = 0;
  if (!is_closed && this->connect_fd > 0) {
    read_count = this->in_buf.read(this->connect_fd, READBYTES);
  }

  return read_count;
}

uint32_t Connector::readable() {
  return in_buf.readable();
}

const char* Connector::peek() {
  return this->in_buf.peek();
}

void Connector::retrieve(int len) {
  return this->in_buf.retrieve(len);
}

int Connector::write(const char* data, int len) {
  int ret = 0;
  if (len > 0 && data != NULL) {
    out_buf.append(data, len);
  }
  return ret;
}

int Connector::send() {
  //    std::unique_lock<std::mutex> locker(this->mutex);
  int ret = 0;
  if (!is_closed && this->connect_fd > 0) {
    int write_able = this->out_buf.readable();
    if (write_able > 0) {
      ret = ::write(this->connect_fd, this->out_buf.peek(), write_able);
      if (ret > 0) {
        this->out_buf.retrieve(ret);
      }
    }
  } else {
    this->out_buf.retrieve_all();
  }
  return ret;
}


// 为了能通过getsockname得到本地地址，这里会连接一个udp的socket
// TCP中调用connect会引起三次握手,client与server建立连结.
// UDP中调用connect内核仅仅把对端ip&port记录下来，不会实际发送数据，
// 所以为了能减小负载，这里用udp
std::string Connector::GetLocalAddress() {
  struct sockaddr_in serveraddr;
  int Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (Socket == -1) {
      printf("new connect_fd error\n");
      return "";
    }
    memset(serveraddr.sin_zero, 0, sizeof(serveraddr.sin_zero));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_LOOPBACK;
    serveraddr.sin_port = htons(4567);

    int ret = ::connect(Socket,
                        (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (ret == -1) {
      printf("connect failed\n");
      return "";
    }
    
    socklen_t len = sizeof(serveraddr);
    if (getsockname(Socket, reinterpret_cast<sockaddr*>(&serveraddr), &len) == -1)
    {
      ::close(Socket);
      return "";
    }
    ::close(Socket);
    in_addr InAddr;
    InAddr.s_addr = serveraddr.sin_addr.s_addr;
    return inet_ntoa(InAddr);
}


void Connector::SetDefaultOpt() {
  // 禁用Nagle算法(小包组合之后发送,这样会出现不可预知的延迟)，提高发送效率
  int noDelay = 1;
  setsockopt(connect_fd, IPPROTO_TCP, TCP_NODELAY,
             reinterpret_cast<char*>(&noDelay), sizeof(int));
}




ConnectorTimerEntry::ConnectorTimerEntry(std::shared_ptr<Connector> connector,
                                         base::EventLoop *ev_loop) {
  this->connector = std::weak_ptr<Connector>(connector);
  this->ev_loop = ev_loop;
}

ConnectorTimerEntry::~ConnectorTimerEntry() {
  if (this->connector.use_count() > 0) {
    // close fd and delete connector
    if (this->connector.use_count() > 0) {
      this->connector.lock()->set_timeout();
    }
  }
}













ConnectorTimeout::ConnectorTimeout(int timeout) {
  timeindex = 0;
  assert(timeout > 0);
  this->timeout = timeout;
  time_wheel = new std::vector<ConnectorTimeout::Bucket*>(timeout);
  for (int i = 0; i < timeout; i++) {
    time_wheel->at(i) = new ConnectorTimeout::Bucket();
  }
  ev_loop = NULL;
  timer = NULL;
}

bool ConnectorTimeout::init(base::EventLoop *ev_loop) {
  timer = new base::Timer(ev_loop, 1);
  timer->init(ConnectorTimeout::timer_callback, this, 5);
  this->ev_loop = ev_loop;
  return true;
}

void ConnectorTimeout::timer_callback(void *data) {
  ConnectorTimeout *timeout = reinterpret_cast<ConnectorTimeout*>(data);
  timeout->process_tick();
}

void ConnectorTimeout::process_tick() {
  timeindex = (timeindex+1)%timeout;
  // empty bucket
  Bucket* bucket = time_wheel->at(timeindex);
  if (bucket != NULL) {
    // printf("clear timerindex:%d\n", timeindex);
    typename std::list<std::shared_ptr<ConnectorTimerEntry>>::iterator iter;
    bucket->entry_list.clear();
  }
}

ConnectorTimeout::~ConnectorTimeout() {
  if (time_wheel != NULL) {
    while (!time_wheel->empty()) {
      Bucket* bucket = time_wheel->back();
      delete bucket;
      time_wheel->pop_back();
    }
    delete time_wheel;
    time_wheel = NULL;
  }
  this->ev_loop = NULL;
  if (timer != NULL) {
    timer->disarms();
    delete timer;
    timer = NULL;
  }
}

void ConnectorTimeout::update_connector_time(
    std::shared_ptr<Connector> connector) {

  if (connector.get() != NULL) {
    int add_index = (timeindex+timeout-1)%timeout;
    if (!connector->haveSetTimer()) {
      std::shared_ptr<ConnectorTimerEntry> shared_entry(
          new ConnectorTimerEntry(connector, ev_loop));
      std::weak_ptr<ConnectorTimerEntry> weak_temp(shared_entry);
      connector->setTimerEntry(weak_temp);
      time_wheel->at(add_index)->entry_list.push_back(shared_entry);

    } else {
      if (!connector->timeout()) {
        time_wheel->at(add_index)->entry_list.push_back(
            connector->getTimerEntry().lock());
      }
    }
  }
}



}  // namespace net
}  // namespace sails

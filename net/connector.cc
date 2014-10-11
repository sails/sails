// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: connector.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:43:02



#include "sails/net/connector.h"

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
}

Connector::Connector() {
  id = 0;
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

bool Connector::connect(const char *ip, uint16_t port, bool keepalive) {
  struct sockaddr_in serveraddr;
  connect_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (connect_fd == -1) {
    printf("new connect_fd error\n");
    return false;
  }
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = inet_addr(ip);
  serveraddr.sin_port = htons(port);


  int ret = ::connect(connect_fd,
                      (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  if (ret == -1) {
    printf("connect failed\n");
    return false;
  }
  this->ip = std::string(ip);
  this->port = port;
  if (keepalive) {
    // new thread to send ping
    // std::thread();
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

int Connector::get_connector_fd() {
  return this->connect_fd;
}

void Connector::close() {
  if (!is_closed && this->connect_fd > 0) {
    // connector.close -> close_cb -> event.stop
    // -> event.stop_cb -> connector.close
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

int Connector::read() {
  //    std::unique_lock<std::mutex> locker(this->mutex);
  int read_count = 0;
  if (!is_closed && this->connect_fd > 0) {
    char tmp[READBYTES] = {'\0'};
    read_count = ::read(this->connect_fd, tmp, READBYTES);
    if (read_count > 0) {
      this->in_buf.append(tmp, read_count);
    }
  }

  return read_count;
}

int Connector::readable() {
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

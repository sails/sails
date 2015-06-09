// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: client.cc
// Description: 性能测试(最大连接数)
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-04-25 19:17:29


#include <signal.h>
#include "sails/net/epoll_server.h"
#include "sails/net/connector.h"
#include "sails/log/logging.h"

typedef struct {
  char msg[20];
} __attribute__((packed)) EchoStruct;

namespace sails {
sails::log::Logger serverlog(sails::log::Logger::LOG_LEVEL_DEBUG,
                             "./log/server.log", sails::log::Logger::SPLIT_DAY);
}


class TestClient : public sails::net::EpollServer<EchoStruct> {
 public:
  TestClient() {
  }
  ~TestClient() {
  }

  EchoStruct* Parse(std::shared_ptr<sails::net::Connector> connector) {
    int read_able = connector->readable();
    if (read_able == 0) {
      return NULL;
    }

    EchoStruct *data =
        reinterpret_cast<EchoStruct*>(malloc(sizeof(EchoStruct)));
    memset(data, '\0', sizeof(EchoStruct));
    strncpy(data->msg, connector->peek(), read_able);
    connector->retrieve(read_able);
    return data;
  }

  void handle(const sails::net::TagRecvData<EchoStruct>& ) {
  }

  void connect() {
    for (int i = 0; i < 60000; i++) {
      std::shared_ptr<sails::net::Connector> connector(
          new sails::net::Connector());
      if (connector->connect("192.168.1.116", 8000, true)) {
        this->AddConnector(connector, connector->get_connector_fd());
        connectorlist.push_back(connector);
        usleep(10000);
      }
    }
  }

  void send() {
    EchoStruct data;
    snprintf(data.msg, sizeof(data.msg), "%s", "hello");
    ssize_t size = connectorlist.size();
    for (ssize_t i = 0; i < size; ++i) {
      std::shared_ptr<sails::net::Connector> connector = connectorlist[i];
      if (connector != NULL) {
        connector->write(reinterpret_cast<char*>(&data), sizeof(data));
        connector->send();
        sleep(1000);
      }
    }
  }

  std::vector<std::shared_ptr<sails::net::Connector>> connectorlist;
};


bool isRun = true;

void sails_signal_handle(int signo, siginfo_t *, void *) {
  switch (signo) {
    case SIGINT:
      {
        isRun = false;
      }
  }
}




int main(int, char *[]) {
  // signal kill
  struct sigaction act;
  act.sa_sigaction = sails_signal_handle;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(SIGINT, &act, NULL) == -1) {
    perror("sigaction error");
    exit(EXIT_FAILURE);
  }

  TestClient client;
  client.Init(8001, 2, 10000, 1, true);

  sleep(2);
  // 开始测试连接
  client.connect();

  while (isRun) {
    client.send();
    // 为了测试最大连接数，发数据的频率要小，并且要修改服务器的超时时间
    sleep(2);
  }

  client.Stop();

  return 0;
}

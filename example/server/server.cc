// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: server.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-13 10:16:55



#include <signal.h>
#include <gperftools/profiler.h>
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


class TestServer : public sails::net::EpollServer<EchoStruct> {
 public:
  TestServer() {
  }
  ~TestServer() {
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

  void handle(const sails::net::TagRecvData<EchoStruct> &recvData) {
    /*
    printf("uid:%u, ip:%s, port:%d, msg:%s\n",
           recvData.uid, recvData.ip.c_str(),
           recvData.port, recvData.data->msg);
    */
    std::string buffer = std::string(recvData.data->msg);
    send(buffer, recvData.ip, recvData.port, recvData.uid, recvData.fd);
    // char *data = recvData.data->msg;
    // send(data, 20, recvData.ip, recvData.port, recvData.uid, recvData.fd);
  }
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

  TestServer server;
  // 注意，如果在测试最大并发数时，这个值要改大一些
  server.Init(9123, 1, 10, 1, false, 2);
  ProfilerStart("server.prof");
  while (isRun) {
    sleep(2);
  }
  ProfilerStop();
  server.Stop();

  return 0;
}

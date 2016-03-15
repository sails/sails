// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: client.cc
// Description: 性能测试(最大tps)
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-04-25 19:17:29


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <thread>  // NOLINT
#include <vector>

int connectserver(const char* ip, int port) {
  struct sockaddr_in serveraddr;
  int connect_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (connect_fd == -1) {
    printf("new connect_fd error\n");
    return -1;
  }
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = inet_addr(ip);
  serveraddr.sin_port = htons(port);


  int ret = connect(connect_fd,
                    (struct sockaddr*)&serveraddr, sizeof(serveraddr));
  if (ret == -1) {
    printf("connect failed\n");
    return -1;
  }
  int noDelay = 1;
  setsockopt(connect_fd, IPPROTO_TCP, TCP_NODELAY,
             reinterpret_cast<char*>(&noDelay), sizeof(noDelay));
  return connect_fd;
}

void test() {
  int fd = connectserver("127.0.0.1", 8000);
    if (fd > 0) {
      char buffer[20] = {"hello, world"};
      char recvbuf[20] = {'\0'};
      int recvNum = 0;
      for (int i = 0; i < 100000; i++) {
        write(fd, buffer, sizeof(buffer));
        int readnum = 0;
        if ((readnum = read(fd, recvbuf, sizeof(recvbuf))) <= 0) {
          printf("on recvNum:%d read num :%d\n", recvNum, readnum);
        } else {
          // printf("recv:%s", recvbuf);
        }
        recvNum++;
      }
      printf("recvnum:%d\n", recvNum);
    }
}
int main(int argc, char* argv[]) {
  int clients = 1;
  if (argc >= 2) {
    clients = atoi(argv[1]);
  }

  std::vector<std::thread> vec_thread;
  for (int i = 0; i < clients; i++) {
    vec_thread.push_back(std::thread(test));
  }
  for (size_t i = 0 ; i < vec_thread.size(); i++) {
    vec_thread[i].join();
  }


  return 0;
}


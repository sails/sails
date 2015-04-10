// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: util.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 09:59:22



#include "sails/base/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

namespace sails {
namespace base {

void setnonblocking(int fd) {
  int opts;
  opts = fcntl(fd, F_GETFL);
  if (opts < 0) {
    perror("fcntl(fd,GETFL)");
    exit(EXIT_FAILURE);
  }
  opts = opts|O_NONBLOCK;
  if (fcntl(fd, F_SETFL, opts) < 0) {
    perror("fcntl(fd,SETFL,opts)");
    exit(EXIT_FAILURE);
  }
}

size_t
readline(int fd, char *vptr, size_t maxlen) {
  size_t n;
  char c, *ptr;

  ptr = vptr;
  for (n = 1; n < maxlen; n++) {
    size_t rc = 0;
    if ((rc = read(fd, &c, 1)) == 1) {
      *ptr++ = c;
      if (c == '\n')
        break;
    } else if (rc == 0) {
      if (n == 1)
        return(0);  // EOF, no data read
      else
        break;  // EOF, some data was read
    } else {
      return(-1);  // error
    }
  }

  *ptr = 0;
  return(n);
}

// 时间毫秒(42) + 节点(12) + 自增长id(10位)
// 时间可以保证100年不重复
// 节点可以用于有多个机器同时产生id时，可以设置不同node
// 每一毫秒能生成最多1024个id,但是由于机器的限制，
// 一毫秒内光调用GetSession也不能达到1000次(按3GHz算，相当于3个指令周期调用一次)
// 所以综合总论100年内不可能有重复
// 0           42     54          64
// +-----------+------+------------+
// |timestamp  |node  |increment   |
// +-----------+------+------------+
uint64_t GetUID(int node) {
  struct timeval t;
  gettimeofday(&t, NULL);
  uint64_t time_num = t.tv_sec*1000+t.tv_usec/1000;
  static int i = 0;
  i++;
  if (i > 1024) {
    i = 0;
  }
  uint64_t value = (time_num << 22) + (node << 10) + i;

  return value;
}


}  // namespace base
}  // namespace sails

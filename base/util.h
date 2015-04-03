// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: util.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 09:59:01



#ifndef SAILS_BASE_UTIL_H_
#define SAILS_BASE_UTIL_H_

#include <stdio.h>
#include <stdint.h>

namespace sails {
namespace base {

void setnonblocking(int fd);

size_t readline(int fd, void *vptr, size_t maxlen);

uint64_t GetUID(int node = 0);

}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_UTIL_H_

// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: mem_usage_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-09-29 16:30:35



 #include <unistd.h>
#include <iostream>
#include "catch.hpp"
#include "sails/system/mem_usage.h"

TEST_CASE("MemUsageTest", "Test") {
  pid_t pid = getpid();
  uint64_t vm_size, mem_size;
  REQUIRE(sails::system::GetMemoryUsedKiloBytes(pid, &vm_size, &mem_size));
  printf("vm size: %lld k\n", vm_size);
  printf("mem size: %lld k\n", mem_size);
  REQUIRE(sails::system::GetMemoryUsedBytes(pid, &vm_size, &mem_size));
  printf("vm size: %lld byte\n", vm_size);
  printf("mem size: %lld byte\n", mem_size);
}

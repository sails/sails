// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: cpu_usage_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-09-29 16:37:45



#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include "sails/system/cpu_usage.h"
#include "catch.hpp"


#define FLAGS_pid 1228
#define FLAGS_sample_period 1000

TEST_CASE("CPU_USAGE", "CpuRate") {
  #ifdef __linux__
  double cpu = 0;
  if (sails::system::GetCpuUsageSinceLastCall(getpid(), &cpu)) {
    std::cout << "current process cpu: " << cpu << std::endl;
  }
  if (sails::system::GetCpuUsageSinceLastCall(FLAGS_pid, &cpu)) {
    std::cout << "cpu usage of process " <<
        FLAGS_pid << ": " << cpu << std::endl;
  }

  if (sails::system::GetProcessCpuUsage(getpid(), FLAGS_sample_period, &cpu)) {
    std::cout << "current process cpu=" << cpu << std::endl;
  }
  if (sails::system::GetProcessCpuUsage(FLAGS_pid, FLAGS_sample_period, &cpu)) {
    std::cout << "cpu usage of process=" <<
        FLAGS_pid << "cpu=" << cpu << std::endl;
  }
  #endif
}


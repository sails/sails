// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: log_test.cc
// Description: 日志测试类
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-08-14 12:02:13


#include "../log/logging.h"
#define CATCH_CONFIG_MAIN  // 这里只用在一个cpp中增加
#include "catch.hpp"


TEST_CASE("LogTest", "[log]") {
  for (int i = 0; i < 100; i++) {
    DEBUG_LOG("test", "test log:%d", 1);
    INFO_LOG("test", "test log:%d", 1);
    WARN_LOG("test", "test log:%d", 1);
    ERROR_LOG("test", "test log:%d", 1);
    sleep(1);
  }
}

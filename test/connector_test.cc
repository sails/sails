// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: connector_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-09-06 13:58:21



#include "catch.hpp"
#include "../net/connector.h"


TEST_CASE("Connector Test", "[connect]") {
  sails::net::Connector connector;
  REQUIRE(connector.connect("220.181.57.217", 80, true));
  printf("ip:%s port:%d\n", connector.getIp().c_str(), connector.getPort());
  REQUIRE(connector.connect("baidu.com", 80, true));
  printf("ip:%s port:%d\n", connector.getIp().c_str(), connector.getPort());

  printf("localIp:%s\n", sails::net::Connector::GetLocalAddress().c_str());
}

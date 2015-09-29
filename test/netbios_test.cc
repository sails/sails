// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: netbios_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-09-29 15:05:13

#include "catch.hpp"
#include "../net/netbios.h"

TEST_CASE("NetBios Test", "[query name]") {
  sails::net::NetbiosQuery query;
  printf("query sigle ip:\n");
  sails::net::NetbiosQueryAns ret;
  if (query.query("192.168.198.1", 30, &ret) == 0) {
    printf("ip:%s,\t name:%s, group:%s, "
           "mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
           ret.ip, ret.name, ret.group, ret.unitId[0], ret.unitId[1],
           ret.unitId[2], ret.unitId[3], ret.unitId[4], ret.unitId[5]);
  }

  printf("query sub net\n");
  std::list<sails::net::NetbiosQueryAns> retlist =
      query.query_subnet("192.168.1.*", 100);
  for (auto &item : retlist) {
    printf("ip:%s,\t name:%s, group:%s, "
           "mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
           item.ip, item.name, item.group, item.unitId[0], item.unitId[1],
           item.unitId[2], item.unitId[3], item.unitId[4], item.unitId[5]);
  }
}

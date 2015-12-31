// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: auto_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-12-30 11:59:49



#include "catch.hpp"
#include "../base/auto_data.h"

TEST_CASE("auto_data", "constructor") {
  // test bool cover
  sails::auto_data cb = true;
  bool b = cb;
  REQUIRE(b);

  // test int conver
  int i = 10;
  sails::auto_data ci = i;
  int vi = ci;
  REQUIRE(vi == i);

  // test int64_t cover
  int64_t i64 = 10;
  sails::auto_data ci64 = i64;
  int64_t vi64 = ci64;
  REQUIRE(vi64 == i64);

  // test float cover
  sails::auto_data cd = 0.1;
  double d = cd;
  REQUIRE(d == 0.1);

  // test string cover
  sails::auto_data cs = "test";
  std::string s = cs;
  REQUIRE(s == "test");
}

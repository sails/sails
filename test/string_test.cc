// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: string_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-12-10 18:51:01


#include "catch.hpp"
#include <stdio.h>
#include <string>
#include "../base/string.h"

TEST_CASE("StringURLEncode", "encode") {
  std::string data = "AGIDO1M8Aj9WZVwxVGMPAFJgATxVYAVmUmYHNVNiBjUEYFNiUjELYQRkATdSYgJmUmFUZFEwVjdRZ1dtU2MCYgAzAzpTPAI/VmVcP1RgDzlSXwE5VTUFNlIxBzFTNgYzBGJTZ1Iw";
  char encode_str[1000] = {"\0"};
  char *en = sails::base::url_encode(data.c_str(),
                                     encode_str, sizeof(encode_str));
  REQUIRE(en != NULL);
  if (en != NULL) {
    printf("encode:%s\n", en);
  }
  
}

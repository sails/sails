// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: json_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-12-10 16:59:16

#include <iostream>
#include <stdexcept>
#include "catch.hpp"
#include "../base/json.hpp"

TEST_CASE("JsonTest1", "right") {
  //  auto j3 = nlohmann::json::parse("{ \"happy\": true, \"pi\": 3.141 }");
  std::string test = "{\"name\":\"xu\"}";
  nlohmann::json root =  nlohmann::json::parse(test);
  std::string name = root["name"];
  std::cout << name << std::endl;
}


TEST_CASE("JsonTest2", "wrong") {
  std::string test = "{";
  try {
    nlohmann::json root =  nlohmann::json::parse(test);
    std::string name = root["name"];
    std::cout << name << std::endl;
  } catch (std::invalid_argument& e) {
    std::cout << e.what() << std::endl;
  }
}


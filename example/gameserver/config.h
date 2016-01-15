// Copyright (C) 2016 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: config.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2016-01-15 15:41:47



#ifndef EXAMPLE_GAMESERVER_CONFIG_H_
#define EXAMPLE_GAMESERVER_CONFIG_H_

#include <map>
#include <string>
#include "sails/base/json.hpp"

using json = nlohmann::json;

namespace sails {

// parser configure file

class Config {
 public:
  explicit Config(std::string file);
  int get_listen_port();
  int get_max_connfd();
  int get_handle_thread_pool();
  int get_handle_request_queue_size();

  std::string get_store_api_url();
  std::string get_local_ip();  // for check room
  std::string get_game_code();
 private:
  json root;
};

}  // namespace sails

#endif  // EXAMPLE_GAMESERVER_CONFIG_H_
















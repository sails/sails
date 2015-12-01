#ifndef _CONFIG_H_
#define _CONFIG_H_

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

#endif  // _CONFIG_H_
















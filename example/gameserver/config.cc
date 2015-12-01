#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <fstream>

namespace sails {

Config::Config(std::string file) {
  std::ifstream ifs;
  ifs.open(file);
  if (!ifs) {
    printf("open file failed\n");
    exit(0);
  }
  // get length of file:
  ifs.seekg(0, ifs.end);
  int length = ifs.tellg();
  ifs.seekg(0, ifs.beg);
 
  std::string str;
  str.resize(length, ' '); // reserve space
  char* begin = &*str.begin();
  
  ifs.read(begin, length);
  ifs.close();
  root = json::parse(str);
}

int Config::get_listen_port() {
  return root["listen_port"];
}

int Config::get_max_connfd() {
  if (root["max_connfd"].empty()) {
    return 2000;
  }
  return root["max_connfd"];
}

int Config::get_handle_thread_pool() {
  if (root["handle_thread_pool"].empty()) {
    int64_t processor_num = sysconf(_SC_NPROCESSORS_CONF);
    if (processor_num < 0) {
      return 2;
    }
    return processor_num;
  }
  return root["handle_thread_pool"];
}

int Config::get_handle_request_queue_size() {
  if (root["handle_request_queue_size"].empty()) {
    return 1000;
  }
  return root["handle_request_queue_size"];
}



std::string Config::get_store_api_url() {
  if (root["store_api_url"].empty()) {
    return "127.0.0.1:9000";
  }
  return root["store_api_url"];
}


std::string Config::get_local_ip() {
  if (root["local_ip"].empty()) {
    return "127.0.0.1";
  }
  return root["local_ip"];
}

std::string Config::get_game_code() {
  if (root["game_code"].empty()) {
    return std::string("");
  }
  return root["game_code"];
}

}  // namespace sails

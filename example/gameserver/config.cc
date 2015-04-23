#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <fstream>

namespace sails {

Config::Config(std::string file) {
  std::ifstream ifs;
  ifs.open(file);
  Json::Reader reader;
  if (!ifs) {
    printf("open file failed\n");
    exit(0);
  }
  if (!reader.parse(ifs, root)) {
    printf("parser failed\n");
    exit(0);
  }
  ifs.close();
}

int Config::get_listen_port() {
  return root["listen_port"].asInt();
}

int Config::get_max_connfd() {
  if (root["max_connfd"].empty()) {
    return 2000;
  }
  return root["max_connfd"].asInt();
}

int Config::get_handle_thread_pool() {
  if (root["handle_thread_pool"].empty()) {
    int64_t processor_num = sysconf(_SC_NPROCESSORS_CONF);
    if (processor_num < 0) {
      return 2;
    }
    return processor_num;
  }
  return root["handle_thread_pool"].asInt();
}

int Config::get_handle_request_queue_size() {
  if (root["handle_request_queue_size"].empty()) {
    return 1000;
  }
  return root["handle_request_queue_size"].asInt();
}



std::string Config::get_store_api_url() {
  if (root["store_api_url"].empty()) {
    return "127.0.0.1:9000";
  }
  return root["store_api_url"].asString();
}


std::string Config::get_local_ip() {
  if (root["local_ip"].empty()) {
    return "127.0.0.1";
  }
  return root["local_ip"].asString();
}

std::string Config::get_game_code() {
  if (root["game_code"].empty()) {
    return std::string("");
  }
  return root["game_code"].asString();
}

}  // namespace sails

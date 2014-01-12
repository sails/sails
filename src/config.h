#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <map>
#include <string>
#include <json/json.h>

namespace sails {

// parser configure file

class Config
{
public:
     Config();
     std::map<std::string, std::string> get_modules();
     int get_listen_port();
     int get_max_connfd();

private:
     Json::Value root;
};

} // namespace sails

#endif /* _CONFIG_H_ */
















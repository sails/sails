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
	std::map<std::string, std::string> get_modules();
};

} // namespace sails

#endif /* _CONFIG_H_ */
















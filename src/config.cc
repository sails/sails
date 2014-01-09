#include "config.h"
#include <fstream>

using namespace std;

namespace sails {

Config::Config() {

	std::ifstream ifs;
	ifs.open("../conf/sails.json");
	Json::Reader reader;
	if(!ifs) {
		printf("open file failed\n");
		exit(0);
	}
	if(!reader.parse(ifs, root)) {
		printf("parser failed\n");
		exit(0);
	}
	ifs.close();
}

map<string, string> Config::get_modules()
{
	map<string, string> retmap;

	int module_size = root["modules"].size();
	if(module_size >  0) {
		for(int i = 0; i < module_size; i++) {
			string name = root["modules"][i]["name"].asString();
			string value = root["modules"][i]["path"].asString();
			if(!name.empty() && !value.empty()) {
				retmap.insert(pair<string, string>(name, value));
			}
		}
	}
	return retmap;
}

int Config::get_listen_port()
{
	return root["listen_port"].asInt();
}

} // namespace sails


















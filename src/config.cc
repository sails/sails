#include "config.h"
#include <fstream>

using namespace std;

namespace sails {

map<string, string> Config::get_modules()
{
	map<string, string> retmap;
	std::ifstream ifs;
	ifs.open("../conf/sails.json");
	Json::Reader reader;
	Json::Value root;
	if(!ifs) {
		printf("open file failed\n");
	}
	if(!reader.parse(ifs, root)) {
		printf("parser failed\n");
		return retmap;
	}
	int module_size = root["modules"].size();
	if(module_size >  0) {
		printf("size:%d\n", module_size);
		for(int i = 0; i < module_size; i++) {
			string name = root["modules"][i]["name"].asString();
			string value = root["modules"][i]["path"].asString();
			if(!name.empty() && !value.empty()) {
				retmap.insert(pair<string, string>(name, value));
			}
		}
	}
	ifs.close();
}

} // namespace sails


















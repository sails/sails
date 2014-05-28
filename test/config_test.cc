#include <gtest/gtest.h>
#include <stdio.h>
#include "config.h"

using namespace sails; 
using namespace std;
TEST(config_test, get_modules) {
	Config config;
	map<string, string> modules;
	config.get_modules(&modules);
	map<string, string>::iterator iter;
	for(iter = modules.begin(); iter != modules.end(); iter++) {
		cout << iter->first << ":" << iter->second << endl;
	}
}


int main(int argc, char *argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}













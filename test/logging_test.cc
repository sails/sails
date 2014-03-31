#include <common/log/logging.h>
#include <stdio.h>
#include <gtest/gtest.h>

using namespace sails::common;

TEST(logging_test, logger)
{
    printf("init log with debug mode, filename /home/....../test.log \n");
    log::Logger log(log::Logger::LOG_LEVEL_DEBUG, "/home/sails/workspace/sails/test/test.log");
    log.debug("test %s", "debug is ok");
    log.info("test %s", "info is ok");
    log.warn("test %s", "warn is ok");
    log.error("test %s", "error is ok");

    printf("init log with info mode, to console !\n");
    log::Logger log1(log::Logger::LOG_LEVEL_INFO);
    log1.debug("test %s", "debug is ok");
    log1.info("test %s", "info is ok");
    log1.warn("test %s", "warn is ok");
    log1.error("test %s", "error is ok");

    printf("init log with error mode, filename ./test.log !\n");
    log::Logger log3(log::Logger::LOG_LEVEL_ERROR, "./test1.log", log::Logger::SPLIT_HOUR);
    log3.debug("test %s", "debug is ok");
    log3.info("test %s", "info is ok");
    log3.warn("test %s", "warn is ok");
    log3.error("test %s", "error is ok");


/*
    printf("start test update infor from log.conf\n");
    	log::Logger log4(log::Logger::LOG_LEVEL_ERROR, "./test2.log", log::Logger::SPLIT_HOUR);
    while(1) {	
	log4.debug("test %s", "debug is ok");
	log4.info("test %s", "info is ok");
	log4.warn("test %s", "warn is ok");
	log4.error("test %s", "error is ok");
	sleep(1);
    }
*/

    printf("test factory\n");
    log::Logger* log5 = log::LoggerFactory::getLog(std::string("testfactory"));
    log5->debug("test %s", "debug is ok");
    log5->info("test %s", "info is ok");
    log5->warn("test %s", "warn is ok");
    log5->error("test %s", "error is ok");

//    printf("test factory\n");
    log::Logger* log6 = log::LoggerFactory::getLog(std::string("testfactory"));
    log6->debug("test %s", "debug is ok");
    log6->info("test %s", "info is ok");
    log6->warn("test %s", "warn is ok");
    log6->error("test %s", "error is ok");



}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}










#include <common/log/logging.h>
#include <stdio.h>
#include <gtest/gtest.h>

using namespace sails::common;

TEST(logging_test, logger)
{
    printf("init log with debug mode, filename /home/....../test.log \n");
    log::Logger log(log::Logger::DEBUG, "/home/sails/workspace/sails/test/test.log");
    log.debug("test %s", "debug is ok");
    log.info("test %s", "info is ok");
    log.warn("test %s", "warn is ok");
    log.error("test %s", "error is ok");

    printf("init log with info mode, to console !\n");
    log::Logger log1(log::Logger::INFO);
    log1.debug("test %s", "debug is ok");
    log1.info("test %s", "info is ok");
    log1.warn("test %s", "warn is ok");
    log1.error("test %s", "error is ok");

    printf("init log with error mode, filename ./test.log !\n");
    log::Logger log3(log::Logger::ERROR, "./test11.log", log::Logger::SPLIT_HOUR);
    log3.debug("test %s", "debug is ok");
    log3.info("test %s", "info is ok");
    log3.warn("test %s", "warn is ok");
    log3.error("test %s", "error is ok");
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}










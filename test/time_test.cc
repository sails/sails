#include <common/base/time_t.h>
#include <stdio.h>
#include <gtest/gtest.h>

using namespace sails;

TEST(time_test, time_with_millisecond) {
    char time[100];
    sails::common::TimeT::time_with_millisecond(time, 100);
    printf("%s\n", time);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}


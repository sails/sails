#include <common/base/timer.h>
#include <stdio.h>
#include <gtest/gtest.h>

using namespace sails::common;


TEST(timer_interface, run_stop)
{
    EventLoop ev_loop;
    ev_loop.init();
    Timer *timer = new HeapTimer(&ev_loop, 3);
    timer->init();
    ev_loop.start_loop();
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}

#include <common/base/timer.h>
#include <stdio.h>
#include <gtest/gtest.h>

using namespace sails::common;


void timer_test_cb(void *data) {
    Timer *timer = (Timer*)data;
    printf("timer test cb \n");
    timer->disarms();
}

void timer_test_cb1(void *data) {
    printf("cb ok, %s\n", (char *)data);
}

TEST(timer_interface, run_stop)
{
    EventLoop ev_loop;
    ev_loop.init();
 

    
    Timer timer(&ev_loop, 3);
    timer.init(timer_test_cb, &timer, 1);

    char data2[] = {"test2"};
    Timer timer1(&ev_loop, 4);
    timer1.init(timer_test_cb1, data2, 5);
//    timer1.disarms();

    ev_loop.start_loop();

    
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}

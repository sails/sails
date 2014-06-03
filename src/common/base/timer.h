#ifndef _TIMER_H_
#define _TIMER_H_

#include <common/base/event_loop.h>
#include <sys/timerfd.h>

namespace sails {
namespace common {

typedef void (*ExpiryAction)(void *data);

class Timer {
public:
    // set tick to zero, the timer expires just once
    Timer(EventLoop *ev_loop,  int tick = 1);
    ~Timer();
    // set when to zero disarms the timer
    bool init(ExpiryAction action, void *data, int when);
    bool disarms();
public:

    static void read_timerfd_data(common::event*, int revents);
    void pertick_processing();
private:
    int tick;
    int timerfd;
    struct itimerspec *new_value;
    EventLoop *ev_loop;
    ExpiryAction action;
    void *data;
    char temp_data[50];
};


} // namespace common
} // namespace sails 

#endif /* _TIMER_H_ */

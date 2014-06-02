#ifndef _TIMER_H_
#define _TIMER_H_

#include <common/base/event_loop.h>
#include <sys/timerfd.h>

namespace sails {
namespace common {

typedef void ExpiryAction(void *data);

class Timer {
public:
    Timer(EventLoop *ev_loop, int tick = 1);
    ~Timer();
    bool init();
public:
    virtual void add_timer(int interval, int& timerId, ExpiryAction expiry_action) = 0;
    
    virtual void delete_timer(int timerId) = 0;
    
    virtual void pertick_processing() = 0;

    static void read_timerfd_data(common::event*, int revents);
private:
    int tick;
    int timerfd;
    struct itimerspec *new_value;
    EventLoop *ev_loop;
    char temp_data[50];
};



class HeapTimer : public Timer {
public:
    HeapTimer(EventLoop *ev_loop, int tick = 1);
    
    void add_timer(int interval, int& timerId, ExpiryAction expiry_action);
    
    void delete_timer(int timerId);
    
    void pertick_processing();

};

} // namespace common
} // namespace sails 

#endif /* _TIMER_H_ */

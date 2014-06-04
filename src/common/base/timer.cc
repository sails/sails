#include <common/base/timer.h>
#include <common/base/event_loop.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

namespace sails {
namespace common {

Timer::Timer(EventLoop *ev_loop, int tick) {
    this->tick = tick;
    this->ev_loop = ev_loop;
    this->data = NULL;
}

bool Timer::init(ExpiryAction action, void *data, int when=1) {

    timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);

    new_value = (struct itimerspec*)malloc(sizeof(struct itimerspec));
    new_value->it_interval.tv_sec = tick;
    new_value->it_interval.tv_nsec = 0;
    new_value->it_value.tv_sec = when;
    new_value->it_value.tv_nsec = 0;
    timerfd_settime(timerfd, 0, new_value, NULL);

    sails::common::event ev;
    ev.fd = timerfd;
    ev.events = sails::common::EventLoop::Event_READ;
    ev.cb = sails::common::Timer::read_timerfd_data;
    
    ev.data = this;
    ev.next = NULL;

    if(!ev_loop->event_ctl(common::EventLoop::EVENT_CTL_ADD, &ev)){
	printf("timerfd:%d\n", timerfd);
	close(timerfd);
	timerfd = 0;
	return false;
    }
    this->action = action;
    this->data = data;
    return true;
}

bool Timer::disarms() 
{
    if(timerfd > 0) {
	close(timerfd);
	timerfd = 0;
    }
}

void Timer::read_timerfd_data(common::event* ev, int revents)
{
    Timer *timer = (Timer*)ev->data;
    if(timer != NULL) {
	memset(timer->temp_data, '\0', 50);
	int n = read(ev->fd, timer->temp_data, sizeof(uint64_t));
	if(n != sizeof(uint64_t)) {
	    return;
	}

	timer->pertick_processing();
    }
}


Timer::~Timer() {
    if(timerfd > 0) {
	close(timerfd);
	timerfd = 0;
    }
    ev_loop = NULL;
    action = NULL;
    if(new_value != NULL) {
	free(new_value);
    }
}
    
void Timer::pertick_processing()
{
    (*action)(this->data);
}



} // namespace common
} // namespace sails











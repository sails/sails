#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_
#include <common/base/uncopyable.h>
#include <sys/epoll.h>

namespace sails {
namespace common {
namespace net {

typedef void (*event_cb)(struct event*, int revents);

struct event {
    int fd;
    int events;
    event_cb cb;
    struct event* next;
};

struct ANFD {
    int isused;
    int fd;
    int events;
    struct event* next;
};


class EventLoop : private Uncopyable{
public:
    static const int INIT_EVENTS = 1000;
    enum OperatorType {
	EVENT_CTL_ADD = 1,
	EVENT_CTL_DEL
    };
    enum Events {
	Event_READ = 1,
	Event_WRITE = 2
    };

    EventLoop();
    ~EventLoop();
    
    void init();
    bool event_ctl(OperatorType op, struct event*);
    bool event_stop(int fd);
    void start_loop();
private:
    bool add_event(struct event*);
    bool delete_event(struct event*);
    void process_event(int fd, int events);
    void array_needsize(int need_cnt);
    void init_events(int start, int count);
    int epollfd;
    struct epoll_event *events;
    struct ANFD *anfds;
    int max_events;
};

} // namespace net
} // namespace common
} // namespace sails

#endif /* _EVENT_LOOP_H_ */












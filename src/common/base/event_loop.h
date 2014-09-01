#ifndef _EVENT_LOOP_H_
#define _EVENT_LOOP_H_
#include <common/base/uncopyable.h>
#include <sys/epoll.h>

namespace sails {
namespace common {

typedef void (*event_cb)(struct event*, int revents, void *owner);
typedef void (*STOPEVENT_CB)(struct event*, void *owner);

typedef union Event_Data {
    void *ptr;
    uint32_t u32;
    uint64_t u64;
} Event_Data;

struct event {
    int fd;
    int events;
    event_cb cb;
    Event_Data data;
    struct event* next;
    STOPEVENT_CB stop_cb;
};

void emptyEvent(struct event& ev);

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
	EVENT_CTL_DEL = 2
    };
    enum Events {
	Event_READ = 1,
	Event_WRITE = 2
    };
    enum EventsData {
	
    };

    EventLoop(void *owner);
    ~EventLoop();
    
    void init();
    bool event_ctl(OperatorType op, struct event*);
    bool event_stop(int fd);
    void start_loop();
    void stop_loop();
    void delete_all_event();
private:
    void *owner;
    bool add_event(struct event*);
    bool delete_event(struct event*);
    bool mod_event(struct event*);
    void process_event(int fd, int events);
    bool array_needsize(int need_cnt);
    void init_events(int start, int count);
    int epollfd;
    struct epoll_event *events;
    struct ANFD *anfds;
    int max_events;

    bool stop;
    int shutdownfd = 0;
};

} // namespace common
} // namespace sails

#endif /* _EVENT_LOOP_H_ */












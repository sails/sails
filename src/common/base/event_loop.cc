#include <common/base/event_loop.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

namespace sails {
namespace common {

//const int EventLoop::INIT_EVENTS = 1000;

EventLoop::EventLoop() {
    events = (struct epoll_event*)malloc(sizeof(struct epoll_event)
					 *INIT_EVENTS);
    anfds = (struct ANFD*)malloc(sizeof(struct ANFD)
					 *INIT_EVENTS);
    memset(anfds, 0, 1000*sizeof(struct ANFD));
    max_events = INIT_EVENTS;
}

EventLoop::~EventLoop() {
    if(events != NULL) {
	for(int i = 0; i < max_events; i++) {
	    event_stop(i);
	}
	free(events);
	events = NULL;
    }
    if(anfds != NULL) {
	free(anfds);
    }
}

void EventLoop::init() {
    
    epollfd = epoll_create(10);
    assert(epollfd > 0);
    for(int i = 0; i < INIT_EVENTS; i++) {
	anfds[i].isused = 0;
	anfds[i].fd = 0;
	anfds[i].events = 0;
	anfds[i].next = NULL;
    }
}
bool EventLoop::add_event(struct event*ev) {
    if(ev->fd >= max_events) {
	// malloc new events and anfds
	if(!array_needsize(ev->fd+1)) {
	    return false;
	}
    }

    struct event *e = (struct event*)malloc(sizeof(struct event));
    e->fd = ev->fd;
    e->events = ev->events;
    e->cb = ev->cb;
    e->data = ev->data;
    e->next = NULL;

    int fd = e->fd;

    int need_add_to_epoll = false;
    if(anfds[fd].isused == 1) {
	if((anfds[fd].events | e->events) != anfds[fd].events) {
	    need_add_to_epoll = true;
	    struct event *t_e = anfds[fd].next;
	    while(t_e->next != NULL) {
		t_e = t_e->next;
	    }
	    t_e->next = e;
	}
    }else {
	anfds[fd].isused = 1;
	need_add_to_epoll = true;
	anfds[fd].next = e;
    }

    if(need_add_to_epoll) {
	struct epoll_event epoll_ev = {};
	short events = 0;
	epoll_ev.data.fd = ev->fd;
	if(ev->events & Event_READ) {
	    events = events | EPOLLIN | EPOLLET;
	}
	if(ev->events & Event_WRITE) {
	    events = events | EPOLLOUT;
	}
	epoll_ev.events = events;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, ev->fd, &epoll_ev) == -1) {
	    perror("epoll_ctl");
	    return false;
	}
    }

    struct event *temp = anfds[fd].next;
    int new_events = 0;
    while(temp != NULL) {
	new_events |= temp->events;
	temp = temp->next;
    }
    anfds[fd].events = new_events;

    return true;
}

bool EventLoop::delete_event(struct event* ev) {
    int fd = ev->fd;
    if(fd < 0 || fd > max_events) {
	return false;
    }
    if(anfds[fd].isused == 1) {
	if((anfds[fd].events & ev->events) == 0) {
	    return true; // not contain ev.events
	}
	struct event* pre = anfds[fd].next;
	struct event* cur = pre->next;
	while(cur != NULL) {
	    int isdelete = 0;
	    if((cur->events & ev->events) > 0) {
		cur->events = cur->events^ev->events;
		if(cur->events == 0) {
		    //can delete it from the list
		    isdelete = 1;
		    cur = cur->next;
		    free(pre->next);
		    pre->next = cur;
		}
	    }
	    if(!isdelete) {
		pre = cur;
		cur = cur->next;
	    }
	}

	if(anfds[fd].events & ev->events) {
	    anfds[fd].events = anfds[fd].events^ev->events;
		if(anfds[fd].events == 0) {
		    //can delete it from the list
		    struct event* temp = anfds[fd].next->next;
		    free(anfds[fd].next);
		    anfds[fd].next = temp;
		}
	}
    }
    return true;
}

bool EventLoop::event_stop(int fd) {
    if(anfds[fd].isused == 1) {
	anfds[fd].isused = 0;
	// detele event list
	struct event* cur = anfds[fd].next;
	struct event* pre = NULL;
	while(cur != NULL) {
	    pre = cur;
	    cur = cur->next;
	    free(pre);
	    pre = NULL;
	}
    }
    return true;
}


bool EventLoop::event_ctl(OperatorType op, struct event* ev) {
    if(op == EventLoop::EVENT_CTL_ADD) {
	return this->add_event(ev);
    }else if(op == EventLoop::EVENT_CTL_DEL) {
	return this->delete_event(ev);
    }
    
    return true;
}

void EventLoop::start_loop() {
    for(;;) {
	int nfds = epoll_wait(epollfd, events, max_events, -1);
	if(nfds == -1) {
	    perror("start_loop, epoll wait");
	    exit(EXIT_FAILURE);
	}
	for(int n = 0; n < nfds; ++n) {
	    int fd = events[n].data.fd;
	    if(anfds[fd].isused == 1) {
		// find events for fd and callback
		int ev = 0;
		if(events[n].events & EPOLLIN) {
		    ev |= Event_READ;
		}
		if(events[n].events & EPOLLOUT) {
		    ev |= Event_WRITE;
		}
		process_event(fd, ev);
	    }
	}
    }
}

void EventLoop::process_event(int fd, int events) {
    if(fd < 0 || fd > max_events) {
	return;
    }
    if(anfds[fd].isused != 1) {
	return;
    }
    if(anfds[fd].events & events) {
	struct event* io_w = anfds[fd].next;
	while(io_w != NULL && io_w->cb != NULL) {
	    if(io_w->events & events && io_w->fd == fd) {
		io_w->cb(io_w, io_w->events);
	    }
	    if(anfds[fd].isused) { // call back may be delete
		io_w = io_w->next;
	    }else {
		break;
	    }
	}
    }
}

bool EventLoop::array_needsize(int need_cnt)
{
    int cur = max_events;
    if(need_cnt > cur) {
	int newcnt = cur;
	do {
	    newcnt += (newcnt >> 1) + 16;
	}while(need_cnt > newcnt);
        struct ANFD* tempadfds = (struct ANFD*)realloc(anfds, sizeof(struct ANFD)*newcnt);
	if(tempadfds == NULL) {
	    return false;
	}
	anfds = tempadfds;

        struct epoll_event* tempevents = (struct epoll_event*)realloc(events, sizeof(struct epoll_event)*newcnt);
	if(tempevents == NULL) {
	    return false;
	}
	events = tempevents;
	
	fprintf(stdin, "relloc %ld for epoll_event array\n", sizeof(struct epoll_event)*newcnt);

	init_events(cur, newcnt-cur);
	max_events = newcnt;
    }
    return true;
}

void EventLoop::init_events(int start, int count) {
    for(int i = 0; i < count; i++) {
	anfds[i+start].isused = 0;
	anfds[i+start].fd = 0;
	anfds[i+start].events = 0;
	anfds[i+start].next = NULL;
    }

}

} // namespace common
} // namespace sails


#include <common/base/event_loop.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

using namespace sails::common;

int main(int argc, char *argv[])
{
    EventLoop *ev_loop = new EventLoop(NULL); 
    ev_loop->init();

    int shutdown = socket(AF_INET, SOCK_STREAM, 0);
    // 创建 shutdown 事件
    sails::common::event shutdown_ev;
    emptyEvent(shutdown_ev);
    shutdown_ev.fd = shutdown;
    shutdown_ev.events = sails::common::EventLoop::Event_READ;
    shutdown_ev.cb = NULL; //不需要事件的处理函数

    assert(ev_loop->event_ctl(EventLoop::EVENT_CTL_ADD,
			      &shutdown_ev));
    
    ev_loop->start_loop();

    return 0;
}

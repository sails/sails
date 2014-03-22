#include "poll.h"
#include <sys/epoll.h>

namespace sails {

void Poll::init() {
    poll_fd = epoll_create(10);
    if (poll_fd == -1) {
	perror("poll_init");
	exit(EXIT_FAILURE);
    }
    return 0;
}

void Poll::set() {
     
}

void Poll::start() {
     
}

void Poll::stop() {
     
}

} //namespace sails











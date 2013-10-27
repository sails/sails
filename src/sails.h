#ifndef _SAILS_H_
#define _SAILS_H_

#include <ev.h>

namespace sails {

  void sails_init();

  void init_config(int argc, char *argv[]);

  void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

  void recv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

}


#endif /* _SAILS_H_ */

















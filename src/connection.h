#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <ev.h>
#include "http.h"

namespace sails {

class Connection {
public:
	static void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);
	
	static void recv_cb(struct ev_loop *loop, struct ev_io *watcher, int revents);

};

} //namespace sails

#endif /* _CONNECTION_H_ */

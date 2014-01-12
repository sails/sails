#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <ev.h>
#include <string>
#include <vector>
#include <thread>
#include "http.h"

namespace sails {

void read_data(int connfd);

typedef struct ConnectionnHandleParam {
	char *message;
	int connfd;
} ConnectionnHandleParam;

class Connection {
public:
     static void set_max_connectfd(int max_connfd);
     static void handle(void *message);
};

} //namespace sails

#endif /* _CONNECTION_H_ */















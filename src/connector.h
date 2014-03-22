#ifndef _CONNECTOR_H_
#define _CONNECTOR_H_

#include <comm/net/buffer.h>

namespace sails {
namespace net {

class Connector {
public:
     Connector(int connect_fd);
     ~Connector();
private:
     Connector(const Connector&);
     Connector& operator=(const Connector&);

public:
     int read();
     const char* peek();
     void retrieve(int len);

     int write(char* data, int len);
     int send();
private:
     sails::net::Buffer in_buf;
     sails::net::Buffer out_buf;
     int connect_fd;
};

} // namespace net
} // namespace sails

#endif /* _CONNECTOR_H_ */

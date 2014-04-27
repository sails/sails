#ifndef _CONNECTOR_H_
#define _CONNECTOR_H_

#include <common/base/buffer.h>

namespace sails {
namespace common {
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
protected:
    sails::common::Buffer in_buf;
    sails::common::Buffer out_buf;
    int connect_fd;
};

} // namespace net
} // namespace common
} // namespace sails

#endif /* _CONNECTOR_H_ */

#ifndef _CONNECTOR_H_
#define _CONNECTOR_H_

#include <common/base/buffer.h>
#include <common/base/uncopyable.h>
#include <list>
#include <memory>

namespace sails {
namespace common {
namespace net {

class ConnectorTimerEntry;

class Connector {
public:
    Connector(int connect_fd);
    Connector(); // after create, must call connect yourself
    ~Connector();
private:
    Connector(const Connector&);
    Connector& operator=(const Connector&);

public:
    bool connect(const char *ip, uint16_t port, bool keepalive);
    int read();
    const char* peek();
    void retrieve(int len);

    int write(char* data, int len);
    int send();


    friend class ConnectorTimerEntry;
    friend class ConnectorTimeout;
protected:
    sails::common::Buffer in_buf;
    sails::common::Buffer out_buf;
    int connect_fd;
    bool has_set_timer;
    std::weak_ptr<ConnectorTimerEntry> timer_entry;
    
};

class ConnectorTimerEntry : public Uncopyable {
public:
    ConnectorTimerEntry(Connector* connector);
    ~ConnectorTimerEntry();

    friend class Connector;
private:
    Connector* connector;
};

class ConnectorTimeout : public Uncopyable {
public:
    ConnectorTimeout(int timeout=ConnectorTimeout::default_timeout); // seconds
    ~ConnectorTimeout();
    
    void update_connector_time(Connector* connector);
private:
    class Bucket {
    public:
	std::list<std::shared_ptr<ConnectorTimerEntry>> entry_list;
    };
private:
    const static int default_timeout = 10;
    int timeout;
    int timeindex;
    std::vector<Bucket*> *time_wheel;
};


} // namespace net
} // namespace common
} // namespace sails

#endif /* _CONNECTOR_H_ */


#ifndef _CONNECTOR_H_
#define _CONNECTOR_H_

#include <common/base/buffer.h>
#include <common/base/uncopyable.h>
#include <common/base/timer.h>
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
    virtual ~Connector();
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
    ConnectorTimerEntry(Connector* connector, EventLoop *ev_loop);
    ~ConnectorTimerEntry();

    friend class Connector;
private:
    Connector* connector;
    EventLoop *ev_loop;
};

class ConnectorTimeout : public Uncopyable {
public:
    ConnectorTimeout(int timeout=ConnectorTimeout::default_timeout); // seconds
    ~ConnectorTimeout();
    
    bool init(EventLoop *ev_loop);
    void update_connector_time(Connector* connector);
private:
    class Bucket {
    public:
	std::list<std::shared_ptr<ConnectorTimerEntry>> entry_list;
    };
public:
    void process_tick();
    static void timer_callback(void *data);
private:
    const static int default_timeout = 10;
    int timeout;
    int timeindex;
    std::vector<Bucket*> *time_wheel;

    EventLoop *ev_loop;
    Timer *timer;
};


} // namespace net
} // namespace common
} // namespace sails

#endif /* _CONNECTOR_H_ */


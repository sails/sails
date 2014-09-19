#ifndef _CONNECTOR_H_
#define _CONNECTOR_H_

#include <common/base/buffer.h>
#include <common/base/uncopyable.h>
#include <common/base/timer.h>
#include <common/net/connector.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <assert.h>
#include <list>
#include <memory>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace sails {
namespace common {
namespace net {



#define READBYTES 512

class Connector;

class ConnectorTimerEntry;
class ConnectorTimeout;



typedef void (*TimeoutCB)(Connector* connector);

typedef union Conn_Data {
    void *ptr;
    uint32_t u32;
    uint64_t u64;
} ConnData;


class Connector {
public:
    Connector(int connect_fd);
    Connector(); // after create, must call connect yourself
    virtual ~Connector();
private:
    Connector(const Connector&);
    Connector& operator=(const Connector&);

public:
    bool connect(const char* ip, uint16_t port, bool keepalive);
    int read();
    const char* peek();
    void retrieve(int len);
    int readable();

    int write(const char* data, int len);
    int send();

    void close();
    bool isClosed();

    
    void setId(uint32_t id);
    uint32_t getId();

    std::string getIp();
    void setIp(std::string ip);

    int getPort();
    void setPort(int port);
    
    int get_listen_fd();
    int get_connector_fd();

    void set_timeout();
    bool timeout();

    void setTimeoutCB(TimeoutCB cb);
    void setTimerEntry(std::weak_ptr<ConnectorTimerEntry> entry);
    std::weak_ptr<ConnectorTimerEntry> getTimerEntry();
    bool haveSetTimer();

    void *owner; // 为了当回调时能找到对应的拥有者
    ConnData data;

protected:
    sails::common::Buffer in_buf;
    sails::common::Buffer out_buf;
    int listen_fd;		
    int connect_fd;		// 连接fd
    std::mutex mutex;
private:
    uint32_t id;
    std::string ip;			// 远程连接的ip
    uint16_t port;			// 远程连接的端口
    int  closeType;			// 0:表示客户端主动关闭；1:服务端主动关闭;2:连接超时服务端主动关闭
    bool is_closed;			// 是否已经关闭
    bool is_timeout;
    TimeoutCB timeoutCB;
    bool has_set_timer;		// 是否设置了超时管理器
    std::weak_ptr<ConnectorTimerEntry> timer_entry; // 超时管理项

};


class ConnectorTimerEntry : public Uncopyable {
public:
    ConnectorTimerEntry(std::shared_ptr<Connector> connector, EventLoop *ev_loop);
    ~ConnectorTimerEntry();

private:
    std::weak_ptr<Connector> connector;
    EventLoop *ev_loop;
};



class ConnectorTimeout : public Uncopyable {
public:
    ConnectorTimeout(int timeout=ConnectorTimeout::default_timeout); // seconds
    ~ConnectorTimeout();
    
    bool init(EventLoop *ev_loop);
    void update_connector_time(std::shared_ptr<Connector> connector);
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

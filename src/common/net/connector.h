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

template<typename T> class Connector;

template<typename T> using PARSER_CB = T* (*)(Connector<T> *connector);
template<typename T> using INVALID_MSG_CB = void (*)(Connector<T> *connector);
template<typename T> using DELETE_CB = void (*)(Connector<T> *connector);

template<typename T> class ConnectorTimerEntry;
template<typename T> class ConnectorTimeout;

template<typename T>
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
    int readable();

    int write(char* data, int len);
    int send();

    void close();
    bool isClosed();

    void parser();
    int get_connector_fd();
    T* get_next_packet();
    
    void set_parser_fun(PARSER_CB<T> parser_cb);
    void set_invalid_msg_cb(INVALID_MSG_CB<T> cb);
    INVALID_MSG_CB<T> get_invalid_msg_cb();
    void set_delete_cb(DELETE_CB<T> cb);

    friend class ConnectorTimerEntry<T>;
    friend class ConnectorTimeout<T>;

    void *data;
    int extern_data;
protected:
    sails::common::Buffer in_buf;
    sails::common::Buffer out_buf;
    int connect_fd;
    std::mutex fd_lock;
    bool has_set_timer;
    std::weak_ptr<ConnectorTimerEntry<T>> timer_entry;

    PARSER_CB<T> parser_cb;
    void push_recv_list(T *packet);
    std::list<T *> recv_list;
    INVALID_MSG_CB<T> invalid_msg_cb;
    DELETE_CB<T> delete_cb;
private:
    bool is_closed;
};

template<typename T>
class ConnectorTimerEntry : public Uncopyable {
public:
    ConnectorTimerEntry(Connector<T>* connector, EventLoop *ev_loop);
    ~ConnectorTimerEntry();

    friend class Connector<T>;
private:
    Connector<T>* connector;
    EventLoop *ev_loop;
};

template<typename T>
class ConnectorTimeout : public Uncopyable {
public:
    ConnectorTimeout(int timeout=ConnectorTimeout::default_timeout); // seconds
    ~ConnectorTimeout();
    
    bool init(EventLoop *ev_loop);
    void update_connector_time(Connector<T>* connector);
private:
    class Bucket {
    public:
	std::list<std::shared_ptr<ConnectorTimerEntry<T>>> entry_list;
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













template<typename T>
Connector<T>::Connector(int conn_fd) {
    parser_cb = NULL;
    invalid_msg_cb = NULL;
    delete_cb = NULL;
    connect_fd = conn_fd;
    has_set_timer = false;
    data = NULL;
    extern_data = 0;
    is_closed = false;
}

template<typename T>
Connector<T>::Connector() {
    parser_cb = NULL;
    invalid_msg_cb = NULL;
    delete_cb = NULL;
    has_set_timer = false;
    data = NULL;
    extern_data = 0;
    is_closed = false;
}

template<typename T>
Connector<T>::~Connector() {

    if (delete_cb != NULL) {
	delete_cb(this);
    }

    if (connect_fd > 0) {
	::close(connect_fd);
    }

    if(timer_entry.use_count() > 0) {
	std::shared_ptr<ConnectorTimerEntry<T>> entry = timer_entry.lock();
        entry->connector = NULL;
    }
}

template<typename T>
bool Connector<T>::connect(const char *ip, uint16_t port, bool keepalive) {
    struct sockaddr_in serveraddr;
    connect_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(connect_fd == -1) {
	printf("new connect_fd error\n");
	return false;
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    serveraddr.sin_port = htons(port);


    int ret = ::connect(connect_fd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if(ret == -1) {
	printf("connect failed\n");
	return false;
    }
    if(keepalive) {
	// new thread to send ping
//	std::thread();
    }

    return true;
}

template<typename T>
int Connector<T>::get_connector_fd() {
    return this->connect_fd;
}

template<typename T>
void Connector<T>::set_parser_fun(PARSER_CB<T> parser_cb) {
    this->parser_cb = parser_cb;
}

template<typename T>
void Connector<T>::set_invalid_msg_cb(INVALID_MSG_CB<T> cb)
{
    this->invalid_msg_cb = cb;
}

template<typename T>
INVALID_MSG_CB<T> Connector<T>::get_invalid_msg_cb() {
    return this->invalid_msg_cb;
}

template<typename T>
void Connector<T>::set_delete_cb(DELETE_CB<T> cb)
{
    this->delete_cb = cb;
}

template<typename T>
void Connector<T>::close()
{
    this->fd_lock.lock();
    if (!is_closed && this->connect_fd > 0) {
	::close(this->connect_fd);
	is_closed = true;
    }
    this->fd_lock.unlock();
}

template<typename T>
bool Connector<T>::isClosed()
{
    return is_closed;
}


template<typename T>
int Connector<T>::read() {
    this->fd_lock.lock();
    int read_count = 0;
    if (!is_closed && this->connect_fd > 0) {
	char tmp[65536] = {'\0'};
	read_count = ::read(this->connect_fd, tmp, 65536);
	if(read_count > 0) {
	    this->in_buf.append(tmp, read_count);
	}
    }
    this->fd_lock.unlock();

    return read_count;
}

template<typename T>
int Connector<T>::readable() {
    return in_buf.readable();
}


template<typename T>
const char* Connector<T>::peek() {
    return this->in_buf.peek();
}

template<typename T>
void Connector<T>::retrieve(int len) {
    return this->in_buf.retrieve(len);
}

template<typename T>
void Connector<T>::parser() {
    if (this->parser_cb != NULL) {
	T* packet = NULL;
	while ((packet = parser_cb(this)) != NULL) {
	    push_recv_list(packet);
	    packet = NULL;
	}
    }
}

template<typename T>
void Connector<T>::push_recv_list(T *packet) {
    if(recv_list.size() <= 20) {
	recv_list.push_back(packet);
    }else {
	char msg[100];
	memset(msg, '\0', 100);
	sprintf(msg, "connect fd %d unhandle recv list more than 20 and can't parser", connect_fd);
	perror(msg);
    }
}

template<typename T>
T* Connector<T>::get_next_packet() {
    if (!recv_list.empty()) {
        T *packet = recv_list.front();
	recv_list.pop_front();
	return packet;
    }
    return NULL;
}

template<typename T>
int Connector<T>::write(char* data, int len) {
    int ret = 0;
    if(len > 0 && data != NULL) {
	out_buf.append(data, len);
    }
    return ret;
}

template<typename T>
int Connector<T>::send() {
    this->fd_lock.lock();
    int ret = 0;
    if (!is_closed && this->connect_fd > 0) {
	int write_able = this->out_buf.readable();
	if(write_able > 0) {
	    ret = ::write(this->connect_fd, this->out_buf.peek(), write_able);
	    if(ret > 0) {
		this->out_buf.retrieve(ret);
	    }
	}
    }
    this->fd_lock.unlock();
    return ret;

}

template<typename T>
ConnectorTimerEntry<T>::ConnectorTimerEntry(Connector<T>* connector, EventLoop *ev_loop) {
    this->connector = connector;
    this->ev_loop = ev_loop;
}

template<typename T>
ConnectorTimerEntry<T>::~ConnectorTimerEntry() {
    if(this->connector != NULL) {
//	printf("delete connector\n");
	// close fd and delete connector
	ev_loop->event_stop(connector->connect_fd);
	delete connector;
	connector = NULL;
    }
//    printf("delete connector timer entry\n");
}


template<typename T>
ConnectorTimeout<T>::ConnectorTimeout(int timeout) {
    timeindex = 0;
    assert(timeout > 0);
    this->timeout = timeout;
    
    time_wheel = new std::vector<ConnectorTimeout::Bucket*>(timeout);
    for(int i = 0; i < timeout; i++) {
	time_wheel->at(i) = new ConnectorTimeout::Bucket();
    }
}

template<typename T>
bool ConnectorTimeout<T>::init(EventLoop *ev_loop) {
    timer = new Timer(ev_loop, 1);
    timer->init(ConnectorTimeout::timer_callback, this, 5);
    this->ev_loop = ev_loop;
}

template<typename T>
void ConnectorTimeout<T>::timer_callback(void *data) {
    ConnectorTimeout<T> *timeout = (ConnectorTimeout*)data;
    timeout->process_tick();
    
}


template<typename T>
void ConnectorTimeout<T>::process_tick() {
    timeindex = (timeindex+1)%timeout;
    //empty bucket
    Bucket* bucket = time_wheel->at(timeindex);
    if (bucket != NULL) {
//	printf("clear timerindex:%d\n", timeindex);
	typename std::list<std::shared_ptr<ConnectorTimerEntry<T>>>::iterator iter;
	for (iter=bucket->entry_list.begin(); iter!=bucket->entry_list.end(); iter++) {
//	    printf("user count :%ld\n",(*iter).use_count());
	}

        bucket->entry_list.clear();
    }
}

template<typename T>
ConnectorTimeout<T>::~ConnectorTimeout() {
    if(time_wheel != NULL) {
	while(!time_wheel->empty()) {
	    Bucket* bucket = time_wheel->back();
	    delete bucket;
	    time_wheel->pop_back();
	}
	delete time_wheel;
	time_wheel = NULL;
    }
    this->ev_loop = NULL;
    if (timer != NULL) {
	timer->disarms();
	delete timer;
	timer = NULL;
    }
}

template<typename T>
void ConnectorTimeout<T>::update_connector_time(Connector<T>* connector)
{
    if(connector != NULL) {
	int add_index = (timeindex+timeout-1)%timeout;
//	printf("add to bucket %d\n", add_index);
	if(!connector->has_set_timer) {
	    connector->has_set_timer = true;
	    std::shared_ptr<ConnectorTimerEntry<T>> shared_entry(new ConnectorTimerEntry<T>(connector, ev_loop));
	    std::weak_ptr<ConnectorTimerEntry<T>> weak_temp(shared_entry);
	    connector->timer_entry = weak_temp;
	    time_wheel->at(add_index)->entry_list.push_back(shared_entry);
	}else {
	    time_wheel->at(add_index)->entry_list.push_back(
		connector->timer_entry.lock());
	}
    }
}


} // namespace net
} // namespace common
} // namespace sails

#endif /* _CONNECTOR_H_ */


#include <common/net/connector.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

namespace sails {
namespace common {
namespace net{

Connector::Connector(int conn_fd) {
    connect_fd = conn_fd;
    has_set_timer = false;
}

Connector::Connector() {
    has_set_timer = false;
}

Connector::~Connector() {
    close(connect_fd);
    if(timer_entry.use_count() > 0) {
	std::shared_ptr<ConnectorTimerEntry> entry = timer_entry.lock();
        entry->connector = NULL;
    }
}


bool Connector::connect(const char *ip, uint16_t port, bool keepalive) {
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

int Connector::read() {
    char tmp[65536];
    int read_count = ::read(this->connect_fd, tmp, 65536);
    if(read_count > 0) {
	this->in_buf.append(tmp, read_count);
    }

    return read_count;
}

const char* Connector::peek() {
    return this->in_buf.peek();
}

void Connector::retrieve(int len) {
    return this->in_buf.retrieve(len);
}

int Connector::write(char* data, int len) {
    int ret = 0;
    if(len > 0 && data != NULL) {
	out_buf.append(data, len);
    }
    return ret;
}

int Connector::send() {
    int ret = 0;
    int write_able = this->out_buf.readable();
    if(write_able > 0) {
	ret = ::write(this->connect_fd, this->out_buf.peek(), write_able);
	if(ret > 0) {
	    this->out_buf.retrieve(ret);
	}
    }
    return ret;

}


ConnectorTimerEntry::ConnectorTimerEntry(Connector* connector, EventLoop *ev_loop) {
    this->connector = connector;
    this->ev_loop = ev_loop;
}


ConnectorTimerEntry::~ConnectorTimerEntry() {
    if(this->connector != NULL) {
	printf("delete connector\n");
	// close fd and delete connector
	ev_loop->event_stop(connector->connect_fd);
	delete connector;
	connector = NULL;
    }
    printf("delete connector timer entry\n");
}

const int ConnectorTimeout::default_timeout;

ConnectorTimeout::ConnectorTimeout(int timeout) {
    timeindex = 0;
    if(timeout <= 0) {
	this->timeout = ConnectorTimeout::default_timeout;
    }else {
	this->timeout = timeout;
    }
    
    time_wheel = new std::vector<ConnectorTimeout::Bucket*>(timeout);
    for(int i = 0; i < timeout; i++) {
	time_wheel->at(i) = new ConnectorTimeout::Bucket();
    }
}

bool ConnectorTimeout::init(EventLoop *ev_loop) {
    timer = new Timer(ev_loop, 1);
    timer->init(ConnectorTimeout::timer_callback, this, 5);
    this->ev_loop = ev_loop;
}

void ConnectorTimeout::timer_callback(void *data) {
    ConnectorTimeout *timeout = (ConnectorTimeout*)data;
    timeout->process_tick();
    
}

void ConnectorTimeout::process_tick() {
    timeindex = (timeindex+1)%timeout;
    //empty bucket
    Bucket* bucket = time_wheel->at(timeindex);
    if (bucket != NULL) {
	printf("clear timerindex:%d\n", timeindex);
	std::list<std::shared_ptr<ConnectorTimerEntry>>::iterator iter;
	for (iter=bucket->entry_list.begin(); iter!=bucket->entry_list.end(); iter++) {
	    printf("user count :%ld\n",(*iter).use_count());
	}

        bucket->entry_list.clear();
    }
}

ConnectorTimeout::~ConnectorTimeout() {
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

void ConnectorTimeout::update_connector_time(Connector* connector)
{
    if(connector != NULL) {
	int add_index = (timeindex+timeout-1)%timeout;
	printf("add to bucket %d\n", add_index);
	if(!connector->has_set_timer) {
	    connector->has_set_timer = true;
	    std::shared_ptr<ConnectorTimerEntry> shared_entry(new ConnectorTimerEntry(connector, ev_loop));
	    std::weak_ptr<ConnectorTimerEntry> weak_temp(shared_entry);
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

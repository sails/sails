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


ConnectorTimerEntry::ConnectorTimerEntry(Connector* connector) {
    this->connector = connector;
}


ConnectorTimerEntry::~ConnectorTimerEntry() {
    if(this->connector != NULL) {
	// close fd and delete connector
	delete connector;
	connector = NULL;
    }
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

ConnectorTimeout::~ConnectorTimeout() {
    if(time_wheel != NULL) {
	while(!time_wheel->empty()) {
	    Bucket* bucket = time_wheel->back();
	    delete bucket;
	    time_wheel->pop_back();
	}
    }
}

void ConnectorTimeout::update_connector_time(Connector* connector)
{
    if(connector != NULL) {
	int add_index = (timeindex+timeout-1)%timeout;
	if(!connector->has_set_timer) {
	    connector->has_set_timer = true;
	    std::shared_ptr<ConnectorTimerEntry> shared_entry(new ConnectorTimerEntry(connector));
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

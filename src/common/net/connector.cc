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
}

Connector::Connector() {
    
}

Connector::~Connector() {
     
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

} // namespace net
} // namespace common
} // namespace sails

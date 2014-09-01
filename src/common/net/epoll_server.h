#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include <common/base/thread_queue.h>
#include <common/net/net_thread.h>

namespace sails {
namespace common {
namespace net {

template<typename T>
class EpollServer {
public:
    ////////////////////////////////////////////////////////////////////////////


    typedef ThreadQueue<TagRecvData*, std::deque<TagRecvData*>> recv_queue;
    typedef ThreadQueue<TagSendData*, std::deque<TagSendData*>> send_queue;
    typedef recv_queue::queue_type recv_queue_type;

    ////////////////////////////////////////////////////////////////////////////
    
    // 构造函数
    EpollServer(unsigned int NetThreadNum = 1);
    
    // 析构函数
    ~EpollServer();

    // 创建epoll
    void createEpoll();

    // 设置空连接超时时间
    void setEmptyConnTimeout(int timeout);

    // 绑定一个网络线程去监听端口
    void bind(int port);

    // 开始运行网络线程
    bool startNetThread();

    // 终止网络线程
    bool stopNetThread();

    // 关闭连接
    void close(int uid);
    
    // 解析数据包
    virtual void parse() {
	printf(" need implement parser method in subclass\n");}
    
    // 处理队列中的数据包
    void handle();

    // 发送数据
    void send(const std::string &s, const std::string &ip, uint16_t port, int uid);

    void process_pipe(common::event* e, int revents);
private:

    // 网络线程
    std::vector<NetThread<T>*> netThreads;


    // 网络线程数目
    unsigned int netThreadNum;

    // 服务是否停止
    bool bTerminate;

    // 业务线程是否启动
    bool handleStarted;
};







template<typename T>
EpollServer<T>::EpollServer(unsigned int netThreadNum) {
    this->netThreadNum = netThreadNum;
    if (netThreadNum < 0) {
        this->netThreadNum = 1;
    }else if (netThreadNum > 15) {
	this->netThreadNum = 15;
    }
    for (size_t i = 0; i < this->netThreadNum; i++) {
	printf("new net thread i:%ld\n", i);
	NetThread<T> *netThread = new NetThread<T>(this);
	netThreads.push_back(netThread);
    }
}


template<typename T>
EpollServer<T>::~EpollServer() {

}


template<typename T>
void EpollServer<T>::createEpoll() {
    for (size_t i = 0; i < this->netThreadNum; i++) {
	netThreads[i]->create_event_loop();
    }
}

template<typename T>
void EpollServer<T>::setEmptyConnTimeout(int timeout) {
    for (int i = 0; i < netThreadNum; i++) {
	netThreads[i]->setEmptyConnTimeout(timeout);
    }
}


template<typename T>
void EpollServer<T>::bind(int port) {
    netThreads[0]->bind(port);
}

// 开始运行网络线程
template<typename T>
bool EpollServer<T>::startNetThread() {
    for (size_t i = 0; i < netThreadNum; i++) {
	printf("start net thread i:%ld\n", i);
	netThreads[i]->run();
    }
}

// 终止网络线程
template<typename T>
bool EpollServer<T>::stopNetThread() {
    for (size_t i = 0; i < netThreadNum; i++) {
	netThreads[i]->terminate();
	netThreads[i]->join();
    }
}


template<typename T>
void EpollServer<T>::process_pipe(common::event* e, int revents) {
    char buf[100]= {'\0'};
    read(e->fd, buf, 100);
    printf("get a pile msg :%s\n", buf);
}




} // net
} // common
} // sails

#endif /* EPOLL_SERVER_H */








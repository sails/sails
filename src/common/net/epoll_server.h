#ifndef EPOLL_SERVER_H
#define EPOLL_SERVER_H

#include <common/base/thread_queue.h>
#include <common/net/handle_thread.h>
#include <common/net/net_thread.h>
#include <common/net/dispatcher_thread.h>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace sails {
namespace common {
namespace net {

template<typename T>
class EpollServer {
public:

    // 构造函数
    EpollServer(unsigned int NetThreadNum = 1);
    
    // 析构函数
    virtual ~EpollServer();

    // 创建epoll
    void createEpoll();

    // 设置空连接超时时间
    void setEmptyConnTimeout(int timeout) { connectorTimeout = timeout;}

    int getEmptyConnTimeout() { return connectorTimeout;}

    // 绑定一个网络线程去监听端口
    void bind(int port);

    // 开始运行网络线程
    bool startNetThread();

    // 终止网络线程
    bool stopNetThread();

    // 增加连接
    void addConnector(std::shared_ptr<common::net::Connector> connector, int fd);
    
    // 选择网络线程
    NetThread<T>* getNetThreadOfFd(int fd) {
	return netThreads[fd % netThreads.size()];
    }


    virtual void create_connector_cb(std::shared_ptr<common::net::Connector> connector);
    
    virtual void parseImp(std::shared_ptr<common::net::Connector> connector);
    // 解析数据包
    virtual T* parse(std::shared_ptr<common::net::Connector> connector) {
	printf(" need implement parser method in subclass\n");
    }

    
    bool add_handle(HandleThread<T> *handle);

    // 开始运行处理线程
    bool startHandleThread();
    // 终止处理线程
    bool stopHandleThread();

    // 分发线程等待数据
    void dipacher_wait();

    // 通知分发线程有数据
    void notify_dispacher();

    // 得到接收队列个数,用于dispacher线程循环的从队列中得到数据
    int getRecvQueueNum();

    // 得到接收到的数据数
    size_t getRecvDataNum();

    // 从io线程队列中得到数据包,用于dispacher线程
    // index指io线程的标志
    TagRecvData<T>* getRecvPacket(int index);

    // 处理线程数
    int getHandleNum() {
	return handleThreads.size();
    }

    // 向处理线程中加入消息
    void addHandleData(TagRecvData<T>*data, int handleIndex);
    

    // 发送数据
    void send(const std::string &s, const std::string &ip, uint16_t port, int uid, int fd);

    // 关闭连接
    void close_connector(const std::string &ip, uint16_t port, int uid, int fd);

    // 当epoll读到0个数据时，客户端主动close
    virtual void closed_connect_cb(std::shared_ptr<common::net::Connector> connector);

    // 当连接超时时,提供应用层处理机会
    virtual void connector_timeout_cb(common::net::Connector* connector);

    void process_pipe(common::event* e, int revents);

    friend class HandleThread<T>;

private:

    // 网络线程
    std::vector<NetThread<T>*> netThreads;
    
    // 逻辑处理线程
    std::vector<HandleThread<T>*> handleThreads;

    // 消息分发线程
    DispatcherThread<T>* dispacher_thread;

    // 网络线程数目
    unsigned int netThreadNum;

    // io线程将数据入队时通过dispacher线程分发
    std::mutex dispacher_mutex;
    std::condition_variable dispacher_notify;

    // 服务是否停止
    bool bTerminate;

    // 业务线程是否启动
    bool handleStarted;

    // 空链超时时间
    int connectorTimeout;
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
    bTerminate = true;
    connectorTimeout = 0;
}


template<typename T>
EpollServer<T>::~EpollServer() {
    printf("delete server\n");
    for (size_t i = 0; i < this->netThreadNum; i++) {
	delete netThreads[i];
    }
    printf("delete server\n");
}


template<typename T>
void EpollServer<T>::createEpoll() {
    for (size_t i = 0; i < this->netThreadNum; i++) {
	netThreads[i]->create_event_loop();
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
void EpollServer<T>::addConnector(std::shared_ptr<common::net::Connector> connector, int fd) {
    NetThread<T>* netThread = getNetThreadOfFd(fd);
    netThread->add_connector(connector);
}


template<typename T>
void EpollServer<T>::process_pipe(common::event* e, int revents) {
    char buf[100]= {'\0'};
    read(e->fd, buf, 100);
    printf("get a pile msg :%s\n", buf);
}

template<typename T>
void EpollServer<T>::parseImp(std::shared_ptr<common::net::Connector> connector) {
    T* packet = NULL;
    while((packet = this->parse(connector)) != NULL) {
	TagRecvData<T>* data = new TagRecvData<T>();
	data->uid = connector->getId();
	data->data = packet;
	data->ip = connector->getIp();
	data->port= connector->getPort();
	data->fd = connector->get_connector_fd();
	
	NetThread<T>* netThread = getNetThreadOfFd(connector->get_connector_fd());
        netThread->addRecvList(data);
    }
}

template<typename T>
void EpollServer<T>::create_connector_cb(std::shared_ptr<common::net::Connector> connector) {
    printf("create connector cb\n");
}

template<typename T>
bool EpollServer<T>::add_handle(HandleThread<T> *handle) {
    handleThreads.push_back(handle);
}

template<typename T>
bool EpollServer<T>::startHandleThread() {
    for (HandleThread<T> *handle : handleThreads) {
	handle->run();
    }
    dispacher_thread = new DispatcherThread<T>(this);
    dispacher_thread->run();
}


template<typename T>
bool EpollServer<T>::stopHandleThread() {
    printf("stop handle thread\n");
    for (int i = 0; i < handleThreads.size(); i++) {
        handleThreads[i]->terminate();
        handleThreads[i]->join();
	handleThreads[i] = NULL;

    }

    printf("stop dispacher thread\n");
    dispacher_thread->terminate();
    dispacher_thread->join();
    delete dispacher_thread;
    dispacher_thread = NULL;
    printf(" end stop dispacher\n");
}

template<typename T>
void EpollServer<T>::addHandleData(TagRecvData<T>*data, int handleIndex) {
    handleThreads[handleIndex]->addForHandle(data);
}


template<typename T>
void EpollServer<T>::dipacher_wait() {
    std::unique_lock<std::mutex> locker(dispacher_mutex);
    size_t dataSize = 0;
    dispacher_notify.wait(locker);

}


template<typename T>
void EpollServer<T>::notify_dispacher() {
    std::unique_lock<std::mutex> locker(dispacher_mutex);
    dispacher_notify.notify_one();
}

template<typename T>
int EpollServer<T>::getRecvQueueNum() {
    return netThreads.size();
}


template<typename T>
size_t EpollServer<T>::getRecvDataNum() {
    size_t num = 0;
    for (int i = 0; i < netThreads.size(); i++) {
	num = num + netThreads[i]->get_recvqueue_size();
    }
    return num;
}

template<typename T>
TagRecvData<T>* EpollServer<T>::getRecvPacket(int index) {
    TagRecvData<T> *data = NULL;
    if (netThreads.size() >= index) {	
        netThreads[index]->getRecvData(data, 0); //不阻塞
    }
    return data;
}

template<typename T>
void EpollServer<T>::send(const std::string &s, const std::string &ip, uint16_t port, int uid, int fd) {
    NetThread<T>* netThread = getNetThreadOfFd(fd);
    if (netThread != NULL) {
	netThread->send(ip, port,uid, s);
    }

}

template<typename T>
void EpollServer<T>::close_connector(const std::string &ip, uint16_t port, int uid, int fd) {
    NetThread<T>* netThread = getNetThreadOfFd(fd);
    if (netThread != NULL) {
	netThread->close_connector(ip, port,uid, fd);
    }
}

template<typename T>
void EpollServer<T>::closed_connect_cb(std::shared_ptr<common::net::Connector> connector) {
    if (!connector->isClosed()) {
	close_connector(connector->getIp(), connector->getPort(), connector->getId(), connector->get_connector_fd());
    }

}

template<typename T>
void EpollServer<T>::connector_timeout_cb(common::net::Connector* connector) {
    
}

} // net
} // common
} // sails

#endif /* EPOLL_SERVER_H */








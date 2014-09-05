#ifndef DISPATCHER_THREAD_H
#define DISPATCHER_THREAD_H
#include <thread>
#include <unistd.h>
#include <common/net/epoll_server.h>

namespace sails {
namespace common {
namespace net {


template<typename T>
class DispatcherThread {
public:
    DispatcherThread(EpollServer<T>* server);
    
    // 网络线程运行状态
    enum RunStatus {
	RUNING,
	PAUSE,
	STOPING
    };

    void run();
    void terminte();
    void join();

    static void dispatch(DispatcherThread<T>* dispacher);

private:
    EpollServer<T>* server;
    int status;
    std::thread *thread;
    bool continueRun;
};




template<typename T>
DispatcherThread<T>::DispatcherThread(EpollServer<T>* server) {
    this->server = server;
    status = DispatcherThread<T>::STOPING;
    thread = NULL;
    continueRun = false;
}

template<typename T>
void DispatcherThread<T>::run() {
    continueRun = true;
    thread = new std::thread(dispatch, this);
    status = DispatcherThread<T>::RUNING;
}


template<typename T>
void DispatcherThread<T>::dispatch(DispatcherThread<T>* dispacher) {
    while(dispacher->continueRun) {
	
	dispacher->server->dipacher_wait(); //会一直wait直到有数据
	
	int recvQueueNum = dispacher->server->getRecvQueueNum();
	for (int i = 0; i < recvQueueNum; i++) {
	    TagRecvData<T>* data = NULL;
	    do {
	        data = dispacher->server->getRecvPacket(i);
		if (data != NULL) {
		    // 开始分发消息
		    int fd = data->fd;
		    int handleNum = dispacher->server->getHandleNum();
		    int selectedHandle = fd % handleNum;
		    dispacher->server->addHandleData(data, selectedHandle);
		}

	    }while(data != NULL);
	}
    }
}


} // namespace net
} // namespace common
} // namespace sails



#endif /* DISPATCHER_THREAD_H */











#include <common/net/epoll_server.h>
#include <common/net/connector.h>
#include <signal.h>

typedef struct {
    char msg[100];
} __attribute__((packed)) EchoStruct;


class TestServer : public sails::common::net::EpollServer<EchoStruct> {
public:
    TestServer(int netThreadNum) : sails::common::net::EpollServer<EchoStruct>(netThreadNum) {
	
    }
    ~TestServer() {
	
    }

    EchoStruct* parse(std::shared_ptr<sails::common::net::Connector> connector){
	int read_able = connector->readable();
	if(read_able == 0) {
	    return NULL;
	}

	EchoStruct *data = (EchoStruct*)malloc(sizeof(EchoStruct));
	memset(data, '\0', sizeof(EchoStruct));
	strncpy(data->msg, connector->peek(), read_able);
	connector->retrieve(read_able);
	return data;
    }
};


class HandleImpl : public sails::common::net::HandleThread<EchoStruct> {
public:
    HandleImpl(sails::common::net::EpollServer<EchoStruct>* server) : sails::common::net::HandleThread<EchoStruct>(server) {
	
    }
    
    void handle(const sails::common::net::TagRecvData<EchoStruct> &recvData) {
//	    server->close_connector(recvData.ip, recvData.port, recvData.uid, recvData.fd);

	printf("uid:%u, ip:%s, port:%d, msg:%s", recvData.uid, recvData.ip.c_str(), recvData.port, recvData.data->msg);
	sails::common::net::TagSendData *sendData = new sails::common::net::TagSendData();
	std::string buffer = std::string(recvData.data->msg);
	server->send(buffer, recvData.ip, recvData.port, recvData.uid, recvData.fd);

	
    }
};




bool isRun = true;
TestServer server(2);
HandleImpl handle(&server);

void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
	    printf("stop netthread\n");
	    server.stopNetThread();
	    server.stopHandleThread();
	    isRun = false;
	}
    }
}




int main(int argc, char *argv[])
{

    // signal kill
    struct sigaction act;
    act.sa_sigaction = sails_signal_handle;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) == -1) {
	perror("sigaction error");
	exit(EXIT_FAILURE);
    }



    server.createEpoll();

//    server.setEmptyConnTimeout(10);
    server.bind(8000);
    server.startNetThread();
    
    server.add_handle(&handle);
    server.startHandleThread();

    while(isRun) {
	sleep(2);
    }
    


    return 0;
}

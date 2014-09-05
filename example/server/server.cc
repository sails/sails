#include <common/net/epoll_server.h>
#include <common/net/connector.h>

typedef struct {
    char msg[100];
} __attribute__((packed)) EchoStruct;


class TestServer : public sails::common::net::EpollServer<EchoStruct> {
public:
    TestServer() : sails::common::net::EpollServer<EchoStruct>(1) {
	
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
	printf("uid:%u, ip:%s, port:%d, msg:%s\n", recvData.uid, recvData.ip.c_str(), recvData.port, recvData.data->msg);
	sails::common::net::TagSendData *sendData = new sails::common::net::TagSendData();
	std::string buffer = std::string(recvData.data->msg);
	server->send(buffer, recvData.ip, recvData.port, recvData.uid, recvData.fd);
	
    }
};









int main(int argc, char *argv[])
{

    TestServer server;
    server.createEpoll();

    server.setEmptyConnTimeout(10);
    server.bind(8000);
    server.startNetThread();
    
    HandleImpl handle(&server);
    server.add_handle(&handle);
    server.startHandleThread();

    while(1) {
	sleep(2);
    }
    server.stopNetThread();


    return 0;
}

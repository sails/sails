//#include <common/net/server.h>
#include <common/net/packets.h>
#include <unistd.h>

using namespace sails;


typedef struct {
    char msg[100];
} __attribute__((packed)) EchoStruct;

/*
typedef sails::common::net::Server<EchoStruct> CommonServer;
CommonServer server;



EchoStruct* parser(common::net::Connector<EchoStruct> *connector) {
    if (connector->readable() <= 0) {
	return NULL;
    }
    int readable = connector->readable();
    int packetlen = readable<100?readable:100;
    printf("packetlen:%d\n", packetlen);
    EchoStruct* echo = (EchoStruct*)malloc(sizeof(EchoStruct));
    memset(echo, 0, sizeof(EchoStruct));
    strncpy(echo->msg, connector->peek(), packetlen);
    connector->retrieve(packetlen);
    return echo;

}


void handle_fun(std::shared_ptr<common::net::Connector<EchoStruct>> connector, EchoStruct *message) {
    int connfd = connector->get_connector_fd();

    printf("request:%s\n", message->msg);

    EchoStruct *response = (EchoStruct*)malloc(sizeof(EchoStruct));
    memset(response, 0, sizeof(EchoStruct));
    memcpy(response, message, sizeof(EchoStruct));
    
    if(response != NULL) {
	// out put
	int n = write(connfd, response, sizeof(EchoStruct));
    }
    
    free(response);
}

int main(int argc, char *argv[])
{
    server.init(); //default listen 8000 port
    server.set_parser_cb(parser);
    server.set_handle_cb(handle_fun);
    server.start();

    return 0;
}


*/

#include <common/net/net_thread.h>
#include <common/net/epoll_server.h>

int main(int argc, char *argv[])
{
    sails::common::net::EpollServer<EchoStruct> server;
    server.createEpoll();
    server.setEmptyConnTimeout(10);
    server.bind(8000);
    server.startNetThread();
/*
    sails::common::net::NetThread<EchoStruct> net_thread;
    net_thread.create_event_loop();
    net_thread.bind(8000);
    net_thread.create_connector_timeout(10);
    net_thread.run();



    sails::common::net::NetThread<EchoStruct> net_thread1;
    net_thread1.create_event_loop();
    net_thread1.bind(8001);
    net_thread1.create_connector_timeout(10);
    net_thread1.run();
*/
    while(1) {
	sleep(2);
    }
    server.stopNetThread();
    return 0;
}










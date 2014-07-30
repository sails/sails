#include <common/net/server.h>
#include <common/net/packets.h>


using namespace sails;


typedef struct {
    char msg[100];
} __attribute__((packed)) EchoStruct;


typedef sails::common::net::Server<EchoStruct> CommonServer;




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
    CommonServer* server = CommonServer::getInstance();
    server->init();
    server->set_parser_cb(parser);
    server->set_handle_cb(handle_fun);
    server->start();
    return 0;
}












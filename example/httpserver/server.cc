#include "sails/net/http_server.h"


class HandleImpl
  : public sails::net::HandleThread<sails::net::HttpRequest> {
public:
  HandleImpl(sails::net::HttpServer* server)
    : sails::net::HandleThread<sails::net::HttpRequest>(server) {
	
  }
    
  void handle(
              const sails::net::TagRecvData<sails::net::HttpRequest> &recvData) {
    recvData.data->to_str();
    printf("uid:%u, ip:%s, port:%d, msg:\n%s\n", recvData.uid, recvData.ip.c_str(), recvData.port, recvData.data->get_raw());
    std::string buffer("test");
    server->send(buffer, recvData.ip, recvData.port, recvData.uid, recvData.fd);
    // 因为最后都被放到一个队列中处理,所以肯定会send之后,再close
    server->CloseConnector(recvData.ip, recvData.port, recvData.uid, recvData.fd);
  }
};


bool isRun = true;
sails::net::HttpServer server(2);
HandleImpl handle(&server);

void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
	    printf("stop netthread\n");
	    server.StopNetThread();
	    server.StopHandleThread();
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



    server.CreateEpoll();

//    server.SetEmptyConnTimeout(10);
    server.Bind(8000);
    server.StartNetThread();
    
    server.AddHandle(&handle);
    server.StartHandleThread();

    while(isRun) {
	sleep(2);
    }
    


    return 0;
}



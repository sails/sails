#include "sails/net/http_server.h"


bool isRun = true;
sails::net::HttpServer server;
sails::net::HttpServerHandle handle(&server);




class HandleTest {
 public:
  void test1(sails::net::HttpRequest& request,
             sails::net::HttpResponse* response) {
    response->SetBody("call test1");
  }
  void test2(sails::net::HttpRequest& request,
             sails::net::HttpResponse* response) {
    response->SetBody("call test2");
  }
};


void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
	    printf("stop netthread\n");
	    server.StopHandleThread();
            server.StopNetThread();
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

    /*
    server.CreateEpoll();

//    server.SetEmptyConnTimeout(10);
    server.Bind(8000);
    server.StartNetThread();
    */
    server.Init(8000, 2, 10, 1);
    
    server.AddHandle(&handle);
    server.StartHandleThread();

    // 请求处理器
    sails::net::HttpServer* httpserver = &server;
    HandleTest test;
    HTTPBIND(httpserver, "/test1", test, HandleTest::test1);
    HTTPBIND(httpserver, "/test2", test, HandleTest::test2);

    

    while(isRun) {
	sleep(2);
    }
    


    return 0;
}



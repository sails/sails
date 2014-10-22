#include "sails/net/http_server.h"



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


bool isRun = true;

void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
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

    sails::net::HttpServer server;
    server.Init(8000, 2, 10, 1);
    
    // 请求处理器
    sails::net::HttpServer* httpserver = &server;
    HandleTest test;
    HTTPBIND(httpserver, "/test1", test, HandleTest::test1);
    HTTPBIND(httpserver, "/test2", test, HandleTest::test2);

    

    while(isRun) {
	sleep(2);
    }
    
    server.Stop();

    return 0;
}



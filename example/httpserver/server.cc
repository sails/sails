#include "sails/net/http_server.h"


bool isRun = true;
sails::net::HttpServer server(2);
sails::net::HttpServerHandle handle(&server);

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



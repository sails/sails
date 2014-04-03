#include <common/net/event_loop.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <gtest/gtest.h>

using namespace sails::common;


void listen_callback(net:: event*ev, int revents)
{
    printf("listen callback start  \n");
    struct sockaddr_in servaddr, local;
    int addrlen = sizeof(struct sockaddr_in);
    int connfd = accept(ev->fd, 
			(struct sockaddr*)&local, (socklen_t*)&addrlen);
    printf("listen callback:%d\n", connfd);
}


TEST(logging_test, logger)
{

    int  listenfd = 0;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("create listen socket");
	exit(EXIT_FAILURE);
    }
    struct sockaddr_in servaddr, local;
    int addrlen = sizeof(struct sockaddr_in);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(30000);
    
    int flag=1,len=sizeof(int); // for can restart right now
    if( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, len) == -1) 
    {
	perror("setsockopt"); 
	exit(EXIT_FAILURE); 
    }
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

    if(listen(listenfd, 10) < 0) {
	perror("listen");
	exit(EXIT_FAILURE);
    }
 
    net::event ev;
    ev.fd = listenfd;
    ev.events = net::EventLoop::Event_READ;
    ev.cb = &listen_callback;

    net::EventLoop ev_loop;
    ev_loop.init();    
    ev_loop.event_ctl(net::EventLoop::EVENT_CTL_ADD, &ev);
    printf("start loop\n");
    ev_loop.start_loop();
    
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();    
}








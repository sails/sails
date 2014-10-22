// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: server.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-13 10:16:55



#include <signal.h>
#include "sails/net/epoll_server.h"
#include "sails/net/connector.h"
#include "sails/log/logging.h"


typedef struct {
    char msg[100];
} __attribute__((packed)) EchoStruct;

namespace sails {
sails::log::Logger serverlog(sails::log::Logger::LOG_LEVEL_DEBUG,
				  "./log/server.log", sails::log::Logger::SPLIT_DAY);
}

class HandleImpl;

class TestServer : public sails::net::EpollServer<EchoStruct, HandleImpl> {
public:
    TestServer() {
	
    }
    ~TestServer() {
	
    }

    EchoStruct* Parse(std::shared_ptr<sails::net::Connector> connector){
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


class HandleImpl : public sails::net::HandleThread<EchoStruct, HandleImpl> {
public:
  HandleImpl(sails::net::EpollServer<EchoStruct, HandleImpl>* server) : sails::net::HandleThread<EchoStruct, HandleImpl>(server) {
	
    }
    
    void handle(const sails::net::TagRecvData<EchoStruct> &recvData) {
//	    server->CloseConnector(recvData.ip, recvData.port, recvData.uid, recvData.fd);

	printf("uid:%u, ip:%s, port:%d, msg:%s", recvData.uid, recvData.ip.c_str(), recvData.port, recvData.data->msg);
	std::string buffer = std::string(recvData.data->msg);
	server->send(buffer, recvData.ip, recvData.port, recvData.uid, recvData.fd);
    }
};




bool isRun = true;
TestServer server;
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


    server.Init(8000, 2, 10, 1);
    /*
    server.CreateEpoll();

//    server.SetEmptyConnTimeout(10);
    server.Bind(8000);
    server.StartNetThread();
    */
    server.AddHandle(&handle);
    server.StartHandleThread();

    while(isRun) {
	sleep(2);
    }
    


    return 0;
}

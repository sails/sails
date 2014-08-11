#include <common/net/server.h>
#include <common/net/packets.h>
#include "config.h"
#include "module_load.h"
#include "handle_rpc.h"
#include <signal.h>
#include <gperftools/profiler.h>

using namespace sails;

typedef sails::common::net::Server<common::net::PacketCommon> CommonServer;

namespace sails {

CommonServer rpcserver;
Config config;
std::map<std::string, std::string> modules;


int register_service() {
    std::map<std::string, std::string>::iterator iter;
    for(iter = modules.begin(); iter != modules.end()
	    ; iter++) {
	if(!iter->second.empty()) {
	    ModuleLoad::load(iter->second);
	}
    }     
    return 0;
}

void sails_exit() {
    ProfilerStop();
    modules.clear();
    ModuleLoad::unload();
    printf("on exit\n");
    rpcserver.stop();
//    exit(EXIT_SUCCESS);
}

void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
	    sails_exit();
	    break;
	}
    }
}

void sails_init(int argc, char *argv[]) {

    // signal kill
    struct sigaction act;
    act.sa_sigaction = sails_signal_handle;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if(sigaction(SIGINT, &act, NULL) == -1) {
	perror("sigaction error");
	exit(EXIT_FAILURE);
    }

    // config
    config.get_modules(&modules);
    // service
    if (register_service() != 0) {
	perror("register service");
	exit(EXIT_FAILURE);
    }
}






common::net::PacketCommon* parser_cb(
    common::net::Connector<common::net::PacketCommon> *connector) {
    if (connector->readable() < sizeof(common::net::PacketCommon)) {
	return NULL;
    }
    common::net::PacketCommon *packet = (common::net::PacketCommon*)connector->peek();
    if (packet->type.opcode >= common::net::PACKET_MAX
	|| packet->type.opcode <= common::net::PACKET_MIN) { // error, and empty all data
	connector->retrieve(connector->readable());
	return NULL;
    }
    if (packet != NULL) {
	int packetlen = packet->len;
	if (packetlen < sizeof(common::net::PacketCommon)) {
	    return NULL;
	}
	if (packetlen > PACKET_MAX_LEN) {
	    connector->retrieve(packetlen);
	    char errormsg[100] = {'\0'};
	    sprintf(errormsg, "receive a invalid packet len:%d", packetlen);
	    perror(errormsg);
	    
	}
	if(connector->readable() >= packetlen) {
	    common::net::PacketCommon *item = (common::net::PacketCommon*)malloc(packetlen);
	    if (item == NULL) {
		char errormsg[100] = {'\0'};
		sprintf(errormsg, "malloc failed due to copy receive data to a common packet len:%d", packetlen);
		perror(errormsg);
		return NULL;
	    }
	    memset(item, 0, packetlen);
	    memcpy(item, packet, packetlen);
	    connector->retrieve(packetlen);

	    return item;
	}
    }
    
    return NULL;

}


common::HandleChain<common::net::PacketCommon*, 
		    common::net::ResponseContent*> handle_chain;

void handle_fun(std::shared_ptr<common::net::Connector<common::net::PacketCommon>> connector, common::net::PacketCommon *message) {

    int connfd = connector->get_connector_fd();
    common::net::PacketCommon *request = message;

    common::net::ResponseContent content;
    memset(&content, 0, sizeof(common::net::ResponseContent));

    common::HandleChain<common::net::PacketCommon*, 
		    common::net::ResponseContent*> handle_chain;
    HandleRPC proto_decode;
    handle_chain.add_handle(&proto_decode);

    handle_chain.do_handle(request, &content);

    if (content.len > 0 && content.data != NULL) {
	int response_len = sizeof(common::net::PacketRPC)+content.len-1;
	common::net::PacketRPC *response = (common::net::PacketRPC*)malloc(response_len);
	memset(response, 0, response_len);
	response->common.type.opcode = common::net::PACKET_PROTOBUF_RET;
	response->common.len = response_len;
	memcpy(response->data, content.data, content.len);
	
	int n = write(connfd, response, response->common.len);
	free(response);
    }


}

} // namespace sails



int main(int argc, char *argv[])
{
    sails::sails_init(argc, argv);
    rpcserver.init(config.get_listen_port(), 10, config.get_handle_thread_pool(), config.get_handle_request_queue_size());
    rpcserver.set_parser_cb(sails::parser_cb);
    rpcserver.set_handle_cb(sails::handle_fun);
    ProfilerStart("sails.prof");
    rpcserver.start();
    printf("end\n");
    return 0;
}

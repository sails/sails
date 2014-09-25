#include "server.h"
#include "handle_rpc.h"

namespace sails {

sails::common::log::Logger serverlog(common::log::Logger::LOG_LEVEL_DEBUG,
				  "./log/server.log", common::log::Logger::SPLIT_DAY);

Server::Server(int netThreadNum) : sails::common::net::EpollServer<common::net::PacketCommon>(netThreadNum){
    
    // 得到配置的模块
    config.get_modules(&modules_name);
    // 注册模块
    std::map<std::string, std::string>::iterator iter;
    for(iter = modules_name.begin(); iter != modules_name.end()
	    ; iter++) {
	if(!iter->second.empty()) {
	    moduleLoad.load(iter->second);
	}
    }     

}


common::net::PacketCommon* Server::parse(std::shared_ptr<sails::common::net::Connector> connector) {


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

Server::~Server() {
    modules_name.clear();
    moduleLoad.unload();
}











HandleImpl::HandleImpl(sails::common::net::EpollServer<sails::common::net::PacketCommon>* server): sails::common::net::HandleThread<sails::common::net::PacketCommon>(server) {
    
}


void HandleImpl::handle(const sails::common::net::TagRecvData<common::net::PacketCommon> &recvData) {
    
    
    common::net::PacketCommon *request = recvData.data;

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
	
	std::string buffer = std::string((char *)response, response_len);
	server->send(buffer, recvData.ip, recvData.port, recvData.uid, recvData.fd);

	free(response);
    }

}



} // namespace sails

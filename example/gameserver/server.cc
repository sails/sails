#include "server.h"
#include <signal.h>
#include <stdlib.h>
#include <common/base/string.h>
#include "product.h"


namespace sails {

Server::Server(int netThreadNum) : sails::common::net::EpollServer<SceNetAdhocctlPacketBase>(netThreadNum){
    playerList.init(50);
}


// 获取游戏
GameWorld* Server::getGameWorld(std::string& gameCode) {
    std::unique_lock<std::mutex> locker(gameworldMutex);;
    std::map<std::string, GameWorld*>::iterator iter = gameWorldMap.find(gameCode);
    if (iter != gameWorldMap.end()) {
        GameWorld* gameWorld = iter->second;
	return gameWorld;
    }else {
	return NULL;
    }
}
    
// 创建游戏
GameWorld* Server::createGameWorld(std::string& gameCode) {
    std::unique_lock<std::mutex> locker(gameworldMutex);;
    GameWorld* gameWorld = new GameWorld(gameCode, this);
    gameWorldMap.insert(std::pair<std::string, GameWorld*>(gameWorld->getGameCode(), gameWorld));
    return gameWorld;
}


void Server::sendDisConnectDataToHandle(std::shared_ptr<common::net::Connector> connector) {
    uint32_t playerId = connector->data.u32;
    if (playerId <= 0) {
	return;
    }
    // 向handle线程发送消息
    SceNetAdhocctlDisconnectPacketS2C* disdata = new SceNetAdhocctlDisconnectPacketS2C();
    disdata->base.opcode = OPCODE_DISCONNECT;
    disdata->ip = HandleImpl::getIp(connector->getIp());
    disdata->mac = HandleImpl::getMacStruct("EE:EE:EE:EE:EE");

    common::net::TagRecvData<SceNetAdhocctlPacketBase> *data = new common::net::TagRecvData<SceNetAdhocctlPacketBase>();
    data->uid = connector->getId();
    data->data = (sails::SceNetAdhocctlPacketBase*)disdata;
    data->ip = connector->getIp();
    data->port= connector->getPort();
    data->fd = connector->get_connector_fd();

    int fd = data->fd;
    int handleNum = getHandleNum();
    int selectedHandle = fd % handleNum;
    addHandleData(data, selectedHandle);

}



void Server::invalid_msg_handle(std::shared_ptr<sails::common::net::Connector> connector) {

    sendDisConnectDataToHandle(connector);

    // 关闭连接
    connector->data.u32 = 0;
    this->close_connector(connector->getIp(), connector->getPort(), connector->getId(), connector->get_connector_fd());

}

void Server::closed_connect_cb(std::shared_ptr<common::net::Connector> connector) {
    printf("closed_connect_cb\n");
    sendDisConnectDataToHandle(connector);

    // 关闭连接
    connector->data.u32 = 0;
    this->close_connector(connector->getIp(), connector->getPort(), connector->getId(), connector->get_connector_fd());
}

Player* Server::getPlayer(uint32_t playerId) {
    Player* player = playerList.get(playerId);
    return player;
}

void Server::deletePlayer(uint32_t playerId) {
    // 删除用户
    Player* player = playerList.get(playerId);
    if (player != NULL) {
	playerList.del(playerId);
	if (player->gameCode.length() > 0 && player->roomCode.length() > 0 && player->playerId > 0) {
	    GameWorld* gameWorld = getGameWorld(player->gameCode);
	    if (gameWorld != NULL) {
		gameWorld->disConnectPlayer(playerId, player->roomCode);
	    }
	}
	printf("delete player\n");
	delete player;
    }
}

int Server::getPlayerState(uint32_t playerId) {
    Player* player = playerList.get(playerId);
    if (player != NULL) {
	return player->userState;
    }
    return USER_STATE_DIC_CONN;
}

void Server::create_connector_cb(std::shared_ptr<common::net::Connector> connector) {
    printf("new player\n");

    uint32_t uid = createPlayer(connector->getIp(), connector->getPort(), connector->get_connector_fd(), connector->getId());

    connector->data.u32 = uid;
    printf("player id:%u\n", uid);

}


uint32_t Server::createPlayer(std::string ip, int port, int fd, uint32_t connectUid) {

    uint32_t uid = playerList.getUniqId();
    Player* player = new Player();

    player->userState = USER_STATE_WAITING;
    player->playerId = uid;
    player->ip = ip;
    player->port = port;
    player->fd = fd;
    player->connectorUid = connectUid;

    playerList.add(player, uid);
    
    return uid;
}

void Server::parseImp(std::shared_ptr<common::net::Connector> connector) {
    SceNetAdhocctlPacketBase* packet = NULL;
    while((packet = this->parse(connector)) != NULL) {
	common::net::TagRecvData<SceNetAdhocctlPacketBase>* data = new common::net::TagRecvData<SceNetAdhocctlPacketBase>();
	data->uid = connector->getId();
	data->data = packet;
	data->ip = connector->getIp();
	data->port= connector->getPort();
	data->fd = connector->get_connector_fd();
	data->extId = connector->data.u32;
	
	common::net::NetThread<SceNetAdhocctlPacketBase>* netThread = getNetThreadOfFd(connector->get_connector_fd());
        netThread->addRecvList(data);
    }

}
SceNetAdhocctlPacketBase* Server::parse(std::shared_ptr<sails::common::net::Connector> connector) {
    
    if (connector->readable() <= 0) {
	return NULL;
    }
    int read_able = connector->readable();
    SceNetAdhocctlPacketBase *packet = (SceNetAdhocctlPacketBase *)connector->peek();
    if (packet->opcode > PACKET_MAX) {
	connector->retrieve(connector->readable());
	invalid_msg_handle(connector);
	return NULL;
    }

    SceNetAdhocctlPacketBase *packet_new = NULL;
    int packet_len = 0;

    // Ping Packet
    if(packet->opcode == OPCODE_PING)
    {
	packet_len = 1;
	packet_new = (SceNetAdhocctlPacketBase*)malloc(sizeof(SceNetAdhocctlPacketBase));
	memset(packet_new, 0, packet_len);
	memcpy((void *)packet_new, (void *)connector->peek(), packet_len);
    }

    // login
    else if(packet->opcode == OPCODE_LOGIN)
    {
	uint32_t playerId = connector->data.u32;
	if (getPlayerState(playerId) == USER_STATE_WAITING) {
	    	// Enough Data available
	    if(read_able >= sizeof(SceNetAdhocctlLoginPacketC2S))
	    {
		packet_len = sizeof(SceNetAdhocctlLoginPacketC2S);
		packet_new = (SceNetAdhocctlPacketBase*)malloc(packet_len);
		memset(packet_new, 0, packet_len);
		memcpy((void *)packet_new, (void *)connector->peek(), packet_len);
	    }
	}
    }
					
    // Group Connect Packet
    else if(packet->opcode == OPCODE_CONNECT)
    {
	printf("start parse connect data\n");
	// Enough Data available
	printf("user state logged in\n");
	if(read_able >= sizeof(SceNetAdhocctlConnectPacketC2S))
	{
	    printf("new connect packet\n");
	    // Cast Packet
	    packet_len = sizeof(SceNetAdhocctlConnectPacketC2S);
	    packet_new = (SceNetAdhocctlPacketBase*)malloc(packet_len);
	    memset(packet_new, 0, packet_len);
	    memcpy((void *)packet_new, (void *)connector->peek(), packet_len);
	}
    }
					
    // Group Disconnect Packet
    else if(packet->opcode == OPCODE_DISCONNECT)
    {
	packet_len = 1;
	packet_new = (SceNetAdhocctlPacketBase*)malloc(packet_len);
	memset(packet_new, 0, packet_len);
	memcpy((void *)packet_new, (void *)connector->peek(), packet_len);
    }
					
    // Network Scan Packet
    else if(packet->opcode == OPCODE_SCAN)
    {
	packet_len = 1;
	packet_new = (SceNetAdhocctlPacketBase*)malloc(packet_len);
	memset(packet_new, 0, packet_len);
	memcpy((void *)packet_new, (void *)connector->peek(), packet_len);
    }				
    // Chat Text Packet
    else if(packet->opcode == OPCODE_CHAT)
    {
	// Enough Data available
	if(read_able >= sizeof(SceNetAdhocctlChatPacketC2S))
	{
	    packet_len = sizeof(SceNetAdhocctlChatPacketC2S);
	    packet_new = (SceNetAdhocctlPacketBase*)malloc(packet_len);
	    memset(packet_new, 0, packet_len);
	    memcpy((void *)packet_new, (void *)connector->peek(), packet_len);
	}
    }
					
    // game data transfer
    else if (packet->opcode == OPCODE_GAME_DATA) {
	if(read_able >= sizeof(SceNetAdhocctlGameDataPacketC2C)) {
	    // cast Packet
	    SceNetAdhocctlGameDataPacketC2C *packet_raw = (SceNetAdhocctlGameDataPacketC2C*)connector->peek();
	    if(read_able >= (sizeof(SceNetAdhocctlGameDataPacketC2C)+packet_raw->len-1)) {
		packet_len = sizeof(SceNetAdhocctlGameDataPacketC2C)+packet_raw->len-1;
		    
	     
		//first search user by ip and mac
		packet_new = (SceNetAdhocctlPacketBase*)malloc(packet_len);
		memset(packet_new, 0, packet_len);
		memcpy((void *)packet_new, (void *)connector->peek(), packet_len);
		
		std::string ip = connector->getIp();
		SceNetAdhocctlGameDataPacketC2C *temp = (SceNetAdhocctlGameDataPacketC2C*)packet;
	    }
	}
    }
    
// Invalid Opcode
    else
    {
	// Notify User
	std::string ip = connector->getIp();
//	printf("Invalid Opcode 0x%02X in Logged-In State from %s (MAC: %02X:%02X:%02X:%02X:%02X:%02X - IP:%s).\n", user->rx[0], (char *)user->resolver.name.data, user->resolver.mac.data[0], user->resolver.mac.data[1], user->resolver.mac.data[2], user->resolver.mac.data[3], user->resolver.mac.data[4], user->resolver.mac.data[5], ip.c_str());

	invalid_msg_handle(connector);
    }
	
    if (packet_len > 0) {
	connector->retrieve(packet_len);
    }
    if (packet_new != NULL) {
	return packet_new;
    }
    return NULL;
}


Server::~Server() {
    
}











HandleImpl::HandleImpl(sails::common::net::EpollServer<SceNetAdhocctlPacketBase>* server): sails::common::net::HandleThread<SceNetAdhocctlPacketBase>(server) {
    
}


void HandleImpl::handle(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {


    SceNetAdhocctlPacketBase *data = recvData.data;
    uint32_t playerId = recvData.extId;

    switch (data->opcode) {
    case OPCODE_PING: {
	break;
    }
    case OPCODE_LOGIN: {
	printf("get login data\n");
	login_user_data(recvData);
	break;
    }
    case OPCODE_CONNECT: {
	printf("get connect data\n");
	printf("get user state:%d\n", ((Server*)server)->getPlayerState(playerId));
	if (((Server*)server)->getPlayerState(playerId) == USER_STATE_LOGGED_IN) {
	    connect_user(recvData);
	}
	break;
    }
    case OPCODE_DISCONNECT: {
	printf("disconnect\n");
	if (((Server*)server)->getPlayerState(playerId) == USER_STATE_LOGGED_IN) {
	    // Leave Game Gro0x00000000019527a0up
	    disconnect_user(recvData);
	}
	break;
    }
    case OPCODE_SCAN: {
	// Send Network List
	if (((Server*)server)->getPlayerState(playerId) == USER_STATE_LOGGED_IN) {
	    send_scan_results(recvData);
	}
	break;
    }
    case OPCODE_CHAT: {
	// Spread Chat Message
	if (((Server*)server)->getPlayerState(playerId) == USER_STATE_LOGGED_IN) {
	    spread_message(recvData);
	}
	break;
    }

    case OPCODE_GAME_DATA: {
	if (((Server*)server)->getPlayerState(playerId) == USER_STATE_LOGGED_IN) {
	    transfer_message(recvData);
	}
	break;
    }
    default: {
	
    }
	
    }
}


// 登录,对用户,游戏进行校验
void HandleImpl::login_user_data(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {

    SceNetAdhocctlLoginPacketC2S * data = (SceNetAdhocctlLoginPacketC2S*)recvData.data;
    uint32_t playerId = recvData.extId;
    printf("user login\n");
    // 找到对应的游戏

    int valid_product_code = 1;
    // 游戏名称合法
    int i = 0; for(; i < PRODUCT_CODE_LENGTH && valid_product_code == 1; i++){
	// Valid Characters
	if(!((data->game.data[i] >= 'A' && data->game.data[i] <= 'Z') || (data->game.data[i] >= '0' && data->game.data[i] <= '9'))) valid_product_code = 0;
    }
	
    // mac地址合法
    if(valid_product_code == 1 && memcmp(&data->mac, "\xFF\xFF\xFF\xFF\xFF\xFF", sizeof(data->mac)) != 0 && memcmp(&data->mac, "\x00\x00\x00\x00\x00\x00", sizeof(data->mac)) != 0 && data->name.data[0] != 0)
    {

	std::string gameCode = game_product_override(&data->game);
	GameWorld* gameWorld = ((Server*)server)->getGameWorld(gameCode);
	if (gameWorld == NULL) {
	    gameWorld = ((Server*)server)->createGameWorld(gameCode);
	}
	Player* player = ((Server*)server)->getPlayer(playerId);
	if (gameWorld != NULL) {
	    player->mac = HandleImpl::getMacStr(data->mac);
	    player->userState =  USER_STATE_LOGGED_IN;
	    player->gameCode = gameCode;

	    printf("player game code :%s\n", gameCode.c_str());
	    return;
	}
    }

    // 不合法
    // 删除用户
    ((Server*)server)->deletePlayer(playerId);
    server->close_connector(recvData.ip, recvData.port, recvData.uid, recvData.fd);
    
}


void HandleImpl::connect_user(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData){
    uint32_t playerId = recvData.extId;
    SceNetAdhocctlConnectPacketC2S * packet = (SceNetAdhocctlConnectPacketC2S *)recvData.data;
    SceNetAdhocctlGroupName* group = (SceNetAdhocctlGroupName*)&packet->group;

    // room 名称合法性检查
    int valid_group_name = 1;
    {
	// Iterate Characters
	int i = 0; for(; i < ADHOCCTL_GROUPNAME_LEN && valid_group_name == 1; i++){
	    // End of Name
	    if(group->data[i] == 0) break;
	    // A - Z
	    if(group->data[i] >= 'A' && group->data[i] <= 'Z') continue;
	    // a - z
	    if(group->data[i] >= 'a' && group->data[i] <= 'z') continue;
	    // 0 - 9
	    if(group->data[i] >= '0' && group->data[i] <= '9') continue;
	    // Invalid Symbol
	    valid_group_name = 0;
	}
    }
    // Valid Group Name
    if(valid_group_name == 1)
    {
	printf("check roomCode ok\n");
	Player* player = ((Server*)server)->getPlayer(playerId);

	if (player != NULL && player->gameCode.length()> 0) {
	    GameWorld* gameWorld = ((Server*)server)->getGameWorld(player->gameCode);
	    if (gameWorld != NULL) {
		std::string roomCode((char*)group->data, ADHOCCTL_GROUPNAME_LEN);
		gameWorld->connectPlayer(playerId, roomCode);
		return;
	    }
	}
    }

    // 不合法
    logout_user(playerId);
    
}

void HandleImpl::disconnect_user(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
    uint32_t playerId = recvData.extId;
    Player* player = ((Server*)server)->getPlayer(playerId);
    if (player != NULL && player->gameCode.length()> 0) {
	GameWorld* gameWorld = ((Server*)server)->getGameWorld(player->gameCode);
	if (gameWorld != NULL) {
	    gameWorld->disConnectPlayer(playerId,player->roomCode);
	}
    }
}


void HandleImpl::logout_user(uint32_t playerId) {
    Player* player = ((Server*)server)->getPlayer(playerId);
    server->close_connector(player->ip, player->port, player->connectorUid, player->fd);
    ((Server*)server)->deletePlayer(playerId);
}

void HandleImpl::send_scan_results(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
    uint32_t playerId = recvData.extId;
    Player* player = ((Server*)server)->getPlayer(playerId);
    if (player != NULL && player->gameCode.length()> 0) {
	GameWorld* gameWorld = ((Server*)server)->getGameWorld(player->gameCode);
	if (gameWorld != NULL) {
	    std::list<std::string> roomList = gameWorld->getRoomList();
	    for (std::string& roomCode : roomList) {
		// Scan Result Packet
		SceNetAdhocctlScanPacketS2C packet;
		
		// Clear Memory
		memset(&packet, 0, sizeof(packet));
		// Set Opcode
		packet.base.opcode = OPCODE_SCAN;
			
		// Set Group Name
		packet.group = getRoomName(roomCode);
		// Iterate Players in Network Group
		packet.mac = HandleImpl::getMacStruct(gameWorld->getRoomHostMac(roomCode));
			
		// Send Group Packet
		std::string buffer = std::string((char*)&packet, sizeof(packet));
	        server->send(buffer, player->ip, player->port, player->connectorUid, player->fd);
	    }

	    // Notify Player of End of Scan
	    uint8_t opcode = OPCODE_SCAN_COMPLETE;
	    std::string buffer = std::string((char*)&opcode, sizeof(opcode));
	    server->send(buffer, player->ip, player->port, player->connectorUid, player->fd);
	}
    }
}

void HandleImpl::spread_message(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
    uint32_t playerId = recvData.extId;
    Player* player = ((Server*)server)->getPlayer(playerId);

    SceNetAdhocctlChatPacketC2S * packet = (SceNetAdhocctlChatPacketC2S *)recvData.data;
    std::string message(packet->message, 64);
    
    if (player!= NULL && player->gameCode.length() > 0) {
	GameWorld* gameWorld = ((Server*)server)->getGameWorld(player->gameCode);
	if (gameWorld != NULL) {
	    gameWorld->spreadMessage(player->roomCode, message);
	}
    }else {
	//还没有加入游戏
    }
}


void HandleImpl::transfer_message(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
    uint32_t playerId = recvData.extId;
    Player* player = ((Server*)server)->getPlayer(playerId);

//    printf("get transfer message from ip:%s, port :%d\n", player->ip.c_str(), player->port);
    
    SceNetAdhocctlGameDataPacketC2C *packet = (SceNetAdhocctlGameDataPacketC2C*)recvData.data;

    std::string peerIp = HandleImpl::getIpstr(packet->ip);
    std::string peerMac = HandleImpl::getMacStr(packet->dmac);

    packet->ip = HandleImpl::getIp(player->ip);
    std::string message((char*)packet, sizeof(SceNetAdhocctlGameDataPacketC2C)+packet->len-1);
    
    if (player->gameCode.length() > 0) {
	GameWorld* gameWorld = ((Server*)server)->getGameWorld(player->gameCode);
	if (gameWorld != NULL) {
	    gameWorld->transferMessage(player->roomCode, peerIp, peerMac, message);
	}
    }
}




std::string HandleImpl::getMacStr(SceNetEtherAddr& macAddr) {
    char mac[20] = {'\0'};
    sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", macAddr.data[0], macAddr.data[1], macAddr.data[2], macAddr.data[3], macAddr.data[4], macAddr.data[5]);
    return std::string(mac);
}


SceNetEtherAddr HandleImpl::getMacStruct(std::string macstr) {
    SceNetEtherAddr addr;
    int mac[6];
    sscanf(macstr.c_str(), "%02X:%02X:%02X:%02X:%02X:%02X", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

    for (int i = 0; i < 6; i++) {
	addr.data[i] = mac[i];
    }
    return addr;
}



std::string HandleImpl::getIpstr(uint32_t ip) {
    char ipstr[16] = {'\0'};
    uint8_t * ipitem = (uint8_t *)&ip;
    sprintf(ipstr, "%u.%u.%u.%u", ipitem[0], ipitem[1], ipitem[2], ipitem[3]);
    return std::string(ipstr);
}

uint32_t HandleImpl::getIp(std::string ipstr) {
    uint32_t ip = 0;
    uint8_t *ipitem = (uint8_t *)&ip;
    uint32_t ips[4];
    sscanf(ipstr.c_str(), "%u.%u.%u.%u", &ips[0], &ips[1], &ips[2], &ips[3]);
    for (int i = 0; i < 4; i++) {
	ipitem[i] = (uint8_t)ips[i];
    }
    return ip;
}

SceNetAdhocctlGroupName HandleImpl::getRoomName(std::string& name) {
    
    const char *namestr = name.c_str();
    SceNetAdhocctlGroupName roomName;

    int len = ADHOCCTL_GROUPNAME_LEN>name.length()?name.length():ADHOCCTL_GROUPNAME_LEN;
    memset(&roomName, 0, sizeof(roomName));
    for (int i = 0; i < len; i++) {
	roomName.data[i] = namestr[i];
    }
    return roomName;
}


std::string HandleImpl::game_product_override(SceNetAdhocctlProductCode * product) {

    // Safe Product Code
    char productid[PRODUCT_CODE_LENGTH + 1];
	
    // Prepare Safe Product Code
    strncpy(productid, product->data, PRODUCT_CODE_LENGTH);
    productid[PRODUCT_CODE_LENGTH] = 0;
	
    // Crosslinked Flag
    int crosslinked = 0;
	
    // Exists Flag
    int exists = 0;

    std::string crosslink;
    std::string productname(productid);
    std::map<std::string, std::string>::iterator crosslinks_iter = crosslinks_map.find(productname);
    if (crosslinks_iter != crosslinks_map.end()) {
	crosslink = crosslinks_iter->second;
	
	// Log Crosslink
	printf("Crosslinked %s to %s.\n", productid,  crosslink.c_str());
	    
	// Set Crosslinked Flag
	crosslinked = 1;
    }
        
    // Not Crosslinked
    if(!crosslinked)
    {
	std::map<std::string, std::string>::iterator products_iter = products_map.find(std::string(productid));
	if (products_iter != products_map.end()) {
	    // Set Exists Flag
	    exists = 1;
	}
	    
	// Game doesn't exist
	if(!exists)
	{
//		products_map.insert(std::pair<std::string, std::string>(std::string(productid), std::string(productid)));
	}
    }
    if (crosslink.length() > 0) {
	return crosslink;
    }
    return productname;
}


} // namespace sails






////////////////////////////////////////main/////////////////////////////////////////////////
bool isRun = true;
sails::Server server(2);
sails::HandleImpl handle(&server);

void sails_signal_handle(int signo, siginfo_t *info, void *ext) {
    switch(signo) {
	case SIGINT:
	{
	    printf("stop netthread\n");
	    server.stopNetThread();
	    server.stopHandleThread();
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



    server.createEpoll();

//    server.setEmptyConnTimeout(10);
    server.bind(27312);
    server.startNetThread();
    
    server.add_handle(&handle);
    server.startHandleThread();

    while(isRun) {
	sleep(2);
    }
    


    return 0;
}

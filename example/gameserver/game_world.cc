#include "game_world.h"
#include "server.h"


namespace sails {

extern sails::common::log::Logger psplog;

GameWorld::GameWorld(std::string gameCode, Server* server) {
    this->gameCode = gameCode;
    this->roomNum = 0;
    this->server = server;
}

GameWorld::~GameWorld() {
    // 删除所有的room
    std::map<std::string, GameRoom*>::iterator it;
    for (it = roomMap.begin(); it != roomMap.end(); it++) {
	GameRoom* room = it->second;
	if (room != NULL) {
	    delete room;
	}
    }
}

GameRoom* GameWorld::getGameRoom(std::string& roomCode) {
    std::unique_lock<std::mutex> locker(roomMutex);
    GameRoom* room = NULL;
    std::map<std::string, GameRoom*>::iterator it = roomMap.find(roomCode);
    if (it != roomMap.end()) {
	room = it->second;
    }
    return room;
}
GameRoom* GameWorld::createGameRoom(std::string& roomCode) {
    std::unique_lock<std::mutex> locker(roomMutex);

    GameRoom* room = new GameRoom(roomCode, 4, this);
    roomMap.insert(std::pair<std::string, GameRoom*>(roomCode, room));
    roomNum++;
    return room;
}


bool GameWorld::connectPlayer(uint32_t playerId, std::string& roomCode) {
    GameRoom* room = getGameRoom(roomCode);
    if (room == NULL) {
	room = createGameRoom(roomCode);
    }
    return room->connectPlayer(playerId);
}

DisconnectState GameWorld::disConnectPlayer(uint32_t playerId, std::string& roomCode) {
    GameRoom* room = getGameRoom(roomCode);
    if (room == NULL) {
	psplog.error("GameWorld::disConnectPlayer playerId:%u not find room", playerId);
	return STATE_NO_ROOM;
    }
    return room->disConnectPlayer(playerId);
}

std::list<std::string> GameWorld::getRoomList() {
    std::list<std::string> roomList;
    GameRoom* room = NULL;
    std::map<std::string, GameRoom*>::iterator it;
    for (it = roomMap.begin(); it != roomMap.end(); it++) {
	room = it->second;
	if (room != NULL) {
	    roomList.push_back(room->getRoomCode());
	}
    }

    return roomList;
}


std::string GameWorld::getRoomHostMac(std::string& roomCode) {
    std::string mac;
    GameRoom* room = getGameRoom(roomCode);
    if (room != NULL) {
	mac = room->getRoomHostMac();
    }
    return mac;
}



void GameWorld::spreadMessage(std::string& roomCode, std::string& message) {
    std::map<std::string, GameRoom*>::iterator it;
    if (roomCode.length() > 0) {
	// 发送给房间内的用户
	it = roomMap.find(roomCode);
	if (it != roomMap.end()) {
	    GameRoom* room = it->second;
	    if (room != NULL) {
		room->spreadMessage(message);
	    }

	}
    }else {
	// 发送给所有用户
	for (it = roomMap.begin(); it != roomMap.end(); it++) {
	    GameRoom* room = it->second;
	    if (room != NULL) {
		room->spreadMessage(message);
	    }
	}
    }
}


void GameWorld::transferMessage(std::string& roomCode, std::string&ip, std::string& mac, std::string& message) {
    if (roomCode.length() > 0) {
	std::map<std::string, GameRoom*>::iterator it = roomMap.find(roomCode);
	if (it != roomMap.end()) {
	    GameRoom* room = it->second;
	    if (room != NULL) {
		room->transferMessage(ip, mac, message);
	    }

	}
    }
}


std::list<std::string> GameWorld::getSessions() {
    std::list<std::string> sessionList;
    std::map<std::string, GameRoom*>::iterator it;
    for (it = roomMap.begin(); it != roomMap.end(); it++) {
	GameRoom* room = it->second;
	if (room != NULL) {
	    std::list<std::string> roomSessionList = room->getRoomSessions();
	    for(std::string& session: roomSessionList) {
		sessionList.push_back(session);
	    }
	}
    }
    return sessionList;
}

std::list<std::string> getGameWorldList();

std::map<std::string, std::list<std::string>> GameWorld::getPlayerNameMap() {
    std::map<std::string, std::list<std::string>> nameMap;
    std::map<std::string, GameRoom*>::iterator it;
    for (it = roomMap.begin(); it != roomMap.end(); it++) {
	GameRoom* room = it->second;
	if (room != NULL) {
	    std::list<std::string> roomList = room->getPlayerNames();
	    nameMap.insert(std::pair<std::string, std::list<std::string>>(room->getRoomCode(), roomList));
	}
    }
    return nameMap;
}

Server* GameWorld::getServer() {
    return server;
}


} // namesapce sails













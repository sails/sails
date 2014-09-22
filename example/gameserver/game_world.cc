#include "game_world.h"
#include "server.h"


namespace sails {


GameWorld::GameWorld(std::string gameCode, Server* server) {
    this->gameCode = gameCode;
    this->roomNum = 0;
    this->server = server;
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
    return room;
}


bool GameWorld::connectPlayer(uint32_t playerId, std::string& roomCode) {
    GameRoom* room = getGameRoom(roomCode);
    if (room == NULL) {
	room = createGameRoom(roomCode);
    }
    return room->connectPlayer(playerId);
}

void GameWorld::disConnectPlayer(uint32_t playerId, std::string& roomCode) {
    GameRoom* room = getGameRoom(roomCode);
    if (room == NULL) {
	return;
    }
    room->disConnectPlayer(playerId);
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

Server* GameWorld::getServer() {
    return server;
}

} // namesapce sails













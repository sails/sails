// Copyright (C) 2016 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: game_world.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2016-01-15 15:59:03



#include "sails/example/gameserver/game_world.h"
#include <utility>
#include "sails/example/gameserver/server.h"


namespace sails {


GameWorld::GameWorld(std::string gameCode, Server* server)
    : gameCode(gameCode) {
  this->roomNum = 0;
  this->server = server;
}

GameWorld::~GameWorld() {
  // 删除所有的room
  for (auto it = roomMap.begin(); it != roomMap.end(); it++) {
    GameRoom* room = it->second;
    if (room != NULL) {
      delete room;
    }
  }
}

GameRoom* GameWorld::getGameRoom(const std::string& roomCode) {
  std::unique_lock<std::mutex> locker(roomMutex);
  GameRoom* room = NULL;
  auto it = roomMap.find(roomCode);
  if (it != roomMap.end()) {
    room = it->second;
  }
  return room;
}
GameRoom* GameWorld::createGameRoom(const std::string& roomCode) {
  std::unique_lock<std::mutex> locker(roomMutex);

  GameRoom* room = new GameRoom(roomCode, 4, this);
  roomMap.insert(std::pair<std::string, GameRoom*>(roomCode, room));
  roomNum++;
  return room;
}


bool GameWorld::connectPlayer(uint32_t playerId, const std::string& roomCode) {
  GameRoom* room = getGameRoom(roomCode);
  if (room == NULL) {
    room = createGameRoom(roomCode);
  }
  return room->connectPlayer(playerId);
}

DisconnectState GameWorld::disConnectPlayer(
    uint32_t playerId, const std::string& roomCode) {
  GameRoom* room = getGameRoom(roomCode);
  if (room == NULL) {
    ERROR_DLOG("psp",
               "GameWorld::disConnectPlayer playerId:%u not find room",
               playerId);
    return STATE_NO_ROOM;
  }
  return room->disConnectPlayer(playerId);
}

std::list<std::string> GameWorld::getRoomList() {
  std::list<std::string> roomList;
  std::map<std::string, GameRoom*>::iterator it;
  for (it = roomMap.begin(); it != roomMap.end(); ++it) {
    GameRoom* room = it->second;
    if (room != NULL) {
      roomList.push_back(room->getRoomCode());
    }
  }

  return roomList;
}


std::string GameWorld::getRoomHostMac(const std::string& roomCode) {
  std::string mac;
  GameRoom* room = getGameRoom(roomCode);
  if (room != NULL) {
    mac = room->getRoomHostMac();
  }
  return mac;
}



void GameWorld::spreadMessage(const std::string& roomCode,
                              const std::string& message) {
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
  } else {
    // 发送给所有用户
    for (it = roomMap.begin(); it != roomMap.end(); ++it) {
      GameRoom* room = it->second;
      if (room != NULL) {
        room->spreadMessage(message);
      }
    }
  }
}


void GameWorld::transferMessage(
    const std::string& roomCode,
    const std::string&ip,
    const std::string& mac,
    const std::string& message) {
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
  for (it = roomMap.begin(); it != roomMap.end(); ++it) {
    GameRoom* room = it->second;
    if (room != NULL) {
      std::list<std::string> roomSessionList = room->getRoomSessions();
      for (std::string& session : roomSessionList) {
        sessionList.push_back(session);
      }
    }
  }
  return sessionList;
}

std::list<std::string> getGameWorldList();

std::map<std::string, std::list<std::string>> GameWorld::getPlayerNameMap() {
  std::map<std::string, std::list<std::string>> nameMap;
  for (auto it = roomMap.begin(); it != roomMap.end(); it++) {
    GameRoom* room = it->second;
    if (room != NULL) {
      std::list<std::string> roomList = room->getPlayerNames();
      nameMap.insert(std::pair<std::string,
                     std::list<std::string>>(room->getRoomCode(), roomList));
    }
  }
  return nameMap;
}

Server* GameWorld::getServer() {
  return server;
}


}  // namespace sails

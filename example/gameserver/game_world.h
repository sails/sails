// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: game_world.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 17:36:17



#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include <string>
#include <list>
#include <map>
#include "game_room.h"
#include <mutex>
#include "sails/base/constant_ptr_list.h"

namespace sails {

class Server;

class GameWorld {
 public:
  GameWorld(std::string gameCode, Server* server);

  ~GameWorld();

  std::string getGameCode() { return gameCode;}

  GameRoom* getGameRoom(std::string& roomCode);
  GameRoom* createGameRoom(std::string& roomCode);


  bool connectPlayer(uint32_t playerId, std::string& roomCode);
  DisconnectState disConnectPlayer(uint32_t playerId, std::string& roomCode);
    
  std::list<std::string> getRoomList();

  std::string getRoomHostMac(std::string& roomCode);

  void spreadMessage(std::string& roomCode, std::string& message);

  void transferMessage(std::string& roomCode, std::string&ip, std::string& mac, std::string& message);

  std::list<std::string> getSessions();
  std::map<std::string, std::list<std::string>> getPlayerNameMap();
    
  Server* getServer();
 private:
  std::string gameCode;
  int roomNum;
  std::map<std::string, GameRoom*> roomMap;
  std::mutex roomMutex;

  Server* server;
};


} // namespace sails
#endif /* GAME_WORLD_H */














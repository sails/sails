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

  GameRoom* getGameRoom(const std::string& roomCode);
  GameRoom* createGameRoom(const std::string& roomCode);


  bool connectPlayer(uint32_t playerId, const std::string& roomCode);
  DisconnectState disConnectPlayer(uint32_t playerId,
                                   const std::string& roomCode);

  std::list<std::string> getRoomList();

  std::string getRoomHostMac(const std::string& roomCode);

  void spreadMessage(const std::string& roomCode, const std::string& message);

  void transferMessage(const std::string& roomCode,
                       const std::string&ip,
                       const std::string& mac,
                       const std::string& message);

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














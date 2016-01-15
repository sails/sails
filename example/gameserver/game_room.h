// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: game_room.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 17:29:44



#ifndef EXAMPLE_GAMESERVER_GAME_ROOM_H_
#define EXAMPLE_GAMESERVER_GAME_ROOM_H_


#include <string>
#include <map>
#include <mutex>  // NOLINT
#include <list>
#include "sails/example/gameserver/player.h"

namespace sails {

class GameWorld;


class GameRoom {
 public:
  GameRoom(std::string roomCode, int seatNum, GameWorld *gameWorld);

  bool connectPlayer(uint32_t playerId);

  // 因为disconnect之后player才能被删除,所以只要保存disconnect和spreadMessage
  // stransferMessage互斥,就不会出现问题
  DisconnectState disConnectPlayer(uint32_t playerId);

  std::string getRoomCode();

  std::string getRoomHostMac();

  void spreadMessage(const std::string& message);

  void transferMessage(const std::string&ip,
                       const std::string& mac, const std::string& message);

  std::list<std::string> getRoomSessions();

  std::list<std::string> getPlayerNames();

 private:
  std::string roomCode;
  int seatNum;
  int usedNum;
  std::map<uint32_t, Player*> playerMap;
  std::mutex playerMutex;

  GameWorld *gameWorld;
};


}  // namespace sails

#endif  // EXAMPLE_GAMESERVER_GAME_ROOM_H_















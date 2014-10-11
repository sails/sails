// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: player.h
// Description: psp玩家
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 17:21:46



#ifndef PLAYER_H
#define PLAYER_H

#include <string>

namespace sails {

enum DisconnectState {
  STATE_SUCCESS = 0,
  STATE_PLAYER_NOT_EXISTS = 1,
  STATE_PLAYER_INVALID = 2,
  STATE_NO_GAMEWOLD = 3,
  STATE_NO_ROOM = 4,
  STATE_FAILED = -1
};

class Player {
 public:
  Player();

  ~Player();

  static void destroy(Player* player);

 public:
  // 用户状态
  int userState;

  uint32_t playerId;
  std::string playerName;
  std::string roomCode;  // psp游戏里没有用id,而是直接用code
  std::string gameCode;

  std::string ip;
  int port;
  std::string mac;
  int fd;
  uint32_t connectorUid;

  int age;  // 存活年龄,当有用户进入和退出时,age加1

  std::string session;
};


}  // namespace sails


#endif  // PLAYER_H

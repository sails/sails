// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: player.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 17:24:21



#include "player.h"
#include <stdint.h>
#include "game_packets.h"


namespace sails {

Player::Player() {
  playerId = 0;
  age = 0;
  port = 0;
  fd = 0;
  userState = USER_STATE_WAITING;
}

Player::~Player() {
}


void Player::destroy(Player* player) {
  delete player;
}



}  // namespace sails











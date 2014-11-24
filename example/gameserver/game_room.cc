// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: game_room.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 17:31:24



#include "game_room.h"
#include "game_world.h"
#include "server.h"
#include "game_packets.h"
#include <sails/log/logging.h>



namespace sails {

GameRoom::GameRoom(std::string roomCode, int seatNum, GameWorld *gameWorld) {
  this->roomCode = roomCode;
  this->seatNum = seatNum;
  this->gameWorld = gameWorld;
}

bool GameRoom::connectPlayer(uint32_t playerId) {

  log::LoggerFactory::getLogD("psp")->debug("join game:%s, room :%s\n",gameWorld->getGameCode().c_str(), roomCode.c_str());
  // 加入map,再通过其它用户
  std::unique_lock<std::mutex> locker(playerMutex);
    
  Server* server = gameWorld->getServer();
  Player* player = server->GetPlayer(playerId);

  int maxAge = -1;
  if (player != NULL) {

    SceNetEtherAddr playerMac = HandleImpl::getMacStruct(player->mac);
    uint32_t playerIp = HandleImpl::getIp(player->ip);
    // BSSID Packet
    SceNetAdhocctlConnectBSSIDPacketS2C bssid;
    // Set BSSID Opcode
    bssid.base.opcode = OPCODE_CONNECT_BSSID;
    // Set Default BSSID(group host)
    bssid.mac = playerMac;

    log::LoggerFactory::getLogD("psp")->debug("connect room, ip:%s, port:%d, mac:%s\n", player->ip.c_str(), player->port, player->mac.c_str());
	
    // 循环通知玩家
    for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
      Player* peer = iter->second;

      // 发送通过
      SceNetAdhocctlConnectPacketS2C packet;
	    
      // Clear Memory
      memset(&packet, 0, sizeof(packet));
      // Set Connect Opcode
      packet.base.opcode = OPCODE_CONNECT;
      // Set Player Name
      strcpy((char *)packet.name.data, player->playerName.c_str());
	    
      // Set Player MAC
      packet.mac = playerMac;
					
      // Set Player IP
      packet.ip = playerIp;
					
      // Send Data
      // 通知对方
      log::LoggerFactory::getLogD("psp")->debug("notify other side\n");
      std::string buffer((char*)&packet, sizeof(packet));
      gameWorld->getServer()->send(buffer, peer->ip, peer->port, peer->connectorUid, peer->fd);
	    
      // Set Player Name
      strcpy((char *)packet.name.data, peer->playerName.c_str());
	    
      // Set Player MAC
      packet.mac = HandleImpl::getMacStruct(peer->mac);
					
      // Set Player IP
      packet.ip = HandleImpl::getIp(peer->ip);
					
      // 通知自己
      std::string buffer2((char*)&packet, sizeof(packet));
      gameWorld->getServer()->send(buffer2, player->ip, player->port, player->connectorUid, player->fd);


      // 最先加入的当这个组的host
      if (peer->age > maxAge) {
        bssid.mac = HandleImpl::getMacStruct(peer->mac);
        maxAge = peer->age;
      }
    }
	
    // Send Network BSSID to User
    std::string buffer((char*)&bssid, sizeof(bssid));
    gameWorld->getServer()->send(buffer, player->ip, player->port, player->connectorUid, player->fd);
	

    playerMap.insert(std::pair<uint32_t, Player*>(playerId, player));

	
    for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
      Player* peer = iter->second;
      if (peer != NULL) {
        peer->age++;
      }
    }
    player->roomCode = roomCode;
    log::LoggerFactory::getLogD("psp")->debug("user connect group\n");

    return true;
  }
  return false;
}

DisconnectState GameRoom::disConnectPlayer(uint32_t playerId) {
    
  std::unique_lock<std::mutex> locker(playerMutex);

  std::map<uint32_t, Player*>::iterator playerIter = playerMap.find(playerId);
  if (playerIter == playerMap.end()) {
    log::LoggerFactory::getLogD("psp")->error("GameRoom::disConnectPlayer playerId:%u not finded", playerId);
    return STATE_PLAYER_NOT_EXISTS;
  }
  Player* player = playerIter->second;
  if (player->roomCode.length() == 0 || player->gameCode.length() == 0) {
    log::LoggerFactory::getLogD("psp")->error("GameRoom::disConnectPlayer playerId:%u not invalid roomCode or gameCode", playerId);
    return STATE_PLAYER_INVALID;
  }
    
  player->roomCode = "";
  //  player->gameCode = "";
  SceNetEtherAddr playerMac = HandleImpl::getMacStruct(player->mac);
  uint32_t playerIp = HandleImpl::getIp(player->ip);
  playerMap.erase(playerId);


  // 向其它玩家发送退出通知
  for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
    Player* peer = iter->second;

    // 发送通过
    SceNetAdhocctlDisconnectPacketS2C packet;
	    
    // Clear Memory
    memset(&packet, 0, sizeof(packet));
    // Set Connect Opcode
    packet.base.opcode = OPCODE_DISCONNECT;
    // Set Player MAC
    packet.mac = playerMac;
					
    // Set Player IP
    packet.ip = playerIp;
					
    // Send Data
    // 通知对方
    std::string buffer = std::string((char*)&packet, sizeof(packet));
    gameWorld->getServer()->send(buffer, peer->ip, peer->port, peer->connectorUid, peer->fd);
	    
  }
  return STATE_SUCCESS;
}



std::string GameRoom::getRoomCode() {
  return roomCode;
}


std::string GameRoom::getRoomHostMac() {
  std::string mac;
  int maxAge = 0;
  for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
    Player* peer = iter->second;
    if (peer != NULL) {
      if (maxAge < peer->age) {
        mac = peer->mac;
      }
    }
  }
  return mac;
}


void GameRoom::spreadMessage(const std::string& message) {
  std::unique_lock<std::mutex> locker(playerMutex);
  for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
    Player* peer = iter->second;
    if (peer != NULL) {
      // 通知对方
      gameWorld->getServer()->send(message, peer->ip, peer->port, peer->connectorUid, peer->fd);
    }
  }
}


void GameRoom::transferMessage(const std::string&ip,
                               const std::string& mac,
                               const std::string& message) {
  std::unique_lock<std::mutex> locker(playerMutex);
  for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
    Player* peer = iter->second;
    if (peer != NULL) {
      if (peer->ip == ip && peer->mac == mac) {
        gameWorld->getServer()->send(message, peer->ip, peer->port, peer->connectorUid, peer->fd);
      }
    }
  }
}



std::list<std::string> GameRoom::getRoomSessions() {
  std::unique_lock<std::mutex> locker(playerMutex);
  std::list<std::string> sessions;
  for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
    Player* peer = iter->second;
    if (peer != NULL) {
      if (peer->session.length() > 0) {
        sessions.push_back(peer->session);
      }
    }
  }
  return sessions;
}


std::list<std::string> GameRoom::getPlayerNames() {
  std::unique_lock<std::mutex> locker(playerMutex);
  std::list<std::string> names;
  for (std::map<uint32_t, Player*>::iterator iter = playerMap.begin(); iter != playerMap.end(); iter++) {
    Player* peer = iter->second;
    if (peer != NULL) {
      names.push_back(peer->playerName);
    }
  }
  return names;
}


} // namespace sails











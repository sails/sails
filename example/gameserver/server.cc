// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: server.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 17:50:59



#include "sails/example/gameserver/server.h"
#include <signal.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <utility>
#include "sails/example/gameserver/product.h"
#include "sails/base/json.hpp"
#include "sails/base/string.h"
#include "sails/log/logging.h"


namespace sails {

sails::Config config("./gameserver.json");


Server::Server()
    : sails::net::EpollServer<SceNetAdhocctlPacketBase>() {
  playerList.init(50);
}


// 获取游戏
GameWorld* Server::GetGameWorld(const std::string& gameCode) {
  std::unique_lock<std::mutex> locker(gameworldMutex);;
  std::map<std::string, GameWorld*>::iterator iter
      = gameWorldMap.find(gameCode);
  if (iter != gameWorldMap.end()) {
    GameWorld* gameWorld = iter->second;
    return gameWorld;
  } else {
    return NULL;
  }
}

// 创建游戏
GameWorld* Server::CreateGameWorld(const std::string& gameCode) {
  std::unique_lock<std::mutex> locker(gameworldMutex);;
  GameWorld* gameWorld = new GameWorld(gameCode, this);
  gameWorldMap.insert(
      std::pair<std::string, GameWorld*>(gameWorld->getGameCode(), gameWorld));
  return gameWorld;
}

void Server::InvalidMsgHandle(
    std::shared_ptr<sails::net::Connector> connector) {
  WARN_LOG("psp", "invalid msg handle:%u", connector->data.u32);
  KillOffPlayer(connector->data.u32);
}

void Server::ClosedConnectCB(std::shared_ptr<net::Connector> connector,
                             CloseConnectorReason) {
  INFO_LOG("psp", "closed_connect_cb");
  uint32_t playerId = connector->data.u32;
  if (playerId <= 0) {
    return;
  }

  DEBUG_LOG("psp", "sendDisConnectDataToHandle playerId:%u", playerId);
  // 向handle线程发送消息
  SceNetAdhocctlDisconnectPacketS2C* disdata
      = reinterpret_cast<SceNetAdhocctlDisconnectPacketS2C*>(malloc(
          sizeof(SceNetAdhocctlDisconnectPacketS2C)));
  disdata->base.opcode = OPCODE_LOGOUT;
  disdata->ip = Server::getIp(connector->getIp());
  disdata->mac = Server::getMacStruct("EE:EE:EE:EE:EE");

  net::TagRecvData<SceNetAdhocctlPacketBase> *data
      = new net::TagRecvData<SceNetAdhocctlPacketBase>();
  data->uid = playerId;
  data->data = (sails::SceNetAdhocctlPacketBase*)disdata;
  data->ip = connector->getIp();
  data->port = connector->getPort();
  data->fd = connector->get_connector_fd();
  data->extdata = connector->data;

  InsertRecvData(data);
}

void Server::KillOffPlayer(uint32_t playerId) {
  Player* player = playerList.get(playerId);
  if (player != NULL) {
    CloseConnector(
        player->ip, player->port, player->connectorUid, player->fd,
        CloseConnectorReason::SERVER_CLOSED);
  }
}

Player* Server::GetPlayer(uint32_t playerId) {
  Player* player = playerList.get(playerId);
  return player;
}

int Server::GetPlayerState(uint32_t playerId) {
  Player* player = playerList.get(playerId);
  if (player != NULL) {
    return player->userState;
  }
  return USER_STATE_DIC_CONN;
}

void Server::CreateConnectorCB(std::shared_ptr<net::Connector> connector) {
  //    printf("new player\n");

  uint32_t uid = CreatePlayer(connector->getIp(),
                              connector->getPort(),
                              connector->get_connector_fd(),
                              connector->getId());

  connector->data.u32 = uid;
}


uint32_t Server::CreatePlayer(
    std::string ip, int port, int fd, uint32_t connectUid) {

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

SceNetAdhocctlPacketBase* Server::Parse(
    std::shared_ptr<sails::net::Connector> connector) {

  if (connector->readable() <= 0) {
    return NULL;
  }
  uint32_t read_able = connector->readable();
  const SceNetAdhocctlPacketBase *packet
      = reinterpret_cast<const SceNetAdhocctlPacketBase*>(connector->peek());
  if (packet->opcode > PACKET_MAX) {
    connector->retrieve(connector->readable());
    InvalidMsgHandle(connector);
    return NULL;
  }

  SceNetAdhocctlPacketBase *packet_new = NULL;
  int packet_len = 0;

  // Ping Packet
  if (packet->opcode == OPCODE_PING) {
    packet_len = 1;
    packet_new = reinterpret_cast<SceNetAdhocctlPacketBase*>(malloc(
        sizeof(SceNetAdhocctlPacketBase)));
    memcpy(packet_new, connector->peek(), packet_len);
  } else if (packet->opcode == OPCODE_LOGIN) {
    // login
    uint32_t playerId = connector->data.u32;
    if (GetPlayerState(playerId) == USER_STATE_WAITING) {
      // Enough Data available
      if (read_able >= sizeof(SceNetAdhocctlLoginPacketC2S)) {
        packet_len = sizeof(SceNetAdhocctlLoginPacketC2S);
        packet_new = reinterpret_cast<SceNetAdhocctlPacketBase*>(
            malloc(packet_len));
        memcpy(packet_new, connector->peek(), packet_len);
      }
    }
  } else if (packet->opcode == OPCODE_CONNECT) {
    // Group Connect Packet

    // Enough Data available
    INFO_LOG("psp", "user state logged in");
    if (read_able >= sizeof(SceNetAdhocctlConnectPacketC2S)) {
      // Cast Packet
      packet_len = sizeof(SceNetAdhocctlConnectPacketC2S);
      packet_new = reinterpret_cast<SceNetAdhocctlPacketBase*>(
          malloc(packet_len));
      memcpy(packet_new, connector->peek(), packet_len);
    }
  } else if (packet->opcode == OPCODE_DISCONNECT) {
    // Group Disconnect Packet

    packet_len = 1;
    packet_new = reinterpret_cast<SceNetAdhocctlPacketBase*>(
        malloc(packet_len));
    memcpy(packet_new, connector->peek(), packet_len);
  } else if (packet->opcode == OPCODE_SCAN) {
    // Network Scan Packet
    packet_len = 1;
    packet_new = reinterpret_cast<SceNetAdhocctlPacketBase*>(
        malloc(packet_len));
    memcpy(packet_new, connector->peek(), packet_len);
  } else if (packet->opcode == OPCODE_CHAT) {
    // Chat Text Packet

    // Enough Data available
    if (read_able >= sizeof(SceNetAdhocctlChatPacketC2S)) {
      packet_len = sizeof(SceNetAdhocctlChatPacketC2S);
      packet_new = reinterpret_cast<SceNetAdhocctlPacketBase*>(
          malloc(packet_len));
      memcpy(packet_new, connector->peek(), packet_len);
    }
  } else if (packet->opcode == OPCODE_GAME_DATA) {
    // game data transfer

    if (read_able >= sizeof(SceNetAdhocctlGameDataPacketC2C)) {
      // cast Packet
      const SceNetAdhocctlGameDataPacketC2C *packet_raw
          = reinterpret_cast<const SceNetAdhocctlGameDataPacketC2C*>(
              connector->peek());
      if (packet_raw->len >0 && packet_raw->len < 4000) {
        if (read_able >= (sizeof(
                SceNetAdhocctlGameDataPacketC2C)+packet_raw->len-1)) {
          packet_len = sizeof(
              SceNetAdhocctlGameDataPacketC2C)+packet_raw->len-1;


          // first search user by ip and mac
          packet_new = reinterpret_cast<SceNetAdhocctlPacketBase*>(
              malloc(packet_len));
          memcpy(packet_new, connector->peek(), packet_len);
        }
      } else {  // len > 4000, erro msg
        packet_len = read_able;
        ERROR_LOG("psp", "game transfer data content len:%d", packet_raw->len);
      }
    }
  } else {
    // Invalid Opcode

    // Notify User
    ERROR_LOG("psp", "recv invalid opcode data:%d", packet->opcode);
    connector->retrieve(connector->readable());
    InvalidMsgHandle(connector);
  }

  if (packet_len > 0) {
    connector->retrieve(packet_len);
  }
  if (packet_new != NULL) {
    return packet_new;
  }
  return NULL;
}


std::list<std::string> Server::GetPlayerSession() {
  std::list<std::string> sessions;
  std::map<std::string, GameWorld*>::iterator iter;
  for (iter = gameWorldMap.begin(); iter != gameWorldMap.end(); ++iter) {
    if (iter->second != NULL) {
      GameWorld* game = iter->second;
      std::list<std::string> gameSessions = game->getSessions();
      for (std::string& session : gameSessions) {
        sessions.push_back(session);
      }
    }
  }
  return sessions;
}


std::list<std::string> Server::GetGameWorldList() {
  std::list<std::string> gameCodeList;
  for (auto game : gameWorldMap) {
    gameCodeList.push_back(game.first);
  }
  return gameCodeList;
}

std::map<std::string, std::list<std::string>> Server::GetPlayerNameMap(
    const std::string& gameCode) {
  std::map<std::string, std::list<std::string>> map;
  GameWorld* game = GetGameWorld(gameCode);
  if (game != NULL) {
    map = game->getPlayerNameMap();
  }
  return map;
}

Server::~Server() {
  // 删除所有gameworld
  for (auto game : gameWorldMap) {
    delete game.second;
  }
  //删除所有用户
  playerList.clear();
}











void Server::handle(
    const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {

  SceNetAdhocctlPacketBase *data = recvData.data;
  uint32_t playerId = recvData.extdata.u32;

  switch (data->opcode) {
    case OPCODE_PING: {
      break;
    }
    case OPCODE_LOGIN: {
      INFO_LOG("psp", "get login data");
      login_user_data(recvData);
      break;
    }
    case OPCODE_CONNECT: {
      INFO_LOG("psp", "get connect data");
      if (GetPlayerState(playerId)
          == USER_STATE_LOGGED_IN) {
        connect_user(recvData);
      }
      break;
    }
    case OPCODE_DISCONNECT: {
      INFO_LOG("psp", "disconnect");
      if (GetPlayerState(playerId)
          == USER_STATE_CONNECTED_ROOM) {
        // Leave Game
        DisconnectState disconnectState = disconnect_user(recvData);
        if (disconnectState != STATE_SUCCESS) {
          ERROR_LOG("psp", "disconnect from game room error:%d",
                    disconnectState);
        }
      }
      break;
    }
    case OPCODE_LOGOUT: {
      INFO_LOG("psp", "logout");
      if (GetPlayerState(playerId)
          == USER_STATE_CONNECTED_ROOM) {
        // Leave Game
        DisconnectState disconnectState = disconnect_user(recvData);
        if (disconnectState != STATE_SUCCESS) {
          ERROR_LOG("psp", "disconnect from game room error:%d",
                    disconnectState);
        }
      }
      Player* player = GetPlayer(playerId);
      if (player != NULL) {
        playerList.del(playerId);
        delete player;
      }
      /*
        logout_user(playerId);
      */
      break;
    }
    case OPCODE_SCAN: {
      // Send Network List
      if (GetPlayerState(playerId)
          == USER_STATE_LOGGED_IN) {
        send_scan_results(recvData);
      }
      break;
    }
    case OPCODE_CHAT: {
      // Spread Chat Message
      if (GetPlayerState(playerId)
          == USER_STATE_LOGGED_IN) {
        spread_message(recvData);
      }
      break;
    }

    case OPCODE_GAME_DATA: {
      if (GetPlayerState(playerId)
          == USER_STATE_CONNECTED_ROOM) {
        transfer_message(recvData);
      }
      break;
    }
    default: {
    }
  }
}


// 登录,对用户,游戏进行校验
void Server::login_user_data(
    const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {

  SceNetAdhocctlLoginPacketC2S * data
      = reinterpret_cast<SceNetAdhocctlLoginPacketC2S*>(recvData.data);
  uint32_t playerId = recvData.extdata.u32;
  // 找到对应的游戏

  int valid_product_code = 1;
  // 游戏名称合法
  int i = 0;
  for (; i < PRODUCT_CODE_LENGTH && valid_product_code == 1; ++i) {
    // Valid Characters
    if (!((data->game.data[i] >= 'A' && data->game.data[i] <= 'Z')
          || (data->game.data[i] >= '0'
              && data->game.data[i] <= '9'))) valid_product_code = 0;
  }

  // mac地址合法
  if (valid_product_code == 1
      && memcmp(&data->mac,
                "\xFF\xFF\xFF\xFF\xFF\xFF", sizeof(data->mac)) != 0
      && memcmp(&data->mac,
                "\x00\x00\x00\x00\x00\x00", sizeof(data->mac)) != 0
      && data->name.data[0] != 0) {
    // session 校验,为了让它不阻塞主逻辑
    // 这里新建一个线程支校验,当不通过时,向handle线程发送一条disconnect命令
    std::string ip = recvData.ip;
    // session 32位
    if (data->session[0] != '\0' && data->session[32] == '\0') {
      std::string session(data->session);
      if ( session.length() == 32 ) {
        std::thread sessionCheckThread(&Server::player_session_check,
                                       this, playerId, session);
        sessionCheckThread.detach();
        std::string gameCode = game_product_override(&data->game);
        GameWorld* gameWorld = GetGameWorld(gameCode);
        if (gameWorld == NULL) {
          gameWorld = CreateGameWorld(gameCode);
        }
        Player* player = GetPlayer(playerId);
        if (gameWorld != NULL) {
          player->mac = Server::getMacStr(data->mac);
          player->userState =  USER_STATE_LOGGED_IN;
          player->gameCode = gameCode;
          player->session = session;

          INFO_LOG("psp", "player game code :%s", gameCode.c_str());
          return;
        }
      } else {
        ERROR_LOG("psp", "playerId:%u, ip:%s, port:%d session invalid:%s",
                  playerId, ip.c_str(), recvData.port, session.c_str());
      }
    } else {
      ERROR_LOG("psp", "playerId:%u, ip:%s, port:%d,"
                " but session invalid session[0] equal 0 or "
                "session[32] not 0 ", playerId, ip.c_str(), recvData.port);
    }
  } else {
    DEBUG_LOG("psp", "gamecode or mac invalid");
  }
  // 不合法
  KillOffPlayer(playerId);
}


void Server::connect_user(
    const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
  uint32_t playerId = recvData.extdata.u32;
  SceNetAdhocctlConnectPacketC2S * packet
      = reinterpret_cast<SceNetAdhocctlConnectPacketC2S*>(recvData.data);
  SceNetAdhocctlGroupName* group
      = reinterpret_cast<SceNetAdhocctlGroupName*>(&packet->group);

  // room 名称合法性检查
  int valid_group_name = 1;
  {
    // Iterate Characters, ADHOCCTL_GROUPNAME_LEN长度限制
    int i = 0;
    for (; i < ADHOCCTL_GROUPNAME_LEN && valid_group_name == 1; ++i) {
      // End of Name
      if (group->data[i] == 0) break;
      // A - Z
      if (group->data[i] >= 'A' && group->data[i] <= 'Z') continue;
      // a - z
      if (group->data[i] >= 'a' && group->data[i] <= 'z') continue;
      // 0 - 9
      if (group->data[i] >= '0' && group->data[i] <= '9') continue;
      // Invalid Symbol
      valid_group_name = 0;
    }
  }
  // Valid Group Name
  if (valid_group_name == 1) {
    Player* player = GetPlayer(playerId);

    if (player != NULL && player->gameCode.length()> 0) {
      GameWorld* gameWorld
          = GetGameWorld(player->gameCode);
      if (gameWorld != NULL) {
        std::string roomCode(reinterpret_cast<char*>(group->data),
                             ADHOCCTL_GROUPNAME_LEN);
        if (gameWorld->connectPlayer(playerId, roomCode)) {
          player->userState = USER_STATE_CONNECTED_ROOM;
        }
        return;
      }
    }
  } else {
    Player* player = GetPlayer(playerId);
    ERROR_LOG("psp",
              "playerId %u invalid_group_name %s, ip:%s, port:%d, mac:%s",
              playerId, group->data, player->ip.c_str(),
              player->port, player->mac.c_str());
  }

  // 不合法
  KillOffPlayer(playerId);
}

DisconnectState Server::disconnect_user(
    const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
  uint32_t playerId = recvData.extdata.u32;
  Player* player = GetPlayer(playerId);
  if (player == NULL) {
    return STATE_PLAYER_NOT_EXISTS;
  }

  if (player != NULL && player->gameCode.length() > 0
      && player->roomCode.length() >0
      && player->ip == recvData.ip
      && player->port == recvData.port) {
    GameWorld* gameWorld = GetGameWorld(player->gameCode);
    if (gameWorld != NULL) {
      return gameWorld->disConnectPlayer(playerId, player->roomCode);
    } else {
      ERROR_LOG("psp",
                "call disconnect_user but playerId:%u not find gamewold",
                playerId);
      return STATE_NO_GAMEWOLD;
    }
  } else {
    ERROR_LOG("psp", "call disconnect_user but playerId:%u not exists",
              playerId);
    return STATE_PLAYER_INVALID;
  }
}


void Server::logout_user(uint32_t playerId) {
  DEBUG_LOG("psp", "call logout use playerId:%u", playerId);
  Player* player = GetPlayer(playerId);
  if (player != NULL) {
    playerList.del(playerId);
    if (player->gameCode.length() > 0
        && player->roomCode.length() > 0 && player->playerId > 0) {
      GameWorld* gameWorld = GetGameWorld(player->gameCode);
      if (gameWorld != NULL) {
        gameWorld->disConnectPlayer(playerId, player->roomCode);
      }
    }
    INFO_LOG("psp", "delete player");
    delete player;
  }
}

void Server::send_scan_results(
    const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
  uint32_t playerId = recvData.extdata.u32;
  Player* player = GetPlayer(playerId);
  if (player != NULL && player->gameCode.length()> 0) {
    GameWorld* gameWorld
        = GetGameWorld(player->gameCode);
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
        packet.mac = Server::getMacStruct(gameWorld->getRoomHostMac(roomCode));

        // Send Group Packet
        std::string buffer
            = std::string((char*)&packet, sizeof(packet));
        send(
            buffer, player->ip, player->port,
            player->connectorUid, player->fd);
      }

      // Notify Player of End of Scan
      uint8_t opcode = OPCODE_SCAN_COMPLETE;
      std::string buffer
          = std::string((char*)&opcode, sizeof(opcode));
      send(
          buffer, player->ip, player->port,
          player->connectorUid, player->fd);
    }
  }
}

void Server::spread_message(
    const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
  uint32_t playerId = recvData.extdata.u32;
  Player* player = GetPlayer(playerId);

  SceNetAdhocctlChatPacketC2S * packet
      = reinterpret_cast<SceNetAdhocctlChatPacketC2S*>(recvData.data);
  std::string message(packet->message, 64);

  if (player!= NULL && player->gameCode.length() > 0) {
    GameWorld* gameWorld
        = GetGameWorld(player->gameCode);
    if (gameWorld != NULL) {
      gameWorld->spreadMessage(player->roomCode, message);
    }
  } else {
    //还没有加入游戏
  }
}


void Server::transfer_message(
    const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData) {
  uint32_t playerId = recvData.extdata.u32;
  Player* player = GetPlayer(playerId);

  // printf("get transfer message from ip:%s, port :%d\n",
  // player->ip.c_str(), player->port);

  SceNetAdhocctlGameDataPacketC2C *packet
      = reinterpret_cast<SceNetAdhocctlGameDataPacketC2C*>(recvData.data);

  std::string peerIp = Server::getIpstr(packet->ip);
  std::string peerMac = Server::getMacStr(packet->dmac);

  packet->ip = Server::getIp(player->ip);

  int totallen = sizeof(SceNetAdhocctlGameDataPacketC2C)+packet->len-1;

  std::string message(reinterpret_cast<char*>(packet), totallen);

  if (player->gameCode.length() > 0) {
    GameWorld* gameWorld
        = GetGameWorld(player->gameCode);
    if (gameWorld != NULL) {
      gameWorld->transferMessage(player->roomCode, peerIp, peerMac, message);
    }
  }
}



void Server::player_session_check(
    uint32_t playerId, std::string session) {
  if ( !check_session(session) ) {
    WARN_LOG("psp", "player:%u session:%s check error",
             playerId, session.c_str());
    KillOffPlayer(playerId);
  }
}



std::string Server::getMacStr(const SceNetEtherAddr& macAddr) {
  char mac[20] = {'\0'};
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
           macAddr.data[0], macAddr.data[1],
           macAddr.data[2], macAddr.data[3],
           macAddr.data[4], macAddr.data[5]);
  return std::string(mac);
}


SceNetEtherAddr Server::getMacStruct(std::string macstr) {
  SceNetEtherAddr addr;
  int mac[6];
  sscanf(macstr.c_str(),
         "%02X:%02X:%02X:%02X:%02X:%02X",
         &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

  for (int i = 0; i < 6; i++) {
    addr.data[i] = mac[i];
  }
  return addr;
}



std::string Server::getIpstr(uint32_t ip) {
  char ipstr[16] = {'\0'};
  uint8_t * ipitem = reinterpret_cast<uint8_t *>(&ip);
  snprintf(ipstr, sizeof(ipstr), "%u.%u.%u.%u",
           ipitem[0], ipitem[1], ipitem[2], ipitem[3]);
  return std::string(ipstr);
}

uint32_t Server::getIp(std::string ipstr) {
  uint32_t ip = 0;
  uint8_t *ipitem = reinterpret_cast<uint8_t *>(&ip);
  uint32_t ips[4];
  sscanf(ipstr.c_str(), "%3u.%3u.%3u.%3u",
         &ips[0], &ips[1], &ips[2], &ips[3]);
  for (int i = 0; i < 4; i++) {
    ipitem[i] = (uint8_t)ips[i];
  }
  return ip;
}

SceNetAdhocctlGroupName Server::getRoomName(const std::string& name) {
  const char *namestr = name.c_str();
  SceNetAdhocctlGroupName roomName;

  int len = ADHOCCTL_GROUPNAME_LEN >
            name.length()?name.length():ADHOCCTL_GROUPNAME_LEN;
  memset(&roomName, 0, sizeof(roomName));
  for (int i = 0; i < len; i++) {
    roomName.data[i] = namestr[i];
  }
  return roomName;
}


std::string Server::game_product_override(
    SceNetAdhocctlProductCode * product) {

  // Safe Product Code
  char productid[PRODUCT_CODE_LENGTH + 1];

  // Prepare Safe Product Code
  strncpy(productid, product->data, PRODUCT_CODE_LENGTH);
  productid[PRODUCT_CODE_LENGTH] = 0;

  // Crosslinked Flag
  int crosslinked = 0;

  // Exists Flag

  std::string crosslink;
  std::string productname(productid);
  std::map<std::string, std::string>::iterator crosslinks_iter
      = crosslinks_map.find(productname);
  if (crosslinks_iter != crosslinks_map.end()) {
    crosslink = crosslinks_iter->second;

    // Log Crosslink
    INFO_LOG("psp", "Crosslinked %s to %s.", productid,  crosslink.c_str());

    // Set Crosslinked Flag
    crosslinked = 1;
  }

  // Not Crosslinked
  if (!crosslinked) {
    std::map<std::string, std::string>::iterator products_iter
        = products_map.find(std::string(productid));
    if (products_iter != products_map.end()) {
    } else {
      // Game doesn't exist
      // products_map.insert(std::pair<std::string,
      // std::string>(std::string(productid), std::string(productid)));
      WARN_LOG("psp", "game %s doesn't exist", productid);
    }
  }
  if (crosslink.length() > 0) {
    return crosslink;
  }
  return productname;
}








///////////////////// session /////////////////////////////////////////


struct ptr_string {
  char *ptr;
  size_t len;
};

size_t read_callback(
    void *buffer, size_t size, size_t nmemb, void *userp) {
  struct ptr_string *data = (struct ptr_string*)userp;
  size_t new_len = data->len + size*nmemb;
  char *ptr = reinterpret_cast<char *>(malloc(new_len+1));
  //  memset(ptr, 0, new_len+1);
  memcpy(ptr, data->ptr, data->len);
  memcpy(ptr+data->len, buffer, size*nmemb);
  free(data->ptr);
  data->ptr = ptr;
  data->len = new_len;
  data->ptr[new_len] = '\0';
  return size*nmemb;
}

bool post_message(const char* url, const char* data, std::string &result) {
  bool ret = false;
  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl = curl_easy_init();
  if (curl != NULL) {
    struct ptr_string login_result;
    login_result.len = 0;
    login_result.ptr = NULL;


    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &login_result);

    CURLcode res;
    res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
      result += std::string(login_result.ptr, login_result.len);
      ret = true;
    } else {
      INFO_LOG("psp", "post url:%s", url);
    }

    if (login_result.len > 0 && login_result.ptr != NULL) {
      login_result.len = 0;
      free(login_result.ptr);
      login_result.ptr = NULL;
    }
  }

  curl_easy_cleanup(curl);
  curl_global_cleanup();
  return ret;
}


bool check_session(std::string session) {
  if (config.get_store_api_url() == "test") {
    return true;  // 测试
  }
  std::string url = config.get_store_api_url() + "/session";
  int port = config.get_listen_port();
  std::string ip = config.get_local_ip();
  char param[100] = {'\0'};
  if (ip.compare("127.0.0.1") == 0) {
    ERROR_LOG("psp", "server ip config error");
    return false;
  }
  std::string result;
  snprintf(param, sizeof(param), "method=CHECK&ip=%s&port=%d&session=%s",
           ip.c_str(), port, session.c_str());

  DEBUG_LOG("psp", "post param:%s", param);
  if ( post_message(url.c_str(), param, result) ) {
    DEBUG_LOG("psp", "get check infor:%s", result.c_str());
    try {
      auto root = nlohmann::json::parse(result);
      if (!root["status"].empty()) {
        int status = root["status"];
        if (status == 0) {  // call right
          if (!root["message"].empty()) {
            int in_room = root["message"][session];
            if (in_room == 1) {
              return true;
            }
          }
        }
      }
    } catch (...) {
      INFO_LOG("psp", "check session exception");
    }
  }
  return false;;
}


bool update_session_timeout(const std::string& session) {
  if (config.get_store_api_url() == "test") {
    return true;  // 测试
  }
  std::string url = config.get_store_api_url()+"/session";
  char param[100] = {'\0'};
  std::string result;
  snprintf(param, sizeof(param), "method=REFRESH&session=%s",
           session.c_str());

  DEBUG_LOG("psp", "post param:%s", param);
  if ( post_message(url.c_str(), param, result) ) {
    DEBUG_LOG("psp", "get update session infor:%s", result.c_str());
    auto root = nlohmann::json::parse(result);
    if (!root["status"].empty()) {
      int status = root["status"];
      if (status == 0) {  // call right
      } else {
        ERROR_LOG("psp", "update session timeout and return error");
      }
    }
  }
  return true;
}

void update_sessions(const std::list<std::string>& sessions) {
  for (const std::string& session : sessions) {
    update_session_timeout(session);
    // printf("updae session :%s\n", session.c_str());
  }
}

////////////////////////// session /////////////////////////////

}  // namespace sails






////////////////////////// main /////////////////////////////////
bool isRun = true;


void sails_signal_handle(int signo,
                         siginfo_t *, void *) {
  switch (signo) {
    case SIGINT:
      {
        isRun = false;
      }
  }
}

int main(int argc, char *argv[]) {
  // signal kill
  struct sigaction act;
  act.sa_sigaction = sails_signal_handle;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(SIGINT, &act, NULL) == -1) {
    perror("sigaction error");
    exit(EXIT_FAILURE);
  }

  sails::Server server;
  server.Init(sails::config.get_listen_port(), 2, 10, 1);

  // 统计
  time_t lastUpdteSession = 0;
  time_t lastLogStatTime = 0;
  time_t now = 0;
  while (isRun) {
    now = time(NULL);


    // 更新session
    if (now - lastUpdteSession >= 30) {  // 30s
      lastUpdteSession = now;
      std::list<std::string> sessions = server.GetPlayerSession();
      INFO_LOG("psp", "playerNum:%d", sessions.size());
      std::thread t(sails::update_sessions, sessions);
      t.detach();
    }

    // 统计
    if (now - lastLogStatTime >= 60) {  // 60s
      lastLogStatTime = now;
      std::list<std::string> gameList = server.GetGameWorldList();
      for (std::string& gameCode : gameList) {
        std::map<std::string, std::list<std::string>> roomsMap
            = server.GetPlayerNameMap(gameCode);
        for (auto roominfo : roomsMap) {
          INFO_LOG("psp", "game %s, room name %s, playerNum:%d",
                   gameCode.c_str(), roominfo.first.c_str(),
                   roominfo.second.size());
        }
      }
    }
    sleep(2);
  }

  printf("server stop\n");
  server.Stop();

  return 0;
}

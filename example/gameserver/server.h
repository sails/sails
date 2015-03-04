// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: server.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 15:43:09



#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <map>
#include <list>
#include <mutex>
#include "sails/net/epoll_server.h"
#include "sails/net/connector.h"
#include "sails/net/packets.h"
#include "game_world.h"
#include "game_packets.h"
#include "config.h"

namespace sails {

class Server : public sails::net::EpollServer<SceNetAdhocctlPacketBase> {
 public:
  Server();

  ~Server();

  // 在这里创建用户
  void CreateConnectorCB(std::shared_ptr<net::Connector> connector);

  // 创建用户
  uint32_t CreatePlayer(std::string ip, int port, int fd, uint32_t connectUid);

  // 数据解析
  SceNetAdhocctlPacketBase* Parse(
      std::shared_ptr<sails::net::Connector> connector);

  // NetThread关闭前，创建一个disconnector的数据包，让handle线程删除数据
  void ClosedConnectCB(std::shared_ptr<net::Connector> connector);

  

  

  
  // 获取游戏
  GameWorld* GetGameWorld(const std::string& gameCode);

  // 创建游戏
  GameWorld* CreateGameWorld(const std::string& gameCode);

  // 非法数据处理(直接移除用户,关闭连接),
  // 关于player的数据操作放到handle线程里
  // 防止多线程操作player的问题.创建一个disconnector的数据包
  void InvalidMsgHandle(std::shared_ptr<sails::net::Connector> connector);

  // 关闭连接，发起删除用户流程，一个对外接口，可以在其它线程中调用
  void KillOffPlayer(uint32_t playerId);
  
  Player* GetPlayer(uint32_t playerId);

  // 操作player属性值
  int GetPlayerState(uint32_t playerId);
  void SetPlayerState(int32_t playerId, int state);

  std::list<std::string> GetPlayerSession();

  std::list<std::string> GetGameWorldList();

  // 得到游戏组里用户
  std::map<std::string, std::list<std::string>> GetPlayerNameMap(
      const std::string& gameCode);

 private:
  std::mutex* GetPlayerMutex(uint32_t playerId);

 private:
  std::map<std::string, GameWorld*> gameWorldMap;
  std::mutex gameworldMutex;
  base::ConstantPtrList<Player> playerList;

  ///  //////////////////// handle //////////////////////////////////////////////
 public:
  void handle(
      const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);

  // 登录,对用户,游戏进行校验
  void login_user_data(
      const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);

  // 把用户加入游戏/组里
  void connect_user
  (const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);
  // 从组里把用户移除
  DisconnectState disconnect_user(
      const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);

  // 获取游戏的组列表
  void send_scan_results(
      const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);

  // 向用户发送聊天数据
  // (如果用户没有在房间里,则向所有用户发送,否则向房间内的用户发送)
  void spread_message(
      const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);

  // 向用户发送游戏数据
  void transfer_message(
      const sails::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);


  // 用户session 校验,如果不成功,则向handle线程发送退出命令
  void player_session_check(uint32_t playerId, std::string session);


  // mac地址转化
  static std::string getMacStr(const SceNetEtherAddr& macAddr);
  static SceNetEtherAddr getMacStruct(std::string macstr);

  // ip 转化
  static std::string getIpstr(uint32_t ip);
  static uint32_t getIp(std::string ip);

  // group name转化
  static SceNetAdhocctlGroupName getRoomName(const std::string& name);

 private:
  // 删除用户, 真正的删除逻辑
  void logout_user(uint32_t playerId);

  std::string game_product_override(SceNetAdhocctlProductCode * product);
};


extern Config config;

// session 校验与更新相关

size_t read_callback(void *buffer, size_t size, size_t nmemb, void *userp);

bool post_message(
    const char* url, const char* data, std::string &result);  // NOLINT'

bool check_session(std::string session);

bool update_session_timeout(const std::string& session);


}  // namespace sails


#endif  // SERVER_H

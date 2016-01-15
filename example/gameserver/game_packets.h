// Copyright (C) 2016 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: game_packets.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2016-01-15 15:47:04




#ifndef EXAMPLE_GAMESERVER_GAME_PACKETS_H_
#define EXAMPLE_GAMESERVER_GAME_PACKETS_H_

#include <stdint.h>
#include <string.h>

namespace sails {


#define OPCODE_PING 0
// #define OPCODE_LOGIN 1
// not allow other ppsspp login
#define OPCODE_LOGIN 111
#define OPCODE_CONNECT 2
#define OPCODE_DISCONNECT 3
#define OPCODE_SCAN 4
#define OPCODE_SCAN_COMPLETE 5
#define OPCODE_CONNECT_BSSID 6
#define OPCODE_CHAT 7

// PSP Product Code
#define PRODUCT_CODE_LENGTH 9


#define OPCODE_GAME_DATA 100
#define OPCODE_LOGOUT 101

#define PACKET_MAX 111

// Ethernet Address (MAC)
#define ETHER_ADDR_LEN 6


#pragma pack(push, 1)

typedef struct SceNetEtherAddr {
  uint8_t data[ETHER_ADDR_LEN];
  SceNetEtherAddr() {
    memset(data, '\0', sizeof(data));
  }
} SceNetEtherAddr;

// Adhoc Virtual Network Name (1234ABCD)
#define ADHOCCTL_GROUPNAME_LEN 8
typedef struct SceNetAdhocctlGroupName {
  uint8_t data[ADHOCCTL_GROUPNAME_LEN];
  SceNetAdhocctlGroupName() {
    memset(data, '\0', sizeof(data));
  }
} SceNetAdhocctlGroupName;

// Player Nickname
#define ADHOCCTL_NICKNAME_LEN 128
typedef struct SceNetAdhocctlNickname {
  uint8_t data[ADHOCCTL_NICKNAME_LEN];
  SceNetAdhocctlNickname() {
    memset(data, '\0', sizeof(data));
  }
} SceNetAdhocctlNickname;


// User States
#define USER_STATE_ERROR -1
#define USER_STATE_WAITING 0
#define USER_STATE_LOGGED_IN 1
#define USER_STATE_CONNECTED_ROOM 2
#define USER_STATE_TIMED_OUT 3
#define USER_STATE_DIC_CONN 4









//////////////// for data transfer /////////////////

struct SceNetAdhocctlProductCode {
  // Game Product Code (ex. ULUS12345)
  char data[PRODUCT_CODE_LENGTH];
  SceNetAdhocctlProductCode() {
    memset(data, '\0', sizeof(data));
  }
};

// Basic Packet
struct SceNetAdhocctlPacketBase {
  uint8_t opcode;
  SceNetAdhocctlPacketBase() {
    opcode = 0;
  }
};

// C2S Login Packet
struct SceNetAdhocctlLoginPacketC2S {
  SceNetAdhocctlPacketBase base;
  SceNetEtherAddr mac;
  SceNetAdhocctlNickname name;
  SceNetAdhocctlProductCode game;
  char session[100];
  SceNetAdhocctlLoginPacketC2S() {
    memset(session, '\0', sizeof(session));
  }
};

// C2S Connect Packet
struct SceNetAdhocctlConnectPacketC2S {
  SceNetAdhocctlPacketBase base;
  SceNetAdhocctlGroupName group;
};

// C2S Chat Packet
struct SceNetAdhocctlChatPacketC2S {
  SceNetAdhocctlPacketBase base;
  char message[64];
  SceNetAdhocctlChatPacketC2S() {
    memset(message, '\0', sizeof(message));
  }
};

// S2C Connect Packet
struct SceNetAdhocctlConnectPacketS2C {
  SceNetAdhocctlPacketBase base;
  SceNetAdhocctlNickname name;
  SceNetEtherAddr mac;
  uint32_t ip;
  SceNetAdhocctlConnectPacketS2C() {
    ip = 0;
  }
};

// S2C Disconnect Packet
struct SceNetAdhocctlDisconnectPacketS2C {
  SceNetAdhocctlPacketBase base;
  uint32_t ip;
  SceNetEtherAddr mac;
  SceNetAdhocctlDisconnectPacketS2C() {
    ip = 0;
  }
};

// S2C Scan Packet
struct SceNetAdhocctlScanPacketS2C {
  SceNetAdhocctlPacketBase base;
  SceNetAdhocctlGroupName group;
  SceNetEtherAddr mac;
};

// S2C Connect BSSID Packet
struct SceNetAdhocctlConnectBSSIDPacketS2C {
  SceNetAdhocctlPacketBase base;
  SceNetEtherAddr mac;
};

// S2C Chat Packet
struct SceNetAdhocctlChatPacketS2C {
  SceNetAdhocctlChatPacketC2S base;
  SceNetAdhocctlNickname name;
};


// c2c Packet
struct SceNetAdhocctlGameDataPacketC2C {
  SceNetAdhocctlPacketBase base;
  uint8_t additional_opcode;
  SceNetEtherAddr smac;
  SceNetEtherAddr dmac;
  uint32_t ip;
  uint16_t sport;
  uint16_t dport;
  int len;
  char data[1];
  SceNetAdhocctlGameDataPacketC2C() {
    additional_opcode = 0;
    ip = 0;
    sport = 0;
    dport = 0;
    len = 0;
    data[0] = '\0';
  }
};

#pragma pack(pop)


}  // namespace sails


#endif  // EXAMPLE_GAMESERVER_GAME_PACKETS_H_

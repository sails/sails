// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: packets.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 14:28:14



#ifndef SAILS_NET_PACKETS_H_
#define SAILS_NET_PACKETS_H_

#include <stdint.h>

namespace sails {
namespace net {


enum PacketDefine {
  PACKET_MIN = 0,
  PACKET_HEARTBEAT = 1,             // heartbeat
  PACKET_EXCEPTION,                 // exception
  PACKET_PROTOBUF_CALL,             // protobuf
  PACKET_PROTOBUF_RET,              // protobuf

  PACKET_MAX              // use as illegal judgment gettype() > MSG_MAX
};

#define PACKET_MAX_LEN  1024

#pragma pack(push, 1)

struct PacketBase {
  uint8_t opcode;
  PacketBase() {
    opcode = 0;
  }
};

struct PacketCommon {
  PacketBase type;
  uint16_t len;  // packet len
  uint32_t sn;  // 包的序列号
  PacketCommon() {
    len = sizeof(PacketCommon);
    sn = 0;
  }
};


#pragma pack(pop)

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET_PACKETS_H_

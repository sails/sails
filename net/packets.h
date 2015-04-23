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

typedef struct {
  uint8_t opcode;
} __attribute__((packed)) PacketBase;

typedef struct {
  PacketBase type;
  unsigned int len;  // packet len
} __attribute__((packed)) PacketCommon;

typedef struct
{
  PacketCommon common;
  char service_name[50];
  char method_name[50];
  int method_index;
  char data[1];
} __attribute__((packed)) PacketRPC;




typedef struct {
  int len;
  char *data;
} ResponseContent;

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET_PACKETS_H_

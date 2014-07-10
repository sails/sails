#ifndef _PACKETS_H_
#define _PACKETS_H_

#include <stdint.h>

namespace sails {
namespace common {
namespace net {


enum PacketDefine
{
    PACKET_MIN = 0,
    PACKET_HEARTBEAT = 1,             // heartbeat
    PACKET_EXCEPTION,                 // exception
    PACKET_PROTOBUF_CALL,             // protobuf
    PACKET_PROTOBUF_RET,              // protobuf

    PACKET_MAX                        // use as illegal judgment gettype() > MSG_MAX
};

typedef struct
{
	uint8_t opcode;
} __attribute__((packed)) PacketBase;

typedef struct
{
    PacketBase type;
    unsigned int len; // packet len
} __attribute__((packed)) PacketCommon;

typedef struct
{
    PacketCommon common;
    char service_name[50];
    char method_name[50];
    int method_index;
    char data[1];
} __attribute__((packed)) PacketRPC;

} // namespace net
} // namespace common
} // namespace sails

#endif /* _PACKETS_H_ */

#ifndef GAME_PACKETS_H
#define GAME_PACKETS_H


namespace sails {


#define OPCODE_PING 0
//#define OPCODE_LOGIN 1
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


#define PACKET_MAX 111

// Ethernet Address (MAC)
#define ETHER_ADDR_LEN 6
typedef struct SceNetEtherAddr {
	uint8_t data[ETHER_ADDR_LEN];
} SceNetEtherAddr;

// Adhoc Virtual Network Name (1234ABCD)
#define ADHOCCTL_GROUPNAME_LEN 8
typedef struct SceNetAdhocctlGroupName {
	uint8_t data[ADHOCCTL_GROUPNAME_LEN];
} SceNetAdhocctlGroupName;

// Player Nickname
#define ADHOCCTL_NICKNAME_LEN 128
typedef struct SceNetAdhocctlNickname {
	uint8_t data[ADHOCCTL_NICKNAME_LEN];
} SceNetAdhocctlNickname;


// User States
#define USER_STATE_ERROR -1
#define USER_STATE_WAITING 0
#define USER_STATE_LOGGED_IN 1
#define USER_STATE_CONNECTED_ROOM 2
#define USER_STATE_TIMED_OUT 3
#define USER_STATE_DIC_CONN 4









//////////////// for data transfer /////////////////

typedef struct
{
	// Game Product Code (ex. ULUS12345)
	char data[PRODUCT_CODE_LENGTH];
} __attribute__((packed)) SceNetAdhocctlProductCode;

// Basic Packet
typedef struct
{
	uint8_t opcode;
} __attribute__((packed)) SceNetAdhocctlPacketBase;

// C2S Login Packet
typedef struct
{
	SceNetAdhocctlPacketBase base;
	SceNetEtherAddr mac;
	SceNetAdhocctlNickname name;
    SceNetAdhocctlProductCode game;
    char session[100];
} __attribute__((packed)) SceNetAdhocctlLoginPacketC2S;

// C2S Connect Packet
typedef struct
{
	SceNetAdhocctlPacketBase base;
	SceNetAdhocctlGroupName group;
} __attribute__((packed)) SceNetAdhocctlConnectPacketC2S;

// C2S Chat Packet
typedef struct
{
	SceNetAdhocctlPacketBase base;
	char message[64];
} __attribute__((packed)) SceNetAdhocctlChatPacketC2S;

// S2C Connect Packet
typedef struct
{
	SceNetAdhocctlPacketBase base;
	SceNetAdhocctlNickname name;
	SceNetEtherAddr mac;
	uint32_t ip;
} __attribute__((packed)) SceNetAdhocctlConnectPacketS2C;

// S2C Disconnect Packet
typedef struct
{
	SceNetAdhocctlPacketBase base;
	uint32_t ip;
        SceNetEtherAddr mac;
} __attribute__((packed)) SceNetAdhocctlDisconnectPacketS2C;

// S2C Scan Packet
typedef struct
{
	SceNetAdhocctlPacketBase base;
	SceNetAdhocctlGroupName group;
	SceNetEtherAddr mac;
} __attribute__((packed)) SceNetAdhocctlScanPacketS2C;

// S2C Connect BSSID Packet
typedef struct
{
	SceNetAdhocctlPacketBase base;
	SceNetEtherAddr mac;
} __attribute__((packed)) SceNetAdhocctlConnectBSSIDPacketS2C;

// S2C Chat Packet
typedef struct
{
	SceNetAdhocctlChatPacketC2S base;
	SceNetAdhocctlNickname name;
} __attribute__((packed)) SceNetAdhocctlChatPacketS2C;


// c2c Packet
typedef struct {
    SceNetAdhocctlPacketBase base;
    uint8_t additional_opcode;
    SceNetEtherAddr smac;
    SceNetEtherAddr dmac;
    uint32_t ip;
    uint16_t sport;
    uint16_t dport;
    int len;
    char data[1];
} __attribute__((packed)) SceNetAdhocctlGameDataPacketC2C;




} // namespace sails


#endif /* GAME_PACKETS_H */

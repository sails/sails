#include "netbios.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <string>
#include <list>


namespace sails {
namespace net {

#define NETBIOS_HDR_LEN 12
#define NETBIOS_ANSWER_LEN 45
struct NetbiosAnswerHeader {
  u_int8_t netbios_name[34];
  u_int16_t netbios_type;
  u_int16_t netbios_class;
  u_int32_t netbios_ttl;
  u_int16_t netbios_length;
  u_int8_t netbios_number;
}  __attribute__((packed));



struct netbios_name_ans {
  char ip[20];
  char name[15];
  char group[15];
  uint8_t unitId[6];
};

// Netbios name types
#define NETBIOS_WORKSTATION   0x00
#define NETBIOS_MESSENGER     0x03
#define NETBIOS_FILESERVER    0x20
#define NETBIOS_DOMAINMASTER  0x1b
// Netbios flags
#define NETBIOS_NAME_FLAG_GROUP (1 << 15)


typedef enum { IP_ADDR, SUBNET_MASK, DEFAULT_GATEWAY, HW_ADDR } ADDR_T;

struct NetbiosHeader {
  u_int16_t netbios_id;
  u_int16_t netbios_flags;
  u_int16_t netbios_questions;
  u_int16_t netbios_answer;
  u_int16_t netbios_authority;
  u_int16_t netbios_additional;
}  __attribute__((packed));

/*netbios type*/
#define NETBIOS_NBSTAT 0x0021
/*netbios class*/
#define NETBIOS_CLASS 0x0001


NetbiosQuery::NetbiosQuery() {
  uint32_t seed = time(NULL);
  transationID = rand_r(&seed);
}


int NetbiosQuery::query(const char*ip, int timeout, NetbiosQueryAns* ret) {
  struct timeval tv;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = timeout % 1000 * 1000;
  int sockfd = Open();
  if (sockfd > 0) {
    // 发送查询
    send_query(ip, sockfd);

    fd_set readfd;
    FD_ZERO(&readfd);
    FD_SET(sockfd, &readfd);
    if (select(sockfd + 1, &readfd, NULL, NULL, &tv) <= 0) {
      printf("select error\n");
      return -1;
    }

    if (!FD_ISSET(sockfd, &readfd)) {
      return -1;
    }
    char buffer[500] = {'\0'};
    int n = 0;
    if ((n = recvfrom(
            sockfd, buffer, sizeof(buffer), 0, NULL, NULL)) < 0) {
      printf("recv 0 byte\n");
      return -1;
    }

    snprintf(ret->ip, sizeof(ret->ip), "%s", ip);
    return handle_ans(buffer, n, ret);
  }
  return -1;
}

std::list<NetbiosQueryAns> NetbiosQuery::query_subnet(const char*subnet,
                                                      int timeout) {
  std::list<NetbiosQueryAns> retlist;
  struct timeval tv;
  tv.tv_sec = timeout / 1000;
  tv.tv_usec = timeout % 1000 * 1000;
  int sockfd = Open();
  if (sockfd > 0) {
    // 发送查询
    for (int i = 1; i < 255; i++) {
      char ip[20] = {'\0'};
      char format_str[20] = {'\0'};
      int subnet_len = strlen(subnet);
      for (int i = 0; i < subnet_len; i++) {
        if (subnet[i] != '*') {
          format_str[i] = subnet[i];
        } else {
          format_str[i] = '%';
          format_str[i+1] = 'd';
          break;
        }
      }
      snprintf(ip, sizeof(ip), format_str, i);
      send_query(ip, sockfd);
    }

    fd_set readfd;
    FD_ZERO(&readfd);
    FD_SET(sockfd, &readfd);

    while (true) {
      if ((select(sockfd + 1, &readfd, NULL, NULL, &tv)) <= 0) {
        break;
      }
      if (!FD_ISSET(sockfd, &readfd)) {
        break;
      }
      char buffer[500] = {'\0'};
      int n = 0;
      struct sockaddr_in addr;
      bzero(&addr, sizeof(addr));
      socklen_t addr_len = sizeof(struct sockaddr_in);
      if ((n = recvfrom(
              sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr,
              &addr_len)) < 0) {
        printf("recv 0 byte\n");
        break;
      }
      NetbiosQueryAns ret;
      if (handle_ans(buffer, n, &ret) == 0) {
        char* ip = inet_ntoa(addr.sin_addr);
        snprintf(ret.ip, sizeof(ret.ip), "%s", ip);
        retlist.push_back(ret);
      }
    }
  }
  return retlist;
}


int NetbiosQuery::Open() {
  int n = 1;
  int sockfd = 0;
  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sockfd > 0) {
    // enable broadcast
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &n, sizeof(n)) < 0) {
      close(sockfd);
      return 0;
    }
  }
  return sockfd;
}


int NetbiosQuery::send_query(const char* ip, int fd) {
  struct sockaddr_in sin;
  char buffer[IP_MAXPACKET];
  ssize_t sizebuffer = 0;

  // clear
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(137);  // netbios


  // clear
  if (inet_aton(ip, &sin.sin_addr) == 0) {
    printf("inet_aton\n");
    return -1;
  }
  memset(buffer, 0, sizeof(buffer));
  struct NetbiosHeader *netbios = (struct NetbiosHeader *)buffer;
  netbios->netbios_id = htons(transationID);
  netbios->netbios_flags = 0;
  netbios->netbios_questions = htons(1);
  netbios->netbios_answer =
      netbios->netbios_authority =
      netbios->netbios_additional = htons(0);
  struct NetbiosAnswerHeader* netbios_answer =
      (struct NetbiosAnswerHeader *)(buffer + NETBIOS_HDR_LEN);
  netbios_answer->netbios_name[0] = 0x20;
  netbios_answer->netbios_name[1] = 0x43;
  netbios_answer->netbios_name[2] = 0x4b;
  for (int i = 3 ; i < 33 ; i++) {
    netbios_answer->netbios_name[i] = 0x41;
  }

  netbios_answer->netbios_type = htons(NETBIOS_NBSTAT);
  netbios_answer->netbios_class = htons(NETBIOS_CLASS);

  sizebuffer = sendto(fd, buffer, 50, 0,
                      (struct sockaddr *)&sin, (socklen_t)sizeof(sin));
  if (sizebuffer != 50) {
    printf("send sizebuffer error\n");
    return -1;
  }
  return 0;
}


int NetbiosQuery::handle_ans(const char* buffer,
                             int len, NetbiosQueryAns* ret) {
  if (len <= NETBIOS_HDR_LEN + NETBIOS_ANSWER_LEN) {
    return -1;
  }
  struct NetbiosHeader* netbios = (struct NetbiosHeader *)buffer;
  struct NetbiosAnswerHeader *netbios_answer = (struct NetbiosAnswerHeader *)(
      buffer + NETBIOS_HDR_LEN);
  // receive
  if (netbios->netbios_id == htons(transationID)) {
    const char* netbios_data = buffer + NETBIOS_HDR_LEN + NETBIOS_ANSWER_LEN;
    int name_count = netbios_answer->netbios_number;
    if (len < NETBIOS_HDR_LEN + NETBIOS_ANSWER_LEN + 18*name_count) {
      return -1;
    }
    const char* group = NULL;
    const char * name = NULL;
    const char* names = netbios_data;

    // netbios_name:char name[15], uint8_t type, uint16_t flag;
    // first search for a group in the name list
    for (uint8_t name_idx = 0; name_idx < name_count; name_idx++) {
      const char *current_name = names + name_idx * 18;
      uint16_t current_flags = (current_name[16] << 8) | current_name[17];
      if (current_flags & NETBIOS_NAME_FLAG_GROUP) {
        group = current_name;
        break;
      }
    }
    // then search for file servers
    for (uint8_t name_idx = 0; name_idx < name_count; name_idx++) {
      const char *current_name = names + name_idx * 18;
      char current_type = current_name[15];
      uint16_t current_flags = (current_name[16] << 8) | current_name[17];
      if (current_flags & NETBIOS_NAME_FLAG_GROUP)
        continue;
      if (current_type == NETBIOS_FILESERVER) {
        name = current_name;
        break;
      }
    }
    if (name) {
      snprintf(ret->name, sizeof(ret->name), "%s", name);
      snprintf(ret->group, sizeof(ret->group), "%s", group);
    }
    const uint8_t *unitId = reinterpret_cast<const uint8_t*>(
        netbios_data + name_count * 18);
    for (int i = 0; i < 6; i++) {
      ret->unitId[i] = unitId[i];
    }
  }
  return 0;
}

}  // namespace net
}  // namespace sails






int main() {
  sails::net::NetbiosQuery query;

  printf("query sigle ip:\n");
  sails::net::NetbiosQueryAns ret;
  if (query.query("192.168.198.1", 30, &ret) == 0) {
    printf("ip:%s, name:%s, group:%s, "
           "mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
           ret.ip, ret.name, ret.group, ret.unitId[0], ret.unitId[1],
           ret.unitId[2], ret.unitId[3], ret.unitId[4], ret.unitId[5]);
  }

  printf("query sub net\n");
  std::list<sails::net::NetbiosQueryAns> retlist =
      query.query_subnet("192.168.198.*", 100);
  for (auto &item : retlist) {
    printf("ip:%s, name:%s, group:%s, "
           "mac:%02x:%02x:%02x:%02x:%02x:%02x\n",
           item.ip, item.name, item.group, item.unitId[0], item.unitId[1],
           item.unitId[2], item.unitId[3], item.unitId[4], item.unitId[5]);
  }
  return 0;
}







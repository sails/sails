// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: netbios.h
// Description: netbios name query
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-09-29 13:43:44


#ifndef NET_NETBIOS_H_
#define NET_NETBIOS_H_

#include <stdint.h>
#include <list>


namespace sails {
namespace net {

struct NetbiosQueryAns {
  char ip[20];
  char name[15];
  char group[15];
  uint8_t unitId[6];
};

class NetbiosQuery {
 public:
  NetbiosQuery();

  // 查询一个地址的netbios name
  int query(const char*ip, int timeout, NetbiosQueryAns* ret);

  // 查询同一个子网下所有的netbios name
  // subnet: 192.168.1.*，只支持在最后一个加通配符
  std::list<NetbiosQueryAns> query_subnet(const char*subnet, int timeout);

 private:
  int Open();

  int send_query(const char* ip, int fd);

  int handle_ans(const char* buffer, int len, NetbiosQueryAns* ret);

 private:
  uint16_t transationID;
};


}  // namespace net
}  // namespace sails





#endif  // NET_NETBIOS_H_



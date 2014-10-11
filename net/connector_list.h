// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: connector_list.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:50:47



#ifndef SAILS_NET_CONNECTOR_LIST_H_
#define SAILS_NET_CONNECTOR_LIST_H_

#include <list>
#include <memory>
#include <mutex>
#include "sails/net/connector.h"

namespace sails {
namespace net {

class ConnectorList {
 public:
  ConnectorList();

  // 析够函数
  ~ConnectorList();

  // 初始化大小
  void init(uint32_t size);

  /**
   * 获取惟一ID
   *
   * @return unsigned int
   */
  uint32_t getUniqId();

  // 添加连接
  void add(std::shared_ptr<Connector> connector);

  // 获取某一个连接
  std::shared_ptr<Connector> get(uint32_t uid);

  // 删除连接
  void del(uint32_t uid);

  // 大小
  size_t size();

 protected:
  // 内部删除, 不加锁
  void _del(uint32_t uid);

 protected:
  // 总计连接数
  uint32_t total;

  // 空闲链表
  std::list<uint32_t> free;

  // 空闲链元素个数
  size_t free_size;

  // 链接列表
  std::shared_ptr<Connector>* vConn;
  // 链接ID的魔数
  uint32_t magic_num;
  std::mutex list_mutex;
};


}  // namespace net
}  // namespace sails


#endif  // SAILS_NET_CONNECTOR_LIST_H_

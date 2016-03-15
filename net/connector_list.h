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
#include <map>
#include "sails/net/connector.h"

namespace sails {
namespace net {

/**
 * 带有时间链表的map
 */
class ConnectorList {
 public:
  /**
   * 构造函数
   */
  ConnectorList();

  /**
   * 析够函数
   */
  ~ConnectorList() { if(_vConn) { delete[] _vConn; } }

  /**
   * 初始化大小
   * @param size
   */
  void init(uint32_t size, uint32_t iIndex = 0);

  /**
   * 获取惟一ID
   *
   * @return unsigned int
   */
  uint32_t getUniqId();

  /**
   * 添加连接
   * @param cPtr
   * @param iTimeOutStamp
   */
  void add(std::shared_ptr<Connector> con, time_t iTimeOutStamp);

  /**
   * 刷新时间链
   * @param uid
   * @param iTimeOutStamp, 超时时间点
   */
  void refresh(uint32_t uid, time_t iTimeOutStamp);

  /**
   * 检查超时数据
   */
  void checkTimeout(time_t iCurTime);

  /**
   * 获取所有链接状态
   * @param lfd listen fd
   * @return ConnStatus
   */
  std::vector<ConnStatus> getConnStatus(int lfd);

  /**
   * 获取某一个连接
   * @param p
   * @return T
   */
  std::shared_ptr<Connector> get(uint32_t uid);

  /**
   * 删除连接
   * @param uid
   */
  void del(uint32_t uid);

  /**
   * 大小
   * @return size_t
   */
  size_t size();

 protected:
  typedef std::pair<std::shared_ptr<Connector>,
                    std::multimap<time_t, uint32_t>::iterator> list_data;

  /**
   * 内部删除, 不加锁
   * @param uid
   */
  void _del(uint32_t uid);

 protected:

  /**
   * 总计连接数
   */
  uint32_t _total;

  /**
   * 空闲链表
   */
  std::list<uint32_t> _free;

  /**
   * 空闲链元素个数
   */
  size_t _free_size;

  /**
   * 链接
   */
  list_data *_vConn;

  /**
   * 超时链表
   */
  std::multimap<time_t, uint32_t> _tl;

  /**
   * 上次检查超时时间
   */
  time_t _lastTimeoutTime;

  /**
   * 链接ID的魔数
   */
  uint32_t _iConnectionMagic;

  std::mutex list_mutex;
};


}  // namespace net
}  // namespace sails


#endif  // SAILS_NET_CONNECTOR_LIST_H_

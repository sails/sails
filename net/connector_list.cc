// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: connector_list.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:54:07



#include "sails/net/connector_list.h"
#include <time.h>
#include <assert.h>
#include <vector>

namespace sails {
namespace net {


const int ConnectorList::blockNum;

ConnectorList::ConnectorList()
    : _free_size(0)
    , blockUsedNum(0)
    , _lastTimeoutTime(0)
    , _iConnectionMagic(0) {
  for (int i = 0; i < blockNum; i++) {
    _vConn[i] = NULL;
  }
}

ConnectorList::~ConnectorList() {
  for (int i = 0; i < blockUsedNum; i++) {
    if (_vConn[i] != NULL) {
      delete[] _vConn[i];
      _vConn[i] = NULL;
    }
  }
}

void ConnectorList::init() {
  _lastTimeoutTime = 0;
  _free_size  = 0;
  // 初始化链接链表
  for (int i = 0; i < blockNum; i++) {
    if (_vConn[i] != NULL) {
      delete[] _vConn[i];
    }
  }

  // 分配blockCapacity个空间( 第一个空间其实无效)
  blockUsedNum = 1;
  _vConn[0] = new list_data[blockCapacity];

  _lastTimeoutTime = time(NULL);

  static int iIndex = 0;
  iIndex++;
  // 8位(256s不重复)+4(index,同一秒可以同时创建16个list)+20(100w)
  _iConnectionMagic = ((((uint32_t)_lastTimeoutTime) << 24)
                       & (0xFFFFFFFF << 24))
                      + ((iIndex << 20) & (0xFFFFFFFF << 20));

  // free从1开始分配, 这个值为uid, 0保留为管道用,
  // epollwait根据0判断是否是管道消息
  _vConn[0][0].first = NULL;
  for (int i = 1; i < blockCapacity; i++) {
    _vConn[0][i].first = NULL;
    _free.push_back(i);
    ++_free_size;
  }
}

uint32_t ConnectorList::getUniqId() {
  std::unique_lock<std::mutex> locker(list_mutex);
  ensure_free_not_empty();
  uint32_t uid = _free.front();
  printf("connector uid:%d\n", uid);
  _free.pop_front();
  --_free_size;
  return _iConnectionMagic | uid;
}

void ConnectorList::ensure_free_not_empty() {
  if (_free_size == 0) {
    // 要增加block
    printf("new block:%d\n", blockUsedNum);
    blockUsedNum++;
    if (_vConn[blockUsedNum-1] == NULL) {
      _vConn[blockUsedNum-1] = new list_data[blockCapacity];
      for (int i = 0; i < blockCapacity; i++) {
        _vConn[blockUsedNum-1][i].first = NULL;
        _free.push_back((blockUsedNum-1)*blockCapacity+i);
        ++_free_size;
      }
    }
  }
}

std::shared_ptr<Connector> ConnectorList::get(uint32_t uid) {
  uint32_t magi = uid & (0xFFFFFFFF << 20);
  uid           = uid & (0x7FFFFFFF >> 11);

  if (magi != _iConnectionMagic) return NULL;

  return _vConn[uid/blockCapacity][uid%blockCapacity].first;
}

void ConnectorList::add(std::shared_ptr<Connector> conn, time_t iTimeOutStamp) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t muid = conn->getId();
  uint32_t magi = muid & (0xFFFFFFFF << 20);
  uint32_t uid  = muid & (0x7FFFFFFF >> 11);

  assert(magi == _iConnectionMagic && uid > 0);

  // 插入对应的block(每个block和blocksize个connector)
  int block = uid / blockCapacity;

  _vConn[block][uid%blockCapacity] = std::make_pair(
      conn, _tl.insert(std::make_pair(iTimeOutStamp, uid)));
}

void ConnectorList::refresh(uint32_t uid, time_t iTimeOutStamp) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t magi = uid & (0xFFFFFFFF << 20);
  uid           = uid & (0x7FFFFFFF >> 11);

  auto connector = _vConn[uid/blockCapacity][uid%blockCapacity].first;

  assert(magi == _iConnectionMagic && uid > 0
         && _vConn[uid/blockCapacity][uid%blockCapacity].first);

  // 防止太频繁无意义的更新最后更新时间
  if (iTimeOutStamp - connector->LastRefreshTime() < 1) {
    return;
  }
  connector->setLastRefreshTime(iTimeOutStamp);

  // 删除超时链表
  _tl.erase(_vConn[uid/blockCapacity][uid%blockCapacity].second);

  _vConn[uid/blockCapacity][uid%blockCapacity].second =
      _tl.insert(std::make_pair(iTimeOutStamp, uid));
}

void ConnectorList::checkTimeout(time_t iCurTime) {
  // 至少1s才能检查一次
  if (iCurTime - _lastTimeoutTime < 1) {
    return;
  }

  _lastTimeoutTime = iCurTime;

  std::unique_lock<std::mutex> locker(list_mutex);

  std::multimap<time_t, uint32_t>::iterator it = _tl.begin();

  while (it != _tl.end()) {
    // 已经检查到当前时间点了, 后续不用在检查了
    if (it->first > iCurTime) {
      break;
    }

    uint32_t uid = it->second;

    ++it;

    auto connector = _vConn[uid/blockCapacity][uid%blockCapacity].first;
    // udp的监听端口, 不做处理
    if (connector->get_listen_fd() == -1) {
      continue;
    }

    // 超时关闭
    connector->set_timeout();

    // 从链表中删除
    // _del(uid);，这里不删除，而是由超时回调函数决定是否删除
  }
}

std::vector<ConnStatus> ConnectorList::getConnStatus(int lfd) {
  std::vector<ConnStatus> v;
  std::unique_lock<std::mutex> locker(list_mutex);

  for (int i = 0; i < blockNum; i++) {
    if (_vConn[i] != NULL) {
      for (int j = 0; j < blockCapacity; j++) {
        auto connector = _vConn[i][j].first;
        // 是当前监听端口的连接
        if (connector != NULL && connector->get_listen_fd() == lfd) {
          ConnStatus cs;

          cs.iLastRefreshTime    = connector->LastRefreshTime();
          cs.ip                  = connector->getIp();
          cs.port                = connector->getPort();
          cs.timeout             = connector->getTimeout();
          cs.uid                 = connector->getId();

          v.push_back(cs);
        }
      }
    }
  }
  return v;
}

void ConnectorList::del(uint32_t uid) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t magi = uid & (0xFFFFFFFF << 20);
  uid           = uid & (0x7FFFFFFF >> 11);

  assert(magi == _iConnectionMagic && uid > 0);

  _del(uid);
}

void ConnectorList::_del(uint32_t uid) {
  auto& data_pair = _vConn[uid/blockCapacity][uid%blockCapacity];
  assert(uid > 0 && data_pair.first != NULL);

  _tl.erase(data_pair.second);

  data_pair.first = NULL;

  _free.push_back(uid);

  ++_free_size;
}

size_t ConnectorList::size() {
  std::unique_lock<std::mutex> locker(list_mutex);
  return blockCapacity*blockUsedNum - _free_size - 1;
}



}  // namespace net
}  // namespace sails

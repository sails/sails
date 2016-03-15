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

namespace sails {
namespace net {


ConnectorList::ConnectorList()
    :_total(0)
    ,_free_size(0)
    ,_vConn(NULL)
    ,_lastTimeoutTime(0)
    ,_iConnectionMagic(0) {
}

void ConnectorList::init(uint32_t size, uint32_t iIndex) {
  _lastTimeoutTime = 0;
  _total = size;
  _free_size  = 0;
  //初始化链接链表
  if(_vConn) delete[] _vConn;
  
  //分配total+1个空间(多分配一个空间, 第一个空间其实无效)
  _vConn = new list_data[_total+1];

  _lastTimeoutTime = time(NULL);

  // 8位(256s不重复)+4(index,同一秒可以同时创建16个list)+20(100w)
  _iConnectionMagic = ((((uint32_t)_lastTimeoutTime) << 24) & (0xFFFFFFFF << 24))
                      + ((iIndex << 20) & (0xFFFFFFFF << 20));
  
  //free从1开始分配, 这个值为uid, 0保留为管道用, epollwait根据0判断是否是管道消息
  for (uint32_t i = 1; i <= _total; i++) {
    _vConn[i].first = NULL;
    _free.push_back(i);
    ++_free_size;
  }
}

uint32_t ConnectorList::getUniqId() {
  std::unique_lock<std::mutex> locker(list_mutex);
  uint32_t uid = _free.front();
  assert(uid > 0 && uid <= _total);
  _free.pop_front();
  --_free_size;
  return _iConnectionMagic | uid;
}

std::shared_ptr<Connector> ConnectorList::get(uint32_t uid) {
  uint32_t magi = uid & (0xFFFFFFFF << 20);
  uid           = uid & (0x7FFFFFFF >> 11);

  if (magi != _iConnectionMagic) return NULL;

  return _vConn[uid].first;
}

void ConnectorList::add(std::shared_ptr<Connector> conn, time_t iTimeOutStamp) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t muid = conn->getId();
  uint32_t magi = muid & (0xFFFFFFFF << 20);
  uint32_t uid  = muid & (0x7FFFFFFF >> 11);

  assert(magi == _iConnectionMagic && uid > 0
         && uid <= _total && !_vConn[uid].first);

  _vConn[uid] = std::make_pair(conn,
                               _tl.insert(std::make_pair(iTimeOutStamp,
                                                         uid)));
}

void ConnectorList::refresh(uint32_t uid, time_t iTimeOutStamp) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t magi = uid & (0xFFFFFFFF << 20);
  uid           = uid & (0x7FFFFFFF >> 11);

  assert(magi == _iConnectionMagic && uid > 0 && uid <= _total && _vConn[uid].first);

  if(iTimeOutStamp - _vConn[uid].first->LastRefreshTime() < 1) {
    return;
  }
  _vConn[uid].first->setLastRefreshTime(iTimeOutStamp);

  //删除超时链表
  _tl.erase(_vConn[uid].second);
  
  _vConn[uid].second = _tl.insert(std::make_pair(iTimeOutStamp, uid));
}

void ConnectorList::checkTimeout(time_t iCurTime)
{
  //至少1s才能检查一次
  if(iCurTime - _lastTimeoutTime < 1) {
    return;
  }

  _lastTimeoutTime = iCurTime;

  std::unique_lock<std::mutex> locker(list_mutex);
  
  std::multimap<time_t, uint32_t>::iterator it = _tl.begin();

  while(it != _tl.end()) {
    //已经检查到当前时间点了, 后续不用在检查了
    if(it->first > iCurTime) {
      break;
    }

    uint32_t uid = it->second;

    ++it;

    //udp的监听端口, 不做处理
    if(_vConn[uid].first->get_listen_fd() == -1) {
      continue;
    }

    //超时关闭
    _vConn[uid].first->set_timeout();
    
    //从链表中删除
    //    _del(uid);，这里不删除，而是由超时回调函数决定是否删除
  }
}

std::vector<ConnStatus> ConnectorList::getConnStatus(int lfd) {
  std::vector<ConnStatus> v;
  
  std::unique_lock<std::mutex> locker(list_mutex);
  
  for(size_t i = 1; i <= _total; i++) {
    //是当前监听端口的连接
    if(_vConn[i].first != NULL && _vConn[i].first->get_listen_fd() == lfd) {
      ConnStatus cs;

      cs.iLastRefreshTime    = _vConn[i].first->LastRefreshTime();
      cs.ip                  = _vConn[i].first->getIp();
      cs.port                = _vConn[i].first->getPort();
      cs.timeout             = _vConn[i].first->getTimeout();
      cs.uid                 = _vConn[i].first->getId();
      
      v.push_back(cs);
    }
  }

  return v;
}

void ConnectorList::del(uint32_t uid) {
  std::unique_lock<std::mutex> locker(list_mutex);

  uint32_t magi = uid & (0xFFFFFFFF << 20);
  uid           = uid & (0x7FFFFFFF >> 11);

  assert(magi == _iConnectionMagic
         && uid > 0
         && uid <= _total
         && _vConn[uid].first);

  _del(uid);
}

void ConnectorList::_del(uint32_t uid) {
  assert(uid > 0 && uid <= _total && _vConn[uid].first);

  _tl.erase(_vConn[uid].second);

  _vConn[uid].first = NULL;
  
  _free.push_back(uid);

  ++_free_size;
}

size_t ConnectorList::size() {
  std::unique_lock<std::mutex> locker(list_mutex);
  
  return _total - _free_size;
}



}  // namespace net
}  // namespace sails

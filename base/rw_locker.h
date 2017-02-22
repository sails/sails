// Copyright (C) 2016 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: rw_locker.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2016-04-20 09:28:18

#ifndef BASE_RW_LOCKER_H_
#define BASE_RW_LOCKER_H_

#include <mutex>  // NOLINT


namespace sails {
namespace base {

class ReadWriteLock {
 public:
  ReadWriteLock() : read_cnt(0) {
  }

  // 读锁
  void readLock() {
    read_mtx.lock();
    if (++read_cnt == 1)
      write_mtx.lock();

    read_mtx.unlock();
  }
  void readUnlock() {
    read_mtx.lock();
    if (--read_cnt == 0)
      write_mtx.unlock();

    read_mtx.unlock();
  }

  // 写锁
  void writeLock() {
    write_mtx.lock();
  }
  void writeUnlock() {
    write_mtx.unlock();
  }

 private:
  std::mutex read_mtx;
  std::mutex write_mtx;
  int read_cnt;  // 已加读锁个数
};

class RWLocker {
 public:
  enum LockerType {
    READ = 1
    , WRITE = 2
  };

 public:
  RWLocker(ReadWriteLock& lock, const LockerType type) {
    this->type = type;
    this->lock = &lock;
    if (type == LockerType::READ) {
      lock.readLock();
    } else {
      lock.writeLock();
    }
  }
  ~RWLocker() {
    if (type == LockerType::READ) {
      lock->readUnlock();
    } else {
      lock->writeUnlock();
    }
  }

 private:
  LockerType type;
  ReadWriteLock* lock;
};


}  // namespace base
}  // namespace sails

#endif  // BASE_RW_LOCKER_H_

// Copyright (C) 2016 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: rwlock_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2016-04-20 10:13:44

#include "catch.hpp"
#include <stdio.h>
#include <string>
#include "../base/rw_locker.h"

TEST_CASE("ReadWriteLocker", "locker") {
  sails::base::ReadWriteLock lock;
  for (int i = 0; i < 1000; i++) {
    sails::base::RWLocker locker(lock, sails::base::RWLocker::LockerType::READ);
  }
  sails::base::RWLocker locker(lock, sails::base::RWLocker::LockerType::WRITE);
}














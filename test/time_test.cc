// Copyright (C) 2015 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: time_test.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2015-10-23 09:55:41

// 经过测试耗时：clock_gettime > time > gettimeofday > rdtsc

// 从下面的测试可以发现，gettimeofday一秒也可以调用上千万次，
// 而rdtsc性能则是它的4-5倍，达到0.1G，也就是说gettimeofday
// 才一百个时钟期，而rdtsc才二十个时钟周期

#include <time.h>
#include <sys/time.h>
#include "catch.hpp"
#include "../base/time_t.h"


// 大约20s
TEST_CASE("TimeTest1", "[time call 10000w]") {
  char nowstr[30] = {'\0'};
  printf("start test system call:time for 10000w\n");
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("start %s\n", nowstr);
  for (int i = 0; i < 100000000; i++) {
    time(NULL);
  }
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("end %s\n", nowstr);
}

// 大约4s
TEST_CASE("TimeTest2", "[gettimeofday call 10000w]") {
  char nowstr[30] = {'\0'};
  printf("start test system call:gettimeofday for 10000w\n");
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("start %s\n", nowstr);
  for (int i = 0; i < 100000000; i++) {
    struct timeval t;
    gettimeofday(&t, NULL);
  }
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("end %s\n", nowstr);
}

// 大约1.5s
TEST_CASE("TimeTest3", "[get_timeofday call 10000w]") {
  char nowstr[30] = {'\0'};
  printf("start test sails call:get_timeofday for 10000w\n");
  // 预先预热2s
  struct timeval t;
  sails::base::TimeT::get_timeofday(&t);
  sleep(2);
  sails::base::TimeT::get_timeofday(&t);
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("start %s\n", nowstr);
  for (int i = 0; i < 100000000; i++) {
    sails::base::TimeT::get_timeofday(&t);
  }
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("end %s\n", nowstr);
  printf("call timeof day :%d\n", sails::base::calltime);
}

#define rdtsc(low, high) \
     __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

// 大约800ms
TEST_CASE("TimeTest4", "[rdtsc 10000w]") {
  char nowstr[30] = {'\0'};
  printf("start test get time by rdtsc for 10000w\n");
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("start %s\n", nowstr);
  uint32_t low    = 0;
  uint32_t high   = 0;
  for (int i = 0; i < 100000000; i++) {
    rdtsc(low, high);
    // uint64_t current_tsc = ((uint64_t)high << 32) | low;
  }
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("end %s\n", nowstr);
}

// 大约30s
TEST_CASE("TimeTest5", "[clock_gettime 1000w]") {
  char nowstr[30] = {'\0'};
  printf("start test current_utc_time for 1000w\n");
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("start %s\n", nowstr);
  timespec ts;
  for (int i = 0; i < 10000000; i++) {
    sails::base::TimeT::current_utc_time(&ts);
  }
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("end %s\n", nowstr);
}

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

// 经过测试耗时：clock_gettime > time > gettimeofday > rdtsc > chrono

// 从下面的测试可以发现，gettimeofday一秒也可以调用上千万次，
// 而rdtsc性能则是它的4-5倍，达到0.1G，也就是说gettimeofday
// 才一百个时钟期，而rdtsc才二十个时钟周期

#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <chrono>
#include "catch.hpp"
#include "../base/time_t.h"
#include "../base/time_provider.h"


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

// 大约4s
TEST_CASE("TimeTest3", "[get_timeofday call 10000w]") {
  char nowstr[30] = {'\0'};
  // 预先预热2s
  struct timeval t;
  sails::base::TimeT::get_timeofday(&t);
  sleep(2);
  printf("start test sails call:get_timeofday for 10000w\n");
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
    uint64_t current_tsc = ((uint64_t)high << 32) | low;
  }
  sails::base::TimeT::time_with_millisecond(nowstr, 30);
  printf("end %s\n", nowstr);
}

// 大约2.6s
TEST_CASE("TimeTest6", "[clock_gettime 10000w]") {
  printf("start test std::chrono::high_resolution_clock::now for 10000w\n");
  std::chrono::high_resolution_clock::time_point start =
        std::chrono::high_resolution_clock::now();
  for (int i = 0; i < 100000000; i++) {
    std::chrono::high_resolution_clock::now();
  }
  std::chrono::high_resolution_clock::time_point end =
        std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
  printf("end %f seconds\n", time_span.count());
}


// 大约4.5s
TEST_CASE("TimeTest7", "[system_clock 10000w]") {
  printf("start test std::chrono::system_clock::now for 10000w\n");
  auto start = std::chrono::system_clock::now();
  for (int i = 0; i < 100000000; i++) {
    std::chrono::system_clock::now();
  }
  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> time_span =
      std::chrono::duration_cast<std::chrono::duration<double>>(end - start);
  printf("end %f seconds\n", time_span.count());
}


// 大约0.5s / 10000w
TEST_CASE("TimeTest8", "[time_provider 10000w]") {
  printf("start test time_provider TNOW for 10000w\n");
  uint64_t start = TNOWMS;
  printf("start time:%llu\n", TNOWMS);
  for (int i = 0; i < 100000000; i++) {
    TNOW;
  }
  printf("end %llu ms\n",TNOWMS - start);
}


// 大约6s / 10000w
TEST_CASE("TimeTest9", "[time_provider 100000w]") {
  printf("start test time_provider TNOWMS for 100000w\n");
  uint64_t start = TNOWMS;
  printf("start time:%llu\n", TNOWMS);
  for (int i = 0; i < 100000000; i++) {
    TNOWMS;
  }
  printf("end %llu ms\n",TNOWMS - start);
}

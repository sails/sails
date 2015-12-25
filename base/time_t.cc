// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: time_t.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 09:35:13



#include "sails/base/time_t.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#ifdef __linux__
#include <sys/timeb.h>
#endif
#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#include "TargetConditionals.h"
#endif
namespace sails {
namespace base {


size_t TimeT::time_str(char*s, size_t max) {
  size_t need_len = 20;
  if (max < need_len) {
    return 0;
  }
  memset(s, '\0', need_len);
  time_t temp;
  temp = time(NULL);
  struct tm t;
  localtime_r(&temp, &t);
  if (strftime(s, max, "%Y-%m-%d %H:%M:%S", &t)) {
    return strlen(s);
  } else {
    return 0;
  }
}

size_t TimeT::time_with_millisecond(char* s, size_t max) {
  size_t need_len = 24;
  if (max < need_len) {
    return 0;
  }

  memset(s, '\0', need_len);
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm t;
  localtime_r(&tv.tv_sec, &t);
  if (strftime(s, max, "%Y-%m-%d %H:%M:%S", &t)) {
    s[strlen(s)] = ' ';

    snprintf(s+strlen(s),
             4, "%d", static_cast<int>(tv.tv_usec/1000));  // NOTLINT'
    return strlen(s);
  } else {
    return 0;
  }
}



time_t TimeT::coverStrToTime(const char* timestr) {
  if (NULL == timestr) {
    return 0;
  }
  struct tm tm_;
  uint32_t year = 0;
  uint32_t month = 0;
  uint32_t day = 0;
  uint32_t hour = 0;
  uint32_t minute = 0;
  uint32_t second = 0;
  sscanf(timestr, "%4u-%2u-%2uT%2u:%2u:%2u",
         &year, &month, &day, &hour, &minute, &second);
  tm_.tm_year  = year-1900;
  tm_.tm_mon   = month-1;
  tm_.tm_mday  = day;
  tm_.tm_hour  = hour;
  tm_.tm_min   = minute;
  tm_.tm_sec   = second;
  tm_.tm_isdst = 0;

  time_t t_ = mktime(&tm_);
  return t_;
}

#ifndef __ANDROID__
#ifndef TARGET_OS_IPHONE
// android的内核不支持这个指令
#define rdtsc(low, high) \
  __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))
#endif
#endif

int calltime = 0;
void TimeT::get_timeofday(timeval* tv) {
#ifdef __ANDROID__
  gettimeofday(tv, NULL);
  return;
#endif
#ifdef TARGET_OS_IPHONE
  gettimeofday(tv, NULL);
  return;
#endif
  // 一个指令周期多少微秒(按照现在的3GHzcpu，算出来差不多应该是0.3ns)
  static float cpu_cycle = 0;
  static timeval last_tv;
  if (cpu_cycle == 0) {
    gettimeofday(tv, NULL);
    static timeval first_call = *tv;  // 记录第一次调用时间
    static uint64_t first_tsc = get_tsc();  // NOLINT
    if (tv->tv_sec - first_call.tv_sec >= 1) {  // 计算这段时间内的cpu指令数
      uint64_t current_tsc = get_tsc();
      // 这期间的微秒
      int sptime = (tv->tv_sec-first_call.tv_sec)*1000*1000+
                        (tv->tv_usec-first_call.tv_usec);
      cpu_cycle = (static_cast<float>(sptime) / (current_tsc-first_tsc));
    }
    last_tv = *tv;
    return;
  }
  static uint64_t last_tsc = 0;
  uint64_t current_tsc = get_tsc();
  int64_t t =  (int64_t)(current_tsc - last_tsc);
  time_t offset =  (time_t)(t * cpu_cycle);

  // 毫秒级，本来应该是1000，但是为了减少误差，用800
  if (t < -1000 || offset > 800) {
    last_tsc = current_tsc;
    gettimeofday(tv, NULL);
    calltime++;
    last_tv = *tv;
  } else {
    tv->tv_sec = last_tv.tv_sec;
    tv->tv_usec = last_tv.tv_usec;
  }
}

int64_t TimeT::getNowMs() {
  timeval tv;
  get_timeofday(&tv);
  return tv.tv_sec*1000+tv.tv_usec/1000;
}

uint64_t TimeT::get_tsc() {
  uint32_t low = 0;
  uint32_t high = 0;
#ifndef __ANDROID__
#ifndef TARGET_OS_IPHONE
  rdtsc(low, high);
#endif
#endif
  return ((uint64_t)high << 32) | low;
}

/*
// unix and not apple need link rt
void TimeT::current_utc_time(struct timespec *ts) {
  // OS X does not have clock_gettime, use clock_get_time
#ifdef __MACH__
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif
}
*/


}  // namespace base
}  // namespace sails

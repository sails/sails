// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: time_t.h
// Description:时间转换常用函数
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 09:34:28



#ifndef BASE_TIME_T_H_
#define BASE_TIME_T_H_

#include <stdint.h>
#include <time.h>
#include <sys/time.h>

namespace sails {
namespace base {

extern int calltime;
class TimeT {
 public:
  // 返回当前时间的字符串： 2014-04-11 10:10:10
  static size_t time_str(char*s, size_t max);

  // 返回当前时间的字符串： 2014-04-11 10:10:10 10
  static size_t  time_with_millisecond(char* s, size_t max);

  // 从2014-04-11 10:10:10到time_t结构进行转换
  static time_t coverStrToTime(const char* timestr);

  // 精确到毫秒，不过它比gettimeofday要快几倍，因为用到了cpu的rdtsc指令
  // 它返回的是cpu启动到现在的运行周期数，在2010年之前的多核cpu，可能会存在
  // cpu之前的加载时间不一致，导致每次读到不同的cpu上使结果不准确，不过新的
  // cpu已经可以保证在多个之间同步
  // 在预热之后(离首次调用1s之后)，性能测试大概能达到8000w/s
  static void get_timeofday(timeval* tv);

  // 得到当前毫秒
  static int64_t getNowMs();

  static uint64_t get_tsc();

  // 得到当前时间高精度纳秒
  static void current_utc_time(struct timespec *ts);
};




}  // namespace base
}  // namespace sails


#endif  // BASE_TIME_T_H_

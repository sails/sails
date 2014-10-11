// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: time_t.h
// Description:时间转换常用函数
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 09:34:28



#ifndef SAILS_BASE_TIME_T_H_
#define SAILS_BASE_TIME_T_H_

#include <time.h>

namespace sails {
namespace base {

class TimeT {
 public:
  // return 2014-04-11 10:10:10
  static size_t time_str(char*s, size_t max);

  // returns  the  number  of  bytes
  // (excluding  the  terminating  null byte) placed in the array s.
  // If the length of the result string (including the terminating null
  // byte) would exceed  max  bytes,  then strftime() returns 0
  // 2014-04-11 10:10:10 10
  static size_t  time_with_millisecond(char* s, size_t max);

  // 从2014-04-11 10:10:10到time_t结构进行转换
  static time_t coverStrToTime(const char* timestr);
};




}  // namespace base
}  // namespace sails


#endif  // SAILS_BASE_TIME_T_H_

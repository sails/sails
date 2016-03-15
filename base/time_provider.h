// Copyright (C) 2016 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: time_provider.h
// Description: 时间提供类，秒级、微妙级时间提供类. 它运行一个单独的线程，
//              大概800ms左右更新一次，所以在得到当前时间（秒）时，就是直
//              接读取保存的值，但是在读取ms时，由于自动更新的频率不够，所
//              以要更通过addTimeOffset来计算和上一次更新的差(所以ms的性
//              能也不会很好大概2000w/s)
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2016-03-14 16:28:31


#ifndef BASE_TIME_PROVIDER_H_
#define BASE_TIME_PROVIDER_H_

#include <string.h>
#include <sys/time.h>
#include <string>
#include <mutex>
#include <thread>
#include <condition_variable>

#define rdtsc(low, high) \
  __asm__ __volatile__("rdtsc" : "=a" (low), "=d" (high))

#define TNOW     sails::base::TimeProvider::getInstance()->getNow()
#define TNOWMS   sails::base::TimeProvider::getInstance()->getNowMs()

namespace sails {
namespace base {

/////////////////////////////////////////////////

class TimeProvider {
 public:
  static TimeProvider* getInstance();

  TimeProvider()
      : terminate(false)
      , use_tsc(true)
      , cpu_cycle(0)
      , buf_idx(0)
      , _thread(NULL) {
    memset(tv, 0, sizeof(tv));
    memset(tsc, 0, sizeof(tsc));

    struct timeval t;
    ::gettimeofday(&t, NULL);
    tv[0] = t;
    tv[1] = t;
  }

  // 析构，停止线程
  ~TimeProvider();

  // 获取时间.
  time_t getNow() {  return tv[buf_idx].tv_sec; }

  // 获取时间，每次调用，它都会去
  void getNow(timeval * tv);

  // 获取ms时间.
  int64_t getNowMs();

  // 获取cpu主频.
  float cpuMHz();

  // 运行
 protected:
  // 自动更新时间，800ms更新一次，所以读取时，只有秒是准确的，毫秒需要
  // 实时计算；如果增加更新频率到1ms以后，也会有问题，因为频率太大会增加
  // 不准确性
  void run();

  static std::mutex g_tl;

  static TimeProvider* g_tp;

 private:
  void setTsc(const timeval& tt);

  void addTimeOffset(timeval& tt, const int &idx);

 protected:
  bool terminate;

  // 在更新时间是是否使用cpu的rdtsc指令，在run方法中，每800微秒就
  // 更新一次时间，那么按照这个算法，是不到1ms的，如果发现更新时两个
  // tsc之间的差值大于了1ms，说明tsc不准确，那么就直接调用gettimeofday
  // 来更新
  bool use_tsc;

 private:
  float cpu_cycle;  // us

  // 这里记录两个时间点和两个tsc是为了方便在读取时和更新线程不用加锁
  volatile int buf_idx;
  timeval tv[2];
  uint64_t tsc[2];

  std::thread* _thread = NULL;
  std::condition_variable cv;
};

}  // namespace sails
}  // namespace base

#endif  // BASE_TIME_PROVIDER_H_

// Copyright (C) 2016 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: time_provider.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2016-03-14 15:56:24



#include <sails/base/time_provider.h>


namespace sails {
namespace base {

std::mutex TimeProvider::g_tl;
TimeProvider* TimeProvider::g_tp = NULL;

TimeProvider* TimeProvider::getInstance() {
  if (!g_tp) {
    std::unique_lock<std::mutex> lock(g_tl);
    if (!g_tp) {
      g_tp = new TimeProvider();
      g_tp->_thread = new std::thread(std::bind(&TimeProvider::run, g_tp));
    }
  }
  return g_tp;
}

TimeProvider::~TimeProvider() {
  std::unique_lock<std::mutex> lock(g_tl);
  terminate = true;
  cv.notify_one();
  g_tp->_thread->join();
}

void TimeProvider::getNow(timeval *tv) {
  int idx = buf_idx;
  *tv = tv[idx];
  // cpu-cycle在两个interval周期后采集完成
  if (cpu_cycle != 0 && use_tsc) {
    addTimeOffset(*tv, idx);
  } else {
    ::gettimeofday(tv, NULL);
  }
}

int64_t TimeProvider::getNowMs() {
    struct timeval tv;
    getNow(&tv);
    return tv.tv_sec * (int64_t)1000 + tv.tv_usec/1000;
}

void TimeProvider::run() {
  while (!terminate) {
    timeval& tt = tv[!buf_idx];

    ::gettimeofday(&tt, NULL);

    setTsc(tt);

    buf_idx = !buf_idx;

    std::unique_lock<std::mutex> lock(g_tl);
    // 修改800时 需对应修改addTimeOffset中offset判读值
    cv.wait_for(lock, std::chrono::milliseconds(800));
  }
}

float TimeProvider::cpuMHz() {
  if (cpu_cycle != 0)
    return 1.0/cpu_cycle;

  return 0;
}

void TimeProvider::setTsc(const timeval& tt) {
    uint32_t low    = 0;
    uint32_t high   = 0;
    rdtsc(low, high);
    uint64_t current_tsc    = ((uint64_t)high << 32) | low;

    uint64_t& last_tsc      = tsc[!buf_idx];
    timeval& last_tt        = tv[buf_idx];

    if (tsc[buf_idx] == 0 || tsc[!buf_idx] == 0) {
        cpu_cycle = 0;
        last_tsc = current_tsc;
    } else {
      time_t sptime = (tt.tv_sec - last_tt.tv_sec)*1000*1000
                      + (tt.tv_usec - last_tt.tv_usec);
      cpu_cycle = (float)(sptime/(current_tsc - tsc[buf_idx]));  // NOLINT
      last_tsc = current_tsc;
    }
}

void TimeProvider::addTimeOffset(timeval& tt, const int &idx) {
    uint32_t low    = 0;
    uint32_t high   = 0;
    rdtsc(low, high);
    uint64_t current_tsc = ((uint64_t)high << 32) | low;
    int64_t t =  (int64_t)(current_tsc - tsc[idx]);
    time_t offset =  (time_t)(t*cpu_cycle);

    // 毫秒级别
    if (t < -1000 || offset > 1000000) {
      use_tsc = false;
      ::gettimeofday(&tt, NULL);
      return;
    }
    tt.tv_usec += offset;
    while (tt.tv_usec >= 1000000) {
      tt.tv_usec -= 1000000;
      tt.tv_sec++;
    }
}


}  // namespace base
}  // namespace sails

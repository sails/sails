// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: cpu_usage.h
// Description: 计算进程和线程cpu使用情况
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-28 09:22:15



#ifndef SAILS_SYSTEM_CPU_USAGE_H
#define SAILS_SYSTEM_CPU_USAGE_H

#include <stdint.h>

namespace sails {
namespace system {

#ifdef __linux__
// 得到进程cpu使用情况,从开始运行到当前状态(/proc/pid/stat
// 先算出cpu总进行时间,再除以进程开始到现在总的实际时间)
bool GetCpuUsageSinceLastCall(int32_t pid, double* cpu);

// 得到进程cpu使用情况,从当然到sample_period毫秒之后这段时间的值(
// 通过算出时间段内cpu总时间和进程运行时间)
bool GetProcessCpuUsage(int32_t pid, uint64_t sample_period, double* cpu);

// 得到线程cpu使用情况
bool GetThreadCpuUsage(
    int32_t pid, int tid, uint64_t sample_period, double* cpu);

// 得到当前开机之后总运行时间(/proc/stat第一行)
bool GetTotalCpuTime(uint64_t* total_cpu_time);

// 得到进程总运行时间(/proc/pid/stat中)
bool GetProcessCpuTime(int32_t pid, uint64_t* process_cpu_time);

// 线程运行时间(/proc/pid/task/tid/stat)
bool GetThreadCpuTime(int32_t pid, int tid, uint64_t* thread_cpu_time);

#endif

}  // namespace system
}  // namespace sails

#endif // SAILS_SYSTEM_CPU_USAGE_H

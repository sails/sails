// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: mem_usage.h
// Description: 得到进程内存使用信息
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-28 13:43:50

#ifndef SAILS_SYSTEM_MEM_USAGE_H_
#define SAILS_SYSTEM_MEM_USAGE_H_

#include <stdint.h>

namespace sails {
namespace system {

// 得到进程内存使用情况(/proc/pid/statm)
bool GetMemoryUsedKiloBytes(
    int32_t pid, uint64_t* vm_size, uint64_t* mem_size);

bool GetMemoryUsedBytes(
    int32_t pid, uint64_t* vm_size, uint64_t* mem_size);


}  // namespace system          
}  // namespace sails

#endif  // SAILS_SYSTEM_MEM_USAGE_H_

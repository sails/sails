// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: mem_usage.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-28 13:47:48



#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <iostream>
# include <unistd.h>

namespace sails {
namespace system {

bool GetMemoryUsedKiloBytes(int32_t pid, uint64_t* vm_size, uint64_t* mem_size)
{
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "/proc/%d", pid);
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
    {
        return false;
    }

    char filename[PATH_MAX];
    snprintf(filename, PATH_MAX, "%s/%s", path, "statm");
    int fd = open(filename, O_RDONLY, 0);
    if (fd == -1)
    {
        return false;
    }
    char buffer[1024];
    int bytes = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    if (bytes <= 0)
    {
        return false;
    }
    buffer[bytes] = '\0';

    long size, resident, share, trs, lrs, drs, dt;
    int ret = sscanf(buffer, "%ld %ld %ld %ld %ld %ld %ld",
            &size, &resident, &share, &trs, &lrs, &drs, &dt);
    if (ret != 7)
    {
        return false;
    }

    // size is 1/4 virtual memory size.
    *vm_size = size * 4;
    // resident is 1/4 memory size.
    *mem_size = resident * 4;

    return true;
}


bool GetMemoryUsedBytes(int32_t pid, uint64_t* vm_size, uint64_t* mem_size)
{
    uint64_t vm_size_in_kb, mem_size_in_kb;
    if (!GetMemoryUsedKiloBytes(pid, &vm_size_in_kb, &mem_size_in_kb))
    {
        return false;
    }
    *vm_size = vm_size_in_kb * 1024;
    *mem_size = mem_size_in_kb * 1024;
    return true;
}

bool GetMemUsage(int32_t pid, uint64_t* vm_size, uint64_t* mem_size)
{
    return GetMemoryUsedKiloBytes(pid, vm_size, mem_size);
}

}  // namespace system          
}  // namespace sails

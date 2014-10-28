#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <gtest/gtest.h>
#include "sails/system/mem_usage.h"

using namespace sails::system;

TEST(MemUsageTest, Test)
{
    pid_t pid = getpid();
    uint64_t vm_size, mem_size;
    EXPECT_TRUE(GetMemoryUsedKiloBytes(pid, &vm_size, &mem_size));
    printf("vm size: %ldk\n", vm_size);
    printf("mem size: %ldk\n", mem_size);
    EXPECT_TRUE(GetMemoryUsedBytes(pid, &vm_size, &mem_size));
    printf("vm size: %ldbyte\n", vm_size);
    printf("mem size: %ldbyte\n", mem_size);
}

int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();  
}

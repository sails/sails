#include <stdlib.h>
#include <sys/types.h>
#include <iostream>
#include <gtest/gtest.h>
#include "sails/system/cpu_usage.h"


using namespace sails::system;

#define FLAGS_pid 1228
#define FLAGS_sample_period 1000

TEST(CPU, CpuRate)
{
  double cpu = 0;
  if (GetCpuUsageSinceLastCall(getpid(), &cpu))
  {
    std::cout << "current process cpu: " << cpu << std::endl;
  }
  if (GetCpuUsageSinceLastCall(FLAGS_pid, &cpu))
  {
    std::cout << "cpu usage of process " << FLAGS_pid << ": " << cpu << std::endl;
  }

  if (GetProcessCpuUsage(getpid(), FLAGS_sample_period, &cpu))
  {
    std::cout << "current process cpu=" << cpu << std::endl;
  }
  if (GetProcessCpuUsage(FLAGS_pid, FLAGS_sample_period, &cpu))
  {
    std::cout << "cpu usage of process=" << FLAGS_pid << "cpu=" << cpu << std::endl;
  }
}

int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();  
}

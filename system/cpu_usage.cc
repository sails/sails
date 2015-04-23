#include "sails/system/cpu_usage.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <string>
#include <vector>
#include "sails/base/string.h"
#include "sails/log/logging.h"

namespace sails {
namespace system {

#ifdef __LINUX__

#define LINUX_VERSION(x, y, z)   (0x10000*(x) + 0x100*(y) + z)
#define PATH_MAX 1024

#ifndef AT_CLKTCK
#define AT_CLKTCK       17  // frequency of times()
#endif
#ifndef NOTE_NOT_FOUND
#define NOTE_NOT_FOUND  42
#endif

extern char **environ;

// namespace common {

static inline int GetLinuxVersion() {
  static struct utsname uts;
  int x = 0, y = 0, z = 0;    /* cleared in case sscanf() < 3 */

  if (uname(&uts) == -1)
    return -1;
  if (sscanf(uts.release, "%d.%d.%d", &x, &y, &z) == 3) {
    return LINUX_VERSION(x, y, z);
  }
  return 0;
}

static inline bool IsPathValid(char* path) {
  struct stat statbuf;
  if (stat(path, &statbuf)) {
    log::LoggerFactory::getLog("server")->error(
        "stat error, permission problem, path=%s, error=%s",
        path, strerror(errno));
    return false;
  }

  return true;
}

static bool OpenAndReadFile(
    char* filename, char* buffer, int buff_len, int* bytes) {
  int fd = open(filename, O_RDONLY, 0);
  if (fd == -1) {
    log::LoggerFactory::getLog("server")->error(
        "open file error, filename=%s", filename);
    return false;
  }

  *bytes = read(fd, buffer, buff_len - 1);
  close(fd);
  if (*bytes <= 0) {
    log::LoggerFactory::getLog("server")->error(
        "read file error, bytes:%d", *bytes);
    return false;
  }

  return true;
}

static void SkipBracket(char* buffer, char** skiped_start_point) {
  *skiped_start_point = strchr(buffer, '(') + 1;
  char* d = strrchr(*skiped_start_point, ')');
  *skiped_start_point = d + 2;  // skip ") "
}

// For ELF executables, notes are pushed before environment and args
static unsigned long FindElfNote(unsigned long name) {
  unsigned long *ep = (unsigned long *)environ;
  while (*ep) ep++;
  ep++;
  while (*ep) {
    if (ep[0] == name) return ep[1];
    ep += 2;
  }
  return NOTE_NOT_FOUND;
}

static unsigned long GetHertz() {
  // Check the linux kernel version support
  if (GetLinuxVersion() <= LINUX_VERSION(2, 4, 0))
    return 0;
  uint64_t hertz = FindElfNote(AT_CLKTCK);
  return hertz != NOTE_NOT_FOUND ? hertz : 0;
}

static bool uptime(double *uptime_secs, double *idle_secs) {
  int fd = open("/proc/uptime", O_RDONLY);
  if (fd == -1)
    return false;
  char buffer[2048] = {0};
  lseek(fd, 0L, SEEK_SET);
  int bytes = read(fd, buffer, sizeof(buffer) - 1);
  close(fd);
  if (bytes < 0)
    return false;
  buffer[bytes] = '\0';

  double up = 0, idle = 0;
  char* savelocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "C");
  if (sscanf(buffer, "%lf %lf", &up, &idle) < 2) {
    setlocale(LC_NUMERIC, savelocale);
    return false;
  }
  setlocale(LC_NUMERIC, savelocale);
  if (uptime_secs) *uptime_secs = up;
  if (idle_secs) *idle_secs = idle;
  return true;
}

static bool StringToUINT64(const std::string& str, uint64_t* value ) {
  int64_t temp = strtol(str.c_str(), (char**)NULL, 10);
  if (temp >= 0) {
    *value = temp;
  } else {
    log::LoggerFactory::getLog("server")->error(
        "StringToUINT64 str:%s value:%ld", str.c_str(), temp);
    return false;
  }
  return true;
}

static unsigned int GetLogicalCpuNumber() {
    return sysconf(_SC_NPROCESSORS_ONLN);
}


bool GetCpuUsageSinceLastCall(int32_t pid, double* cpu) {
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/proc/%d", pid);
  if (!IsPathValid(path)) {
    return false;
  }

  int buff_len = 1024;
  char buffer[1024] = {0};
  int bytes = 0;
  char filename[PATH_MAX];
  snprintf(filename, sizeof(filename), "%s/%s", path, "stat");
  bool file_ret = OpenAndReadFile(filename, buffer, buff_len, &bytes);
  if (!file_ret) {
    return false;
  }
  buffer[bytes] = '\0';

  char* s = strchr(buffer, '(') + 1;
  char* d = strrchr(s, ')');
  s = d + 2;  // skip ") "

  std::vector<std::string> fields;
  sails::base::SplitString(std::string(s), " ", &fields);
  uint64_t utime, stime, cutime, cstime, start_time;
  bool ret = StringToUINT64(fields[11], &utime);
  ret = ret && StringToUINT64(fields[12], &stime);
  ret = ret && StringToUINT64(fields[13], &cutime);
  ret = ret && StringToUINT64(fields[14], &cstime);
  ret = ret && StringToUINT64(fields[19], &start_time);
  if (!ret)
    return false;

  double uptime_secs, idle_secs;
  if (!uptime(&uptime_secs, &idle_secs))
    return false;

  uint64_t seconds_since_boot = static_cast<uint64_t>(uptime_secs);
  // frequency of times()
  uint64_t hertz = GetHertz();
  if (hertz == 0) hertz = 100;
  // seconds of process life
  uint64_t seconds = seconds_since_boot - start_time / hertz;
  uint64_t total_time = utime + stime + cutime + cstime;

  // scaled %cpu, 999 means 99.9%
  *cpu = 0;
  if (seconds)
    *cpu = (total_time * 1000ULL / hertz) / seconds;
  *cpu = *cpu / 10;

  return true;
}

// For system cpu usage
bool GetTotalCpuTime(uint64_t* total_cpu_time) {
  // Check the linux kernel version support
  if (GetLinuxVersion() < LINUX_VERSION(2, 6, 24)) {
    return false;
  }

  // --- 0. check /proc permission
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/%s", "proc");
  if (!IsPathValid(path)) {
    return false;
  }

  // --- 1. read /proc/stat
  int buff_len = 1024;
  char buffer[1024] = {0};
  int bytes = 0;
  char filename[PATH_MAX];
  snprintf(filename, sizeof(filename), "/%s/%s", "proc", "stat");
  bool file_ret = OpenAndReadFile(filename, buffer, buff_len, &bytes);
  if (!file_ret) {
    return false;
  }
  buffer[bytes] = '\0';

  char* s = buffer;
  char* d = strchr(buffer, '\n');

  // --- 2. split string (user nice system idle
  // iowait irq softirq stealstolen guest)
  std::vector<std::string> fields;
  std::string cpustr(s, d-s);
  sails::base::SplitString(cpustr, " ", &fields);

  uint64_t user, nice, system, idle, iowait, irq, softirq, stealstolen, guest;
  bool ret = StringToUINT64(fields[1], &user);
  ret = ret && StringToUINT64(fields[2], &nice);
  ret = ret && StringToUINT64(fields[3], &system);
  ret = ret && StringToUINT64(fields[4], &idle);
  ret = ret && StringToUINT64(fields[5], &iowait);
  ret = ret && StringToUINT64(fields[6], &irq);
  ret = ret && StringToUINT64(fields[7], &softirq);
  ret = ret && StringToUINT64(fields[8], &stealstolen);
  ret = ret && StringToUINT64(fields[9], &guest);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "get param error, fields size=%d", fields.size());
    return false;
  }

  *total_cpu_time = user + nice + system + idle + iowait +
                    irq + softirq + stealstolen + guest;

  return true;
}

bool GetProcessCpuTime(int32_t pid, uint64_t* process_cpu_time) {
  // Check the linux kernel version support
  if (GetLinuxVersion() < LINUX_VERSION(2, 6, 24)) {
    return false;
  }

  // --- 0. check /proc/pid permission
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/proc/%d", pid);
  if (!IsPathValid(path)) {
    return false;
  }

  int buff_len = 1024;
  char buffer[1024] = {0};
  int bytes = 0;
  char filename[PATH_MAX];
  snprintf(filename, sizeof(filename), "%s/%s", path, "stat");
  bool file_ret = OpenAndReadFile(filename, buffer, buff_len, &bytes);
  if (!file_ret) {
    return false;
  }
  buffer[bytes] = '\0';

  // skip ") "
  char* s = buffer;
  SkipBracket(buffer, &s);

  std::vector<std::string> fields;
  sails::base::SplitString(std::string(s), " ", &fields);
  uint64_t utime, stime, cutime, cstime, start_time;
  bool ret = StringToUINT64(fields[11], &utime);
  ret = ret && StringToUINT64(fields[12], &stime);
  ret = ret && StringToUINT64(fields[13], &cutime);
  ret = ret && StringToUINT64(fields[14], &cstime);
  ret = ret && StringToUINT64(fields[19], &start_time);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "get param error, fields size=%u", fields.size());
    return false;
  }

  *process_cpu_time = utime + stime + cutime + cstime;
  return true;
}

bool GetThreadCpuTime(int32_t pid, int tid, uint64_t* thread_cpu_time) {
  // Check the linux kernel version support
  if (GetLinuxVersion() < LINUX_VERSION(2, 6, 24)) {
    return false;
  }

  // --- 0. check /proc/pid/task/tid permission
  char path[PATH_MAX];
  snprintf(path, sizeof(path), "/proc/%d/task/%d", pid, tid);
  if (!IsPathValid(path)) {
    return false;
  }

  // --- 1. read /proc/pid/task/tid/stat
  int buff_len = 1024;
  char buffer[1024] = {0};
  int bytes = 0;
  char filename[PATH_MAX];
  snprintf(filename, sizeof(filename), "%s/%s", path, "stat");

  bool file_ret = OpenAndReadFile(filename, buffer, buff_len, &bytes);
  if (!file_ret) {
    return false;
  }
  buffer[bytes] = '\0';

  // skip ") "
  char* s = buffer;
  SkipBracket(buffer, &s);

  std::vector<std::string> fields;
  sails::base::SplitString(std::string(s), " ", &fields);
  uint64_t utime, stime;
  bool ret = StringToUINT64(fields[11], &utime);
  ret = ret && StringToUINT64(fields[12], &stime);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "get param error, fields size=%u", fields.size());
    return false;
  }

  *thread_cpu_time = utime + stime;
  return true;
}

bool GetProcessCpuUsage(int32_t pid, uint64_t sample_period, double* cpu) {
  // Check the linux kernel version support
  if (GetLinuxVersion() < LINUX_VERSION(2, 6, 24)) {
    return false;
  }

  // first time
  uint64_t total_cpu_time_first = 0;
  bool ret = GetTotalCpuTime(&total_cpu_time_first);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "First GetTotalCpuTime error");
    return false;
  }

  uint64_t process_cpu_time_first = 0;
  ret = GetProcessCpuTime(pid, &process_cpu_time_first);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "First GetProcessCpuTime error");
    return false;
  }

  // usleep
  usleep(sample_period * 1000);

  // second time
  uint64_t total_cpu_time_second = 0;
  ret = GetTotalCpuTime(&total_cpu_time_second);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "Second GetTotalCpuTime error");
    return false;
  }

  uint64_t process_cpu_time_second = 0;
  ret = GetProcessCpuTime(pid, &process_cpu_time_second);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "Second GetProcessCpuTime error");
    return false;
  }

  *cpu = 0;
  if (total_cpu_time_second == total_cpu_time_first) {
    log::LoggerFactory::getLog("server")->error(
        "GetTotalCpuTime same");
    return false;
  }

  *cpu = 100 * static_cast<double>(
      process_cpu_time_second - process_cpu_time_first) /
         (total_cpu_time_second - total_cpu_time_first) * GetLogicalCpuNumber();

  return true;
}

bool GetThreadCpuUsage(
    int32_t pid, int tid, uint64_t sample_period, double* cpu) {
  // Check the linux kernel version support
  if (GetLinuxVersion() < LINUX_VERSION(2, 6, 24)) {
    return false;
  }

  // first time
  uint64_t total_cpu_time_first = 0;
  bool ret = GetTotalCpuTime(&total_cpu_time_first);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "First GetTotalCpuTime error");
    return false;
  }

  uint64_t thread_cpu_time_first = 0;
  ret = GetThreadCpuTime(pid, tid, &thread_cpu_time_first);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "First GetThreadCpuTime error");
    return false;
  }

  // usleep
  usleep(sample_period * 1000);

  // second time
  uint64_t total_cpu_time_second = 0;
  ret = GetTotalCpuTime(&total_cpu_time_second);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "Second GetTotalCpuTime error");
    return false;
  }

  uint64_t thread_cpu_time_second = 0;
  ret = GetThreadCpuTime(pid, tid, &thread_cpu_time_second);
  if (!ret) {
    log::LoggerFactory::getLog("server")->error(
        "Second GetThreadCpuTime error");
    return false;
  }

  *cpu = 0;
  if (total_cpu_time_second == total_cpu_time_first) {
    log::LoggerFactory::getLog("server")->error("GetTotalCpuTime same");
    return false;
  }

  *cpu = 100 * static_cast<double>(
      thread_cpu_time_second - thread_cpu_time_first) /
         (total_cpu_time_second - total_cpu_time_first) * GetLogicalCpuNumber();

  return true;
}

bool GetCpuUsage(int32_t pid, double* cpu) {
  return GetCpuUsageSinceLastCall(pid, cpu);
}

#endif

}  // namespace system
}  // namespace sails

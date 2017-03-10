// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: logging.h
// Description: 可以直接通过new来得到一个logger,
//              但是推荐通过LoggerFactory达到单例目的
//              factory生成的日志默认是debug级别,可以通过
//              修改log.conf:LogLevel=info重新定义级别,合法的关键字:
//              debug, info ,warn, error,修改后10秒生效
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:18:57



#ifndef LOG_LOGGING_H_
#define LOG_LOGGING_H_

#include <stdio.h>
#include <time.h>
#include <string>
#include <map>
#include <mutex>  // NOLINT



#define DEBUG_LOG(t, ...) {                                             \
    if (sails::log::LoggerFactory::instance()->getLog(t)->getLevel() <= \
        sails::log::Logger::LOG_LEVEL_DEBUG) {                          \
      sails::log::LoggerFactory::instance()->getLog(t)->debug(__VA_ARGS__); \
    }                                                                   \
}
#define INFO_LOG(t, ...) {                                              \
    if (sails::log::LoggerFactory::instance()->getLog(t)->getLevel() <= \
        sails::log::Logger::LOG_LEVEL_INFO) {                           \
      sails::log::LoggerFactory::instance()->getLog(t)->info(__VA_ARGS__); \
    }                                                                   \
}
#define WARN_LOG(t, ...) {                                              \
    if (sails::log::LoggerFactory::instance()->getLog(t)->getLevel() <= \
        sails::log::Logger::LOG_LEVEL_WARN) {                           \
      sails::log::LoggerFactory::instance()->getLog(t)->warn(__VA_ARGS__); \
    }                                                                   \
}
#define ERROR_LOG(t, ...) {                                             \
    if (sails::log::LoggerFactory::instance()->getLog(t)->getLevel() <= \
        sails::log::Logger::LOG_LEVEL_ERROR) {                          \
      sails::log::LoggerFactory::instance()->getLog(t)->error(__VA_ARGS__); \
    }                                                                   \
}


namespace sails {
namespace log {

#define MAX_FILENAME_LEN 1000


class Logger {
 public:
  enum LogLevel{
    LOG_LEVEL_NONE =-1,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
  };

  enum SAVEMODE{
    SPLIT_NONE,
    SPLIT_MONTH,
    SPLIT_DAY,
    SPLIT_HOUR
  };

  enum LogType {
    LOG_FILE = 1,
    LOG_CONSOLE = 2,
  };

  // 默认应该是直接输出到控制台，参数顺序按照这个定
  explicit Logger(const char* name = "default",
                  LogLevel level = LOG_LEVEL_DEBUG,
                  int type = LogType::LOG_CONSOLE,
                  SAVEMODE mode = SAVEMODE::SPLIT_NONE,
                  const char* path = "./log/");

  void debug(const char* format, ...);
  void info(const char* format, ...);
  void warn(const char* format, ...);
  void error(const char* format, ...);

  void setMode(SAVEMODE mode) {
    this->save_mode = mode;
  }
  void setLevel(LogLevel level) {
    this->level = level;
  }
  LogLevel getLevel() {
    return level;
  }
  void setLogType(int logType) {
    this->logType = logType;
  }
  void setLogPath(const char* path) {
    snprintf(this->path, sizeof(this->path), "%s", path);
    ensure_directory_exist();
  }

 private:
  void output(Logger::LogLevel level, const char* format, va_list ap);
  void set_msg_prefix(Logger::LogLevel level, char *msg);
  void set_filename_by_savemode(char* filename, int len);
  bool ensure_directory_exist();

  LogLevel level;
  int logType;
  char name[MAX_FILENAME_LEN];
  char path[MAX_FILENAME_LEN];
  SAVEMODE save_mode;
  std::mutex writeMutex;
};




// 以单例模式
class LoggerFactory {
 private:
  LoggerFactory();
  ~LoggerFactory();

 public:
  static LoggerFactory* instance();
  Logger* getLog(std::string log_name);


  // 全局配置
  struct LogConfig {
    std::string path;
    Logger::LogLevel level;
    int logType;  // 可以是stdout和file两种1,2,3
    Logger::SAVEMODE split;
  };

 private:
  static LoggerFactory* _pInstance;
  // for log
  std::string path;
  LogConfig config;
  static std::mutex logMutex;
  std::map<std::string, Logger*> log_map;
  class CGarbo {  // 它的唯一工作就是在析构函数中删除LoggerFactory的实例
   public:
    ~CGarbo() {
      if (LoggerFactory::_pInstance) {
        delete LoggerFactory::_pInstance;
        LoggerFactory::_pInstance = NULL;
      }
    }
  };
};

}  // namespace log
}  // namespace sails

#endif  // LOG_LOGGING_H_

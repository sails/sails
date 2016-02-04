// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: logging.h
// Description: 可以直接通过new来得到一个logger,
//              但是推荐通过LoggerFactory达到单例目的
//              factory生成的日志默认是info级别,可以通过
//              修改log.conf:LogLevel=debug重新定义级别,合法的关键字:
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



#define DEBUG_LOG(t, ...) { \
    sails::log::LoggerFactory::getLog(t)->debug(__VA_ARGS__);   \
  }
#define INFO_LOG(t, ...) { \
    sails::log::LoggerFactory::getLog(t)->info(__VA_ARGS__);    \
}
#define WARN_LOG(t, ...) { \
    sails::log::LoggerFactory::getLog(t)->warn(__VA_ARGS__);    \
}
#define ERROR_LOG(t, ...) { \
    sails::log::LoggerFactory::getLog(t)->error(__VA_ARGS__);   \
}
#define DEBUG_HLOG(t, ...) { \
    sails::log::LoggerFactory::getLogH(t)->debug(__VA_ARGS__);   \
  }
#define INFO_HLOG(t, ...) { \
    sails::log::LoggerFactory::getLogH(t)->info(__VA_ARGS__);    \
}
#define WARN_HLOG(t, ...) { \
    sails::log::LoggerFactory::getLogH(t)->warn(__VA_ARGS__);    \
}
#define ERROR_HLOG(t, ...) { \
    sails::log::LoggerFactory::getLogH(t)->error(__VA_ARGS__);   \
}
#define DEBUG_DLOG(t, ...) { \
    sails::log::LoggerFactory::getLogD(t)->debug(__VA_ARGS__);   \
  }
#define INFO_DLOG(t, ...) { \
    sails::log::LoggerFactory::getLogD(t)->info(__VA_ARGS__);    \
}
#define WARN_DLOG(t, ...) { \
    sails::log::LoggerFactory::getLogD(t)->warn(__VA_ARGS__);    \
}
#define ERROR_DLOG(t, ...) { \
    sails::log::LoggerFactory::getLogD(t)->error(__VA_ARGS__);   \
}
#define DEBUG_MLOG(t, ...) { \
    sails::log::LoggerFactory::getLogM(t)->debug(__VA_ARGS__);   \
  }
#define INFO_MLOG(t, ...) { \
    sails::log::LoggerFactory::getLogM(t)->info(__VA_ARGS__);    \
}
#define WARN_MLOG(t, ...) { \
    sails::log::LoggerFactory::getLogM(t)->warn(__VA_ARGS__);    \
}
#define ERROR_MLOG(t, ...) { \
    sails::log::LoggerFactory::getLogM(t)->error(__VA_ARGS__);   \
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

  explicit Logger(LogLevel level);
  Logger(LogLevel level, const char *name);
  Logger(LogLevel level, const char *name, SAVEMODE mode);

  void debug(const char* format, ...);
  void info(const char* format, ...);
  void warn(const char* format, ...);
  void error(const char* format, ...);

 private:
  void output(Logger::LogLevel level, const char* format, va_list ap);
  void set_msg_prefix(Logger::LogLevel level, char *msg);
  void set_filename_by_savemode(char* filename, int len);
  void check_loginfo();
  Logger::LogLevel get_level_by_name(const char *name);
  void set_file_path();
  bool ensure_directory_exist();

  LogLevel level;
  char name[MAX_FILENAME_LEN];
  char path[MAX_FILENAME_LEN];
  SAVEMODE save_mode;
  static char log_config_file[100];
  time_t update_loginfo_time;
  std::mutex writeMutex;
};


// 以单例模式
class LoggerFactory {
 private:
  LoggerFactory();
  ~LoggerFactory();

 public:
  static Logger* getLog(std::string log_name);  // SPLIT_NONE
  static Logger* getLogD(std::string log_name);  // SPLIT_DAY
  static Logger* getLogH(std::string log_name);  // SPLIT_HOUR
  static Logger* getLogM(std::string log_name);  // SPLIT_MONTH

 private:
  Logger* getLog(std::string log_name, Logger::SAVEMODE save_mode);
  static LoggerFactory* instance();
  static LoggerFactory* _pInstance;
  // for log
  std::string path;
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

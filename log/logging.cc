// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: logging.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 11:21:00



#include "sails/log/logging.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#ifdef __linux__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif
#include "sails/base/time_t.h"
#include "sails/base/string.h"
#include "sails/base/filesys.h"
#ifdef __ANDROID__
#include <android/log.h>
#endif

namespace sails {
namespace log {


char Logger::log_config_file[100] = "./log.conf";

Logger::Logger(Logger::LogLevel level) {
  this->level = level;
  this->save_mode = Logger::SPLIT_NONE;
  memset(name, '\0', MAX_FILENAME_LEN);
  update_loginfo_time = 0;
}

Logger::Logger(LogLevel level, const char *name) {
  assert(name);

  this->level = level;
  this->save_mode = Logger::SPLIT_NONE;
  memset(this->name, '\0', MAX_FILENAME_LEN);
  update_loginfo_time = 0;

  strncpy(this->name, name, strlen(name));
  set_file_path();
  assert(ensure_directory_exist());
}

Logger::Logger(LogLevel level, const char *filename, SAVEMODE mode) {
  assert(filename);

  this->level = level;
  this->save_mode = mode;
  memset(this->name, '\0', MAX_FILENAME_LEN);
  update_loginfo_time = 0;

  strncpy(this->name, filename, strlen(filename));
  set_file_path();
  assert(ensure_directory_exist());
}

void Logger::debug(const char *format, ...) {
  check_loginfo();
  if (LOG_LEVEL_DEBUG >= level) {
    va_list ap;
    va_start(ap, format);
    this->output(Logger::LOG_LEVEL_DEBUG, format, ap);
    va_end(ap);
  }
}

void Logger::info(const char *format, ...) {
  check_loginfo();
  if (LOG_LEVEL_INFO >= level) {
    va_list ap;
    va_start(ap, format);
    this->output(Logger::LOG_LEVEL_INFO, format, ap);
    va_end(ap);
  }
}

void Logger::warn(const char *format, ...) {
  check_loginfo();
  if (LOG_LEVEL_WARN >= level) {
    va_list ap;
    va_start(ap, format);
    this->output(Logger::LOG_LEVEL_WARN, format, ap);
    va_end(ap);
  }
}

void Logger::error(const char *format, ...) {
  check_loginfo();
  if (LOG_LEVEL_ERROR >= level) {
    va_list ap;
    va_start(ap, format);
    this->output(Logger::LOG_LEVEL_ERROR, format, ap);
    va_end(ap);
  }
}

void Logger::output(Logger::LogLevel level, const char *format, va_list ap) {
  char msg[1000];
  memset(msg, '\0', 1000);
  set_msg_prefix(level, msg);
  vsnprintf(msg+strlen(msg), 900, format, ap);  // NOLINT'
  msg[strlen(msg)] = '\n';

  char filename[MAX_FILENAME_LEN];
  memset(filename, '\0', MAX_FILENAME_LEN);
  if (strlen(this->name) > 0) {
    set_filename_by_savemode(filename, MAX_FILENAME_LEN);
    // 防止多个线程同时写一个文件，fopen会失败
    std::unique_lock<std::mutex> writelocker(writeMutex);
#ifdef __linux__
#ifdef __ANDROID__
    // ANDROID_LOG_DEBUG为3
    __android_log_print(level+2, this->name, msg);
#else
    int write_fd = -1;
    if ((write_fd=open(filename, O_WRONLY|O_APPEND|O_CREAT,
                       S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)) >= 3) {
      write(write_fd, msg, strlen(msg));
      close(write_fd);
    } else {
      char err_msg[40] = {'\0'};
      snprintf(err_msg, sizeof(err_msg),
              "can't open file %s to write, %s\n",
              filename, strerror(errno));  // NOLINT'
      write(2, err_msg, strlen(err_msg));
    }
#endif
#else
    FILE* file = fopen(filename, "a");
    if (file != NULL) {
      fwrite(msg, 1, strlen(msg), file);
      fclose(file);
      file = NULL;
    } else {
      char err_msg[MAX_FILENAME_LEN+30];
      snprintf(err_msg, sizeof(err_msg), "can't open file %s to write, %s\n",
              filename, strerror(errno));  // NOLINT'
      fprintf(stderr, "%s", err_msg);
    }
#endif

  } else {
#ifdef __linux__
    write(1, msg, strlen(msg));
#else
    printf("%s\n", msg);
#endif
  }
}

void Logger::set_filename_by_savemode(char *filename, int len) {
  if (strlen(this->name) > 0
     && filename != NULL) {
    snprintf(filename, len, "%s", this->name);

    char time[30];
    if (base::TimeT::time_with_millisecond(time, 30) <= 0) {
      return;
    }

    if (this->save_mode != Logger::SPLIT_NONE) {
      switch (this->save_mode) {
        case SPLIT_MONTH:
          strncpy(filename+strlen(filename), time, 7);
          break;
        case SPLIT_DAY:
          strncpy(filename+strlen(filename), time, 10);
          break;
        case SPLIT_HOUR:
          strncpy(filename+strlen(filename), time, 13);
          break;
        case SPLIT_NONE:
          break;
      }
    }

    filename[strlen(filename)] = '.';
    strncpy(filename+strlen(filename), "log", 3);
  }
}

void Logger::set_msg_prefix(Logger::LogLevel level, char *msg) {
  if (base::TimeT::time_with_millisecond(msg, 1000) <= 0) {
    return;
  }
  msg[strlen(msg)] = ' ';
  if (level == LOG_LEVEL_DEBUG) {
    strncpy(msg+strlen(msg), "[debug]", 7);
  } else if (level == LOG_LEVEL_INFO) {
    strncpy(msg+strlen(msg), "[info]", 6);
  } else if (level == LOG_LEVEL_WARN) {
    strncpy(msg+strlen(msg), "[warn]", 6);
  } else if (level == LOG_LEVEL_ERROR) {
    strncpy(msg+strlen(msg), "[error]", 7);
  }
  msg[strlen(msg)] = ':';
}


void Logger::check_loginfo() {
  time_t current_time = time(NULL);
  if (current_time - update_loginfo_time > 10) {
    FILE* file = fopen(log_config_file, "r");
    if (file != NULL) {
      char buf[100];
      while (fgets(buf, 100, file) != NULL) {
        int size = strlen(buf);
        if (size > 0) {
          if (buf[size-1] == '\n') {  // linux file
            buf[size-1] = '\0';
          } else if (buf[size-2] == '\r'
                     && buf[size-1] == '\n')  {  // windows file
            buf[size-2] = '\0';
            buf[size-1] = '\0';
          }
          char name[31] = {'\0'};
          char value[31] = {'\0'};
          sscanf(buf, "%30[^=]=%30s", name, value);
          if (strncmp(name, "LogLevel", 8) == 0) {
            Logger::LogLevel setlevel = get_level_by_name(value);
            if (setlevel != Logger::LOG_LEVEL_NONE) {
              // printf("set level %d\n", setlevel);
              this->level = setlevel;
            }
          }
        }
        memset(buf, '\0', 100);
      }

      fclose(file);
    }
    update_loginfo_time = current_time;
  }
}

Logger::LogLevel Logger::get_level_by_name(const char *name) {
  if (name == NULL || strlen(name) == 0) {
    return Logger::LOG_LEVEL_NONE;
  }
  // 去掉前面的空字符，第一个[a-z][A-Z]
  int len = strlen(name);
  const char* p = name;
  for (int i = 0; i < len; i++) {
    if ((p[i] >= 'a' && p[i] <= 'z')
        || (p[i] >= 'A' && p[i] <= 'Z')) {
      break;
    } else {
      p++;
    }
  }
  if (strncasecmp("debug", p, 5) == 0) {
    return Logger::LOG_LEVEL_DEBUG;
  } else if (strncasecmp("info", p, 4) == 0) {
    return Logger::LOG_LEVEL_INFO;
  } else if (strncasecmp("warn", p, 4) == 0) {
    return Logger::LOG_LEVEL_WARN;
  } else if (strncasecmp("error", p, 5) == 0) {
    return Logger::LOG_LEVEL_ERROR;
  }
  return Logger::LOG_LEVEL_NONE;
}

void Logger::set_file_path() {
  memset(this->path, '\0', MAX_FILENAME_LEN);
  if (strlen(name) > 0) {
    int index = base::last_index_of(name, '/');
    if (index > 0) {
      strncpy(path, name, index);
    } else {
      strncpy(path, "./log", 5);
    }
  }
}

bool Logger::ensure_directory_exist() {
  return base::make_directory(this->path);
}



///////////////////////////// log facotry /////////////////////////////////

LoggerFactory* LoggerFactory::_pInstance = 0;
std::mutex LoggerFactory::logMutex;

LoggerFactory::LoggerFactory() : path("./log") {
}

LoggerFactory::~LoggerFactory() {
  std::unique_lock<std::mutex> locker(LoggerFactory::logMutex);
  for (auto& item : log_map) {
    if (item.second != NULL) {
      delete item.second;
    }
  }
  log_map.clear();
}


Logger* LoggerFactory::getLog(std::string log_name,
                              Logger::SAVEMODE save_mode) {
  std::unique_lock<std::mutex> locker(LoggerFactory::logMutex);
  std::map<std::string, Logger*>::iterator it;
  if ((it=log_map.find(log_name)) != log_map.end()) {
    return it->second;
  } else {
    char name[200] = {'\0'};
#ifdef __ANDROID__
    snprintf(name, sizeof(name),
             "%s", log_name.c_str());
#else
    snprintf(name, sizeof(name),
             "%s/%s", path.c_str(), log_name.c_str());
#endif
    Logger* logger = new Logger(Logger::LOG_LEVEL_INFO,
                                name, save_mode);
    log_map.insert(
        std::pair<std::string, Logger*>(log_name, logger));  // NOLINT'
    return logger;
  }
}


LoggerFactory* LoggerFactory::instance() {
  if (_pInstance == NULL) {
    std::unique_lock<std::mutex> locker(LoggerFactory::logMutex);
    if (_pInstance == NULL) {
        static LoggerFactory loggerFactory;
        _pInstance = &loggerFactory;
    }
  }
  return _pInstance;
}

Logger* LoggerFactory::getLog(std::string log_name) {
  return LoggerFactory::instance()->getLog(log_name, Logger::SPLIT_NONE);
}

Logger* LoggerFactory::getLogD(std::string log_name) {
  return LoggerFactory::instance()->getLog(log_name, Logger::SPLIT_DAY);
}

Logger* LoggerFactory::getLogH(std::string log_name) {
  return LoggerFactory::instance()->getLog(log_name, Logger::SPLIT_HOUR);
}

Logger* LoggerFactory::getLogM(std::string log_name) {
  return LoggerFactory::instance()->getLog(log_name, Logger::SPLIT_MONTH);
}

}  // namespace log
}  // namespace sails





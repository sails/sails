// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: filesys.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:05:23



#include "sails/base/filesys.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if (defined __linux__) || (defined __APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#elif WIN32
#include <io.h>
#include <windows.h>
#include <direct.h>
#endif

namespace sails {
namespace base {


bool make_directory(const char* path) {
  char dir_name[1000];
  char path_name[1000];
  memset(dir_name, '\0', 1000);
  memset(path_name, '\0', 1000);
  strncpy(path_name, path, strlen(path));
  int len = strlen(path_name);
  if (path_name[len-1] != '/') {
    // strcat(path_name, "/");
    snprintf(path_name+len, 2, "%s", "/");  // NOLINT'
  }

  len = strlen(path_name);
  for (int i = 1; i < len; i++) {
    if (path_name[i] == '/') {
      memset(dir_name, '\0', 1000);
      strncpy(dir_name, path, i+1);
#if (defined __linux__) || (defined __APPLE__)
      if (access(dir_name, R_OK) != 0) {
        if (mkdir(dir_name, 0766) != 0) {
          return false;
        }
      }
#elif WIN32
      if (_access(dir_name, 0) != 0) {
        if (_mkdir(dir_name) != 0) {
          return false;
        }
      }
#endif
    }
  }
  return true;
}

}  // namespace base
}  // namespace sails

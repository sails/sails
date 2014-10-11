// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: filesys.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:05:04



#ifndef SAILS_BASE_FILESYS_H_
#define SAILS_BASE_FILESYS_H_


namespace sails {
namespace base {

char* get_dir_separator(char *separator);

bool make_directory(const char* path);

}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_FILESYS_H_

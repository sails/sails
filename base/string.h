// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: string.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:16:16



#ifndef SAILS_BASE_STRING_H_
#define SAILS_BASE_STRING_H_

#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <vector>
#include <string>

namespace sails {
namespace base {

// cat n byte of src to dst, len stand for the maxlen of dst
size_t strlncat(char *dst, size_t len, const char *src, size_t n);

size_t strlcat(char *dst, const char *src, size_t len);

size_t strlncpy(char *dst, size_t len, const char *src, size_t n);

size_t strlcpy(char *dst, const char *src, size_t len);


int first_index_of(const char* src, char c);
int first_index_of_substr(const char* src, const char* substr);
int last_index_of(const char* src, char c);

char* url_encode(const char *source_str, char *encode_str);

std::vector<std::string> split(const std::string& str, const char* c);

void SplitString(const std::string& full,
                 const char* delim,
                 std::vector<std::string>* result);

// 字符转换 code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
/*
int code_convert(
    char *from_charset, char *to_charset,
    char *inbuf, int inlen, char *outbuf, int outlen);
*/
}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_STRING_H_
















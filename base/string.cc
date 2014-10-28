// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: string.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:20:32



#include "sails/base/string.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

namespace sails {
namespace base {


size_t
strlncat(char *dst, size_t len, const char *src, size_t n) {
  size_t slen;
  size_t dlen;

  slen = strnlen(src, n);
  dlen = strnlen(dst, len);

  if (dlen < len) {
    size_t rlen = len - dlen;
    size_t ncpy = slen < rlen ? slen : (rlen - 1);
    memcpy(dst + dlen, src, ncpy);
    dst[dlen + ncpy] = '\0';
  }

  assert(len > slen + dlen);
  return slen + dlen;
}

size_t strlcat(char *dst, const char *src, size_t len) {
  return strlncat(dst, len, src, (size_t) -1);
}

size_t strlncpy(char *dst, size_t len, const char *src, size_t n) {
  size_t slen;

  slen = strnlen(src, n);

  if (len > 0) {
    size_t ncpy = slen < len ? slen : (len - 1);
    memcpy(dst, src, ncpy);
    dst[ncpy] = '\0';
  }

  assert(len > slen);
  return slen;
}

size_t
strlcpy(char *dst, const char *src, size_t len) {
  return strlncpy(dst, len, src, (size_t) -1);
}

int last_index_of(const char* src, char c) {
  int index = -1;
  int len = 0;
  if ((len=strlen(src)) > 0) {
    for (int i = len-1; i >= 0; i--) {
      if (src[i] == c) {
        index = i;
        break;
      }
    }
  }

  return index;
}

int
first_index_of(const char* src, char c) {
  int index = -1;
  int len = 0;
  if ((len=strlen(src)) > 0) {
    for (int i = 0; i < len; i++) {
      if (src[i] == c) {
        index = i;
        break;
      }
    }
  }
  return index;
}

int first_index_of_substr(const char* src, const char* substr) {
  int src_len = strlen(src);
  int substr_len = strlen(substr);

  if (src_len < substr_len) {
    return -1;
  }

  for (int i = 0; i < src_len; i++) {
    for (int j = 0; j < substr_len; j++) {
      if (src[i+j] != substr[j]) {
        break;
      } else {
        if (j == substr_len - 1) {
          return i;
        }
      }
    }
  }
  return -1;
}



// the principle is put char into hex encoding
char* url_encode(const char *source_str, char *encode_str) {
  char hexchars[] = "0123456789ABCDEF";

  int source_str_len = strlen(source_str);

  int encode_index = 0;
  for (int i = 0; i < source_str_len; i++) {
    char c = source_str[i];
    if (c == ' ') {
      encode_str[encode_index++] = '+';
    } else if ((c < '0' && c != '-' && c != '.') ||
             (c < 'A' && c > '9') ||
             (c > 'Z' && c < 'a' && c != '_') ||
             (c > 'z')) {
      encode_str[encode_index++] = '%';
      encode_str[encode_index++] = hexchars[c >> 4];
      encode_str[encode_index++]= hexchars[c & 15];
    } else {
      encode_str[encode_index++] = c;
    }
  }

  encode_str[encode_index] = '\0';
  return encode_str;
}



std::vector<std::string> split(const std::string& str, const char* c) {
  char *cstr, *p;
  std::vector<std::string> res;
  cstr = new char[str.size()+1];
  snprintf(cstr, str.length(), "%s", str.c_str());
  p = strtok(cstr, c);  // NOLINT'
  while (p != NULL) {
    res.push_back(p);
    p = strtok(NULL, c);  // NOLINT'
  }
  delete[] cstr;
  return res;
}

void SplitString(const std::string& str,
                 const char* delim,
                 std::vector<std::string>* result) {
  char *cstr, *p;
  cstr = new char[str.size()+1];
  snprintf(cstr, str.length(), "%s", str.c_str());
  p = strtok(cstr, delim);  // NOLINT'
  while (p != NULL) {
    result->push_back(p);
    p = strtok(NULL, delim);  // NOLINT'
  }
  delete[] cstr;
}

}  // namespace base
}  // namespace sails

// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: buffer.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-10 15:34:04


#ifndef SAILS_BASE_BUFFER_H_
#define SAILS_BASE_BUFFER_H_

#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
// #include <endian.h>
#include <vector>
#include <algorithm>
#include "sails/base/uncopyable.h"


namespace sails {
namespace base {

class Buffer : public Uncopyable {
 public:
  Buffer():data(init_size), read_index(0), write_index(0) {
    assert(readable() == 0);
    assert(writeable() == init_size);
  }

 public:
  static const size_t init_size = 1024;

  // capacity
  size_t readable() { return write_index - read_index; }

  size_t writeable() { return data.size() - write_index; }

  void ensure_writeable(size_t len) {
    if (writeable() < len) {
      make_space(len);
    }
    assert(writeable() >= len);
  }

  void make_space(size_t len) {
    if (writeable() + read_index < len) {
      data.resize(write_index + len);
    } else {
      // move data to front
      size_t readablesize = readable();
      std::copy(begin()+read_index, begin()+write_index, begin());
      read_index = 0;
      write_index = read_index+readablesize;
      assert(readablesize == readable());
    }
  }

  void shrink(size_t reserve) {
    Buffer other;
    other.ensure_writeable(readable()+reserve);
    other.append(peek(), writeable());
    swap(other);
  }

  // element access

  char* begin() { return &*data.begin(); }
  const char* peek() { return begin() + read_index; }

  int32_t peek_int32() {
    assert(readable() >= sizeof(int32_t));
    int32_t be32 = 0;
    memcpy(&be32, peek(), sizeof(int32_t));
    return ntohl(be32);
  }

  void retrieve(size_t len) {
    if (len > readable()) {
      retrieve_all();
    } else {
      read_index+=len;
    }
  }

  void retrieve_all() { read_index = 0; write_index = 0; }

  int32_t read_int32() {
    int32_t result = peek_int32();
    retrieve(sizeof(int32_t));
    return result;
  }

  char *read_str(char* str, int max) {
    int read = read_string(str, max);
    retrieve(read);
    return str;
  }

  int read_string(char* str, int max) {
    int n = readable();
    int read = n > max?max:n;
    strncpy(str, begin(), read);
    return read;
  }


  // element modifiers
  void swap(Buffer &other) {
    data.swap(other.data);
    std::swap(read_index, other.read_index);
    std::swap(write_index, other.write_index);
  }

  void append(const char* data, size_t len) {
    ensure_writeable(len);
    std::copy(data, data+len, begin()+write_index);
    write_index+=len;
  }
  void append(const void* data, size_t len) {
    append(static_cast<const char*>(data), len);
  }
  void append_int32(int32_t x) {
    int32_t be32 = (int32_t)htonl(x);
    append(&be32, sizeof(int32_t));
  }

  ssize_t read(int fd, size_t len) {
    ensure_writeable(len);
    ssize_t readLen = ::read(fd, begin()+write_index, len);
    if (readLen > 0) {
      write_index+=readLen;
    }
    return readLen;
  }

 private:
  std::vector<char> data;
  size_t read_index;
  size_t write_index;
};


}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_BUFFER_H_

// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: handle.h
// Description: 处理器链
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:13:20



#ifndef SAILS_BASE_HANDLE_H_
#define SAILS_BASE_HANDLE_H_

#include <stdio.h>
#include <vector>

namespace sails {
namespace base {

template <typename T, typename U> class HandleChain;

template <typename T, typename U>
class Handle {
 public:
  virtual void do_handle(T t, U u, HandleChain<T, U> *chain) = 0;
  virtual ~Handle();
};

template <typename T, typename U>
Handle<T, U>::~Handle() {
}



template <typename T, typename U>
class HandleChain {
 public:
  HandleChain();

  ~HandleChain();

  void do_handle(T t, U u);

  void add_handle(Handle<T, U> *handle);
 private:
  std::vector<Handle<T, U>*> chain;
  int index;
};



/**
 * implements template in head file
 */


template <typename T, typename U>
HandleChain<T, U>::HandleChain() {
  this->index = 0;
}

template <typename T, typename U>
HandleChain<T, U>::~HandleChain() {
  /*
    while(this->chain.size() > 0) {
    Handle<T, U> *handle = this->chain.back();
    if(handle != NULL) {
    delete handle;
    }
    this->chain.pop_back();
    }
  */
}

template <typename T, typename U>
void HandleChain<T, U>::do_handle(T t, U u) {
  if (this->chain.size() > index) {
    Handle<T, U> *handle = this->chain.at(index++);
    if (handle != NULL) {
      handle->do_handle(t, u, this);
    }
  } else {
    // printf("end handle chain\n");
  }
}

template <typename T, typename U>
void HandleChain<T, U>::add_handle(Handle<T, U> *handle) {
  this->chain.push_back(handle);
}



}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_HANDLE_H_

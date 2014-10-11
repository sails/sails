// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: filter.h
// Description: 过滤器链
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-11 10:55:17



#ifndef SAILS_BASE_FILTER_H_
#define SAILS_BASE_FILTER_H_

#include <stdio.h>
#include <vector>

namespace sails {
namespace base {


template <typename T, typename U> class FilterChain;

template <typename T, typename U>
class Filter {
 public:
  virtual void do_filter(T t, U u, FilterChain<T, U> *chain) = 0;
};



template <typename T, typename U>
class FilterChain {
 public:
  FilterChain();

  void do_filter(T t, U u);

  void add_filter(Filter<T, U> *filter);
 private:
  std::vector<Filter<T, U>*> chain;
  int index;
};



/**
 * implements template in head file
 */


template <typename T, typename U>
FilterChain<T, U>::FilterChain() {
  this->index = 0;
}

template <typename T, typename U>
void FilterChain<T, U>::do_filter(T t, U u) {
  if (this->chain.size() > index) {
    Filter<T, U> *filter = this->chain.at(index++);
    if (filter != NULL) {
      filter->do_filter(t, u, this);
    }
  } else {
    printf("end filter chain\n");
  }
}

template <typename T, typename U>
void FilterChain<T, U>::add_filter(Filter<T, U> *filter) {
  this->chain.push_back(filter);
}



}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE _FILTER_H_

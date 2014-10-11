// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Filename: uncopyable.h
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-10 15:34:36





#ifndef SAILS_BASE_UNCOPYABLE_H_
#define SAILS_BASE_UNCOPYABLE_H_

namespace sails {
namespace base {

class Uncopyable {
 protected:
  Uncopyable() {}
  ~Uncopyable() {}
 private:
  Uncopyable(const Uncopyable&);
  const Uncopyable& operator=(const Uncopyable&);
};

}  // namespace base
}  // namespace sails

#endif  // SAILS_BASE_UNCOPYABLE_H_

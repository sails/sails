// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: singleton.h
// Description:单例模式
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-22 14:35:28

#ifndef SAILS_BASE_SINGLETON_H_
#define SAILS_BASE_SINGLETON_H_

#include <stdio.h>
#include <mutex>

namespace sails {
namespace base {

template<typename T>
class Singleton {
 private:
  Singleton();
  ~Singleton() {
    _pInstance = NULL;
  }

 public:
  static T* instance() {
    if (_pInstance == NULL) {
      std::unique_lock<std::mutex> locker (sMutex);
      if (_pInstance == NULL) {
        static T mInstance;
        _pInstance = &mInstance;
      }
    }
    return _pInstance;
  }
 private:
  static T* _pInstance;
  static std::mutex sMutex;
};



template<typename T>
T* Singleton<T>::_pInstance = NULL;

template<typename T>
T* Singleton<T>::sMutex;

}  // namespace base
}  // namespace sails


#endif  // SAILS_BASE_SINGLETON_H_


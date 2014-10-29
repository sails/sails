// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.org/.
//
// Filename: mime.h
// Description: mine-type(/etc/mime.types)
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-28 15:22:47


#ifndef SAILS_NET_MINE_H_
#define SAILS_NET_MINE_H_

#include <string>
#include <map>
#include <mutex>
#include "sails/base/uncopyable.h"


namespace sails {
namespace net {

class MimeType : public sails::base::Uncopyable {
 public:
  MimeType() {}
  explicit MimeType(const std::string& mime) {
    Set(mime);
  }
  MimeType(const std::string& type, const std::string& subtype) :
      _type(type), _subtype(subtype) {
  }

  ~MimeType() {}
  
  void Set(const std::string& type, const std::string& subtype) {
    _type = type;
    _subtype = subtype;
  }

  bool Set(const std::string& mime) {
    std::string::size_type pos = mime.find('/');
    if (pos != std::string::npos) {
      _type = mime.substr(0, pos);
      _subtype = mime.substr(pos + 1, mime.size() + 1);
      return true;
    }
    return false;
  }
  
  const std::string& Type() {
    return _type;
  }

  const std::string& SubType() {
    return _subtype;
  }
  
 private:
  std::string _type;
  std::string _subtype;
};

class MimeTypeManager : public sails::base::Uncopyable {
 private:
  MimeTypeManager() {}
  ~MimeTypeManager() {}

  // 通过文件/etc/mime.types初始化
  void init();
  
 public:
  static MimeTypeManager* instance() {
    if ( _instance == NULL) {
      std::unique_lock<std::mutex> locker(MimeTypeMutex);
      if (_instance == NULL) {
        static MimeTypeManager manager;
        manager.init();
        _instance = &manager;
      }
    }
    return _instance;
  }

  // 通过文件扩展名得到MimeType
  bool GetMimeTypebyFileExtension(const std::string& ext, MimeType* type);
  
 private:
  std::map<std::string, std::string> typemap;
  static MimeTypeManager* _instance;
  static std::mutex MimeTypeMutex;
};

}  // namespace net
}  // namespace sails

#endif  // SAILS_NET_MINE_H_

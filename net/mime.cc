// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.org/.
//
// Filename: mime.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-28 15:38:15

#include "sails/net/mime.h"
#include <iostream>
#include <fstream>
#include "sails/base/string.h"
#include "sails/log/logging.h"

namespace sails {
namespace net {

MimeTypeManager* MimeTypeManager::_instance = NULL;
std::mutex MimeTypeManager::MimeTypeMutex;

void MimeTypeManager::init() {
  typemap.insert(std::make_pair(".xml", "text/xml"));
  typemap.insert(std::make_pair(".html", "text/html"));
  typemap.insert(std::make_pair(".htm", "text/html"));
  typemap.insert(std::make_pair(".txt", "text/plain"));
  typemap.insert(std::make_pair(".js", "application/x-javascript"));
  typemap.insert(std::make_pair(".ico", "image/x-icon"));

  std::ifstream file("/etc/mime.types");
  if (!file) {
    log::LoggerFactory::getLog("server")->warn("error open mime.types!");
    return;
  }
  std::string line;
  std::vector<std::string> result;
  while (getline(file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    
    sails::base::SplitString(line, " \t", &result);
    for (std::vector<std::string>::iterator iter = result.begin() + 1;
         iter != result.end(); ++iter) {
      std::string ext = "."+*iter;
      std::string type = typemap[ext];
      if (type.empty()) {
        typemap[ext] = result[0];
      }
    }
    line.clear();
    result.clear();
  }
}


bool MimeTypeManager::GetMimeTypebyFileExtension(const std::string& ext,
                                                 MimeType* type) {
  auto iter = typemap.find(ext);
  if (iter != typemap.end()) {
    return type->Set(iter->second);
  }
  return false;
}

          
}  // namespace net          
}  // namespace sails

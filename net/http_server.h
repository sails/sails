// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.org/.
//
// Filename: http_server.h
// Description: http server extends epoll server
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-15 13:37:11

#ifndef SAILS_NET_HTTP_SERVER_H_
#define SAILS_NET_HTTP_SERVER_H_

#include "sails/net/http.h"
#include "sails/net/epoll_server.h"
#include "sails/net/http_parser.h"

namespace sails {
namespace net {


class HttpServer : public EpollServer<HttpRequest> {
 public:
  explicit HttpServer(uint32_t netThreadNum = 1);
  ~HttpServer();

 private:
  // HttpRequest删除器
  void Tdeleter(HttpRequest* data);

  // 创建连接后,为每个connector建立一个http parser,挂在connector上
  void CreateConnectorCB(std::shared_ptr<net::Connector> connector);

  // 调用http parser进行解析
  HttpRequest* Parse(std::shared_ptr<net::Connector> connector);

  // 删除http parser
  void ClosedConnectCB(std::shared_ptr<net::Connector> connector);

 private:
  struct http_parser_settings settings;
};

}  // namespace net          
}  // namespace sails


#endif  // SAILS_NET_HTTP_SERVER_H_














// Copyright (C) 2014 sails Authors.
// All rights reserved.
//
// Official git repository and contact information can be found at
// https://github.com/sails/sails and http://www.sailsxu.com/.
//
// Filename: http_server.cc
//
// Author: sailsxu <sailsxu@gmail.com>
// Created: 2014-10-15 16:22:41

#include "sails/net/http_server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <utility>
#include "sails/net/mime.h"

namespace sails {
namespace net {


HttpServer::HttpServer() {

}

HttpServer::~HttpServer() {
}

void HttpServer::Tdeleter(HttpRequest* data) {
  delete data;
}

void HttpServer::CreateConnectorCB(
    std::shared_ptr<net::Connector> connector) {
  // connector的data属性设置成parser,parser的data设置成解析的msg
  http_parser *parser =
      reinterpret_cast<http_parser*>(malloc(sizeof(http_parser)));
  connector->data.ptr = parser;
  http_parser_init(parser, HTTP_REQUEST);
  ParserFlag* parserFlag = new ParserFlag();
  parser->data = parserFlag;
  parserFlag->message =
      reinterpret_cast<http_message*>(malloc(sizeof(http_message)));
  http_message_init(parserFlag->message);
}

void HttpServer::ClosedConnectCB(
    std::shared_ptr<net::Connector> connector) {
  printf("closeconnect cb \n");
  if (connector->data.ptr != NULL) {
    http_parser *parser = reinterpret_cast<http_parser*>(connector->data.ptr);
    if (parser == NULL) {
      connector->data.ptr = NULL;
      return;
    }
    ParserFlag* parserFlag = reinterpret_cast<ParserFlag*>(parser->data);
    if (parserFlag != NULL && parserFlag->message != NULL) {
      free(parserFlag->message);
      if (parserFlag->messageList.size() > 0) {
        for (auto it = parserFlag->messageList.begin();
             it != parserFlag->messageList.end(); ++it) {
          http_message* msg = parserFlag->messageList.front();
          if (msg != NULL) {
            free(msg);
          }
          parserFlag->messageList.pop_front();
        }
      }
    }
    if (parserFlag != NULL) {
      delete parserFlag;
    }
    printf("free connector parser\n");
    free(parser);

    connector->data.ptr = NULL;
  }
}

HttpRequest* HttpServer::Parse(std::shared_ptr<net::Connector> connector) {
  HttpRequest* request = NULL;
  if (connector->data.ptr != NULL) {
    http_parser *parser = reinterpret_cast<http_parser*>(connector->data.ptr);
    ParserFlag* flag = reinterpret_cast<ParserFlag*>(parser->data);
    // 有数据解析,可能一次就已经解析出了多个数据包,
    // 所以即使connector中没有数据,依然会执行多次
    if (connector->readable() > 0) {
      char data[10000];
      memset(data, '\0', 10000);

      strncpy(data, connector->peek(), 10000);
      // 默认不是直接返回的,这儿通过把它这次解析过程中解析到
      // 的消息放到messagelist中
      http_parser *parser = reinterpret_cast<http_parser*>(connector->data.ptr);
      size_t nparsed = sails_http_parser(parser,
                                         data,
                                         flag);
      if (nparsed > 0) {
        connector->retrieve(nparsed);
      }
    }
    if (flag->messageList.size() > 0) {
      struct http_message* message = flag->messageList.front();
      flag->messageList.pop_front();
      request = new HttpRequest(message);
    }
  }
  return request;
}

int HttpServer::RegisterProcessor(
    std::string path, HttpProcessor processor) {
  if (processor == NULL) {
    return -1;
  }
  std::map<std::string, HttpProcessor>::iterator iter
      = processorMap.find(path);
  if (iter != processorMap.end()) {
    return 1;
  }
  processorMap.insert(
      std::pair<std::string, HttpProcessor>(path,
                                            processor));
  return 0;
}

HttpProcessor HttpServer::FindProcessor(std::string path) {
  std::map<std::string, HttpProcessor>::iterator iter
      = processorMap.find(path);
  if (iter == processorMap.end()) {
    return NULL;
  }
  return iter->second;
}


void HttpServer::handle(
    const sails::net::TagRecvData<sails::net::HttpRequest> &recvData) {

  /*
  static char data[10*1024] = {'\0'};
  recvData.data->ToString(data, 10*1024);
  log::LoggerFactory::getLogD("access")->info(
      "uid:%u, ip:%s, port:%d, msg:\n%s",
      recvData.uid, recvData.ip.c_str(), recvData.port, data);
  */
  log::LoggerFactory::getLogD("access")->info(
      "uid:%u, ip:%s, port:%d, msg:\n%s",
      recvData.uid, recvData.ip.c_str(), recvData.port,
      recvData.data->GetRequestPath().c_str());
  sails::net::HttpResponse *response = new sails::net::HttpResponse();
  sails::net::HttpRequest *request = recvData.data;
  process(request, response);

  char response_str[MAX_BODY_SIZE*2] = {'\0'};
  response->ToString(response_str, MAX_BODY_SIZE*2);

  std::string buffer(response_str, MAX_BODY_SIZE*2);

  this->send(buffer, recvData.ip, recvData.port, recvData.uid, recvData.fd);
  // 因为最后都被放到一个队列中处理,所以肯定会send之后,再close
  this->CloseConnector(recvData.ip, recvData.port, recvData.uid, recvData.fd);

  delete response;
}

void HttpServer::process(sails::net::HttpRequest* request,
             sails::net::HttpResponse* response) {
  std::string path = request->GetRequestPath();
  // 通过后缀名预设content-type
  int extensionIndex = path.find_last_of(".", path.size());
  if (extensionIndex > 0) {
    std::string fileExtension = path.substr(extensionIndex, 7);
    if (fileExtension.size() > 0) {
      MimeType mimetype;
      MimeTypeManager::instance()->GetMimeTypebyFileExtension(
          fileExtension, &mimetype);
      if (mimetype.Type().size() > 0) {
        response->SetHeader("Content-Type", mimetype.ToString().c_str());
      }
    }
  }
  HttpProcessor processor = FindProcessor(path);
  if (processor == NULL) {
    // 检查是不是在请求本地资源文件,存在则直接下载
    // 这儿简单的一次性读取所有内容,所以资源文件不能太大
    std::string filename;
    if (path.size() == 1) {
      if (path == "/") {
        filename = "index.html";
      }
    } else if (path.size() == 0) {
      filename = "index.html";
    } else {
      filename = path.substr(1, 100);
    }
    std::string filepath = this->StaticResourcePath()
                           + filename;
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd < 0) {
      log::LoggerFactory::getLog("server")->debug(
          "error open %s", filepath.c_str());
      response->SetResponseStatus(HttpResponse::Status_NotFound);
      response->SetBody("404", 3);
      return;
    } else {
      // 得到文件大小
      uint32_t filesize = -1;
      struct stat statbuff;
      if (stat(filepath.c_str(), &statbuff) < 0) {
        response->SetResponseStatus(HttpResponse::Status_NotFound);
        response->SetBody("404", 3);
        return;
      } else {
        filesize = statbuff.st_size;
      }
      if (filesize > MAX_BODY_SIZE) {
        char errormsg[100] = {"The File Too Large"};
        response->SetBody(errormsg, strlen(errormsg));
        return;
      }
      char* data = reinterpret_cast<char*>(malloc(filesize+1));
      memset(data, 0, filesize+1);
      int readLen = read(fd, data, filesize+1);
      response->SetBody(data, readLen);
      free(data);
    }
  } else {
    processor(request, response);
  }
}

}  // namespace net
}  // namespace sails







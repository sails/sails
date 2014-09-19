#ifndef SERVER_H
#define SERVER_H


#include <common/net/epoll_server.h>
#include <common/net/connector.h>
#include <common/net/packets.h>
#include "config.h"
#include "module_load.h"

namespace sails {


class Server : public sails::common::net::EpollServer<common::net::PacketCommon> {
public:
    Server(int netThreadNum);

    ~Server();

    common::net::PacketCommon* parse(std::shared_ptr<sails::common::net::Connector> connector);

private:
    Config config;
// rpc 模块,不同的项目放同一个模块中
    std::map<std::string, std::string> modules_name;
    ModuleLoad moduleLoad;
};



class HandleImpl : public sails::common::net::HandleThread<sails::common::net::PacketCommon> {
public:
    HandleImpl(sails::common::net::EpollServer<sails::common::net::PacketCommon>* server);
    
    void handle(const sails::common::net::TagRecvData<common::net::PacketCommon> &recvData);
};



} // namespace sails


#endif /* SERVER_H */




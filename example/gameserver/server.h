#ifndef SERVER_H
#define SERVER_H


#include <map>
#include <mutex>
#include <common/net/epoll_server.h>
#include <common/net/connector.h>
#include <common/net/packets.h>
#include "game_world.h"
#include "game_packets.h"
#include "config.h"

namespace sails {


class Server : public sails::common::net::EpollServer<SceNetAdhocctlPacketBase> {
public:
    Server(int netThreadNum);

    ~Server();

    void create_connector_cb(std::shared_ptr<common::net::Connector> connector);
    void Tdeleter(SceNetAdhocctlPacketBase *data);

    // 获取游戏
    GameWorld* getGameWorld(std::string& gameCode);
    
    // 创建游戏
    GameWorld* createGameWorld(std::string& gameCode);

    // 创建用户
    uint32_t createPlayer(std::string ip, int port, int fd, uint32_t connectUid);
    // 数据解析
    void parseImp(std::shared_ptr<common::net::Connector> connector);
    SceNetAdhocctlPacketBase* parse(std::shared_ptr<sails::common::net::Connector> connector);

    void sendDisConnectDataToHandle(uint32_t playerId, std::string ip, int port, int fd,  uint32_t uid);
    
    // 非法数据处理(直接移除用户,关闭连接),关于player的数据操作放到handle线程里,防止多线程操作player的问题.创建一个disconnector的数据包
    void invalid_msg_handle(std::shared_ptr<sails::common::net::Connector> connector);

    // 客户端主动close, 创建一个disconnector的数据包
    void closed_connect_cb(std::shared_ptr<common::net::Connector> connector);

    // 当连接超时时,创建一个disconnector数据包
    void connector_timeout_cb(common::net::Connector* connector);
    
    // 移除用户
    void deletePlayer(uint32_t playerId);

    Player* getPlayer(uint32_t playerId);

    // 操作player属性值
    int getPlayerState(uint32_t playerId);
    void setPlayerState(int32_t playerId, int state);
    
    std::list<std::string> getPlayerSession();

    std::list<std::string> getGameWorldList();

    // 得到游戏组里用户
    std::map<std::string, std::list<std::string>> getPlayerNameMap(std::string& gameCode);

private:
    std::mutex* getPlayerMutex(uint32_t playerId);
private:
    std::map<std::string, GameWorld*> gameWorldMap;
    std::mutex gameworldMutex;
    common::ConstantPtrList<Player> playerList;
};












class HandleImpl : public sails::common::net::HandleThread<SceNetAdhocctlPacketBase> {
public:
    HandleImpl(sails::common::net::EpollServer<SceNetAdhocctlPacketBase>* server);
    
    void handle(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);

    // 登录,对用户,游戏进行校验
    void login_user_data(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);

    // 把用户加入游戏/组里
    void connect_user(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);
    // 从组里把用户移除
    void disconnect_user(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);
        
    // 获取游戏的组列表
    void send_scan_results(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);
    
    // 向用户发送聊天数据(如果用户没有在房间里,则向所有用户发送,否则向房间内的用户发送)
    void spread_message(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);
    
    // 向用户发送游戏数据
    void transfer_message(const sails::common::net::TagRecvData<SceNetAdhocctlPacketBase> &recvData);


    // 用户session 校验,如果不成功,则向handle线程发送退出命令
    void player_session_check(uint32_t playerId, std::string ip, int port, int fd, uint32_t uid, std::string session);
    

    

    // mac地址转化
    static std::string getMacStr(SceNetEtherAddr& macAddr);
    static SceNetEtherAddr getMacStruct(std::string macstr);

    // ip 转化
    static std::string getIpstr(uint32_t ip);
    static uint32_t getIp(std::string ip);

    // group name转化
    static SceNetAdhocctlGroupName getRoomName(std::string& name);
private:
    // 删除用户
    void logout_user(uint32_t playerId);

    std::string game_product_override(SceNetAdhocctlProductCode * product);
};


extern Config config;

// session 校验与更新相关

struct ptr_string {
  char *ptr;
  size_t len;
};

size_t read_callback(void *buffer, size_t size, size_t nmemb, void *userp);

bool post_message(const char* url, const char* data, std::string &result);

bool check_session(std::string session);

bool update_session_timeout(std::string session);


} // namespace sails


#endif /* SERVER_H */

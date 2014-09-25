#ifndef PLAYER_H
#define PLAYER_H

#include <string>

namespace sails {


class Player {
public:
    Player();

    ~Player();

    static void destroy(Player* player);
    
    
public:
    
    // 用户状态
    int userState;

    uint32_t playerId;
    std::string playerName;
    std::string roomCode; // psp游戏里没有用id,而是直接用code
    std::string gameCode;

    std::string ip;
    int port;
    std::string mac;
    int fd;
    uint32_t connectorUid;

    int age; // 存活年龄,当有用户进入和退出时,age加1

    std::string session;
};


} // namespace sails


#endif /* PLAYER_H */

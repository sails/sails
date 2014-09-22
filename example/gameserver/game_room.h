#ifndef GAME_ROOM_H
#define GAME_ROOM_H


#include <string>
#include <map>
#include <player.h>
#include <mutex>

namespace sails {

class GameWorld;


class GameRoom {
public:
    GameRoom(std::string roomCode, int seatNum, GameWorld *gameWorld);

    bool connectPlayer(uint32_t playerId);

    void disConnectPlayer(uint32_t playerId);

    std::string getRoomCode();

    std::string getRoomHostMac();

    void spreadMessage(std::string& message);

    void transferMessage(std::string&ip, std::string& mac, std::string& message);
private:
    std::string roomCode;
    int seatNum;
    int usedNum;
    std::map<uint32_t, Player*> playerMap;
    std::mutex playerMutex;

    GameWorld *gameWorld;
};


} // namespace sails

#endif /* GAME_ROOM_H */















#include <stdint.h>
#include "player.h"


namespace sails {

Player::Player() {
    playerId = 0;
    age = 0;
}

Player::~Player() {
    
}


void Player::destroy(Player* player) {
    delete player;
}



} // namespace sails











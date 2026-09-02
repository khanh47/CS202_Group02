#include "Game/Behaviours/FireballAttackStrategy.h"
#include "Game/Behaviours/Animatable.h"
#include "Audio/SoundManager.h"
#include "Game/Objects/Player/Player.h"
#include "Game/World/GameWorld.h"
#include <string>

void FireballAttackStrategy::executeAttack(Player& player, GameWorld& world) {
    sf::Vector2f playerPos = player.getPosition();
    const bool facingRight = !player.isFacingLeft();
    const int playerIndex = player.getCharacter() == "mario" ? 0 : 1;
    if(!world.spawnFireball(playerPos, facingRight, playerIndex))
        return;

    Audio::SoundManager::getInstance().playEffect("fireball");

    auto* animatable = player.getBehaviour<Animatable>();
    auto* moveable = player.getBehaviour<Moveable>();    
    if(!animatable || !moveable) return;
    if(moveable->isAirbone()) animatable->playAnimation("air_shot", true);
    else animatable->playAnimation("shoot", true);
}

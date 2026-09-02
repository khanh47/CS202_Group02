#include "Game/Objects/Player/State/SuperState.h"
#include "Audio/SoundManager.h"
#include "Game/Objects/Player/Player.h"
#include "Game/Behaviours/Invincible.h"

#include <utility>

SuperState::SuperState(std::string character)
    : _character(std::move(character)) {}

void SuperState::handleSuperMushroom(Player& player) {
    (void)player;
    Audio::SoundManager::getInstance().playEffect("power_up");
}

void SuperState::handleFireFlower(Player& player) {
    player.startTransformation(Player::TransformTarget::Fire);
}

void SuperState::handleSuperStar(Player& player) {
    player.startTransformation(Player::TransformTarget::StarMan);
}

void SuperState::handleEnemy(Player& player) {
    player.startTransformation(Player::TransformTarget::Normal);
    player.addBehaviour<Invincible>(2.0f);
}

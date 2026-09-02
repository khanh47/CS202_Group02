#include "Game/Objects/Player/State/FireState.h"
#include "Audio/SoundManager.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Behaviours/Invincible.h"
#include "Game/Objects/Player/Player.h"

#include <utility>

FireState::FireState(std::string character)
    : _character(std::move(character)) {}

void FireState::handleSuperMushroom(Player& player) {
    (void)player;
    Audio::SoundManager::getInstance().playEffect("power_up");
}

void FireState::handleFireFlower(Player& player) {
    (void)player;
    Audio::SoundManager::getInstance().playEffect("power_up");
}

void FireState::handleSuperStar(Player& player) {
    player.startTransformation(Player::TransformTarget::StarMan);
}

void FireState::handleEnemy(Player& player) {
    player.startTransformation(Player::TransformTarget::Normal);
    player.addBehaviour<Invincible>(2.0f);
}

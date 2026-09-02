#include "Game/Objects/Player/State/StarManStateDecorator.h"
#include "Audio/SoundManager.h"
#include "Game/Objects/Player/State/SuperState.h"
#include "Game/Objects/Player/State/FireState.h"
#include "Game/Objects/Player/Player.h"

#include <string>

StarManStateDecorator::StarManStateDecorator(std::unique_ptr<PlayerState> wrappedState, float durationSeconds)
    : PlayerStateDecorator(std::move(wrappedState)),
      _remainingTime(durationSeconds) {
}

std::string StarManStateDecorator::getStateName() const {
    return "StarMan (" + PlayerStateDecorator::getStateName() + ")";
}

float StarManStateDecorator::getMoveSpeedMultiplier() const {
    return PlayerStateDecorator::getMoveSpeedMultiplier() * 1.5f;
}

float StarManStateDecorator::getJumpSpeedMultiplier() const {
    return PlayerStateDecorator::getJumpSpeedMultiplier() * 1.2f;
}

void StarManStateDecorator::handleSuperMushroom(Player& player) {
    if (_wrappedState) {
        //OOP principal violate but it works so just leave it there lol
        if (_wrappedState->getStateName() == "Normal") {
            _wrappedState = std::make_unique<SuperState>(player.getCharacter());
            player.startTransformation(Player::TransformTarget::None);
            Audio::SoundManager::getInstance().playEffect("power_up");
            return;
        }
    }

    Audio::SoundManager::getInstance().playEffect("power_up");
}

void StarManStateDecorator::handleFireFlower(Player& player) {
    if (_wrappedState) {
        //OOP principal violate but it works so just leave it there lol
        if (_wrappedState->getStateName() != "Fire") {
            _wrappedState = std::make_unique<FireState>(player.getCharacter());
            player.startTransformation(Player::TransformTarget::None);
            Audio::SoundManager::getInstance().playEffect("power_up");
            return;
        }
    }

    Audio::SoundManager::getInstance().playEffect("power_up");
}

void StarManStateDecorator::handleSuperStar(Player& player) {
    (void)player;
    resetTimer(10.0f);
}

void StarManStateDecorator::update(Player& player, float dt) {
    PlayerStateDecorator::update(player, dt);
    _remainingTime -= dt;
}

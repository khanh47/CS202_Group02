#include "Game/Objects/Player/State/MegaStateDecorator.h"
#include "Audio/SoundManager.h"

MegaStateDecorator::MegaStateDecorator(
    std::unique_ptr<PlayerState> wrappedState,
    float durationSeconds,
    std::unique_ptr<PlayerState> stateToRestore
)
    : PlayerStateDecorator(std::move(wrappedState)),
      _remainingTime(durationSeconds),
      _stateToRestore(std::move(stateToRestore)) {
}

std::string MegaStateDecorator::getStateName() const {
    return "Mega (" + PlayerStateDecorator::getStateName() + ")";
}

float MegaStateDecorator::getMoveSpeedMultiplier() const {
    return PlayerStateDecorator::getMoveSpeedMultiplier() * 1.5f;
}

void MegaStateDecorator::handleSuperMushroom(Player&) {
    Audio::SoundManager::getInstance().playEffect("power_up");
}

void MegaStateDecorator::handleFireFlower(Player&) {
    Audio::SoundManager::getInstance().playEffect("power_up");
}

void MegaStateDecorator::handleSuperStar(Player&) {
    Audio::SoundManager::getInstance().playEffect("power_up");
}

float MegaStateDecorator::getJumpSpeedMultiplier() const {
    return PlayerStateDecorator::getJumpSpeedMultiplier() * 1.2f;
}

sf::Vector2f MegaStateDecorator::getScaleMultiplier() const {
    // Four times the normal player hitbox makes the transformation visibly
    // colossal while keeping the sprite anchored to the player's feet.
    return {scaleMultiplier, scaleMultiplier};
}

std::unique_ptr<PlayerState> MegaStateDecorator::takeStateAfterMega() {
    if (_stateToRestore) {
        return std::move(_stateToRestore);
    }
    return unwrap();
}

void MegaStateDecorator::update(Player& player, float dt) {
    PlayerStateDecorator::update(player, dt);
    _remainingTime -= dt;
}

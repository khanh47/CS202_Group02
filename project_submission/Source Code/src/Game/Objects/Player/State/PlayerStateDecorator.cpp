#include "Game/Objects/Player/State/PlayerStateDecorator.h"
#include <utility>

PlayerStateDecorator::PlayerStateDecorator(std::unique_ptr<PlayerState> wrappedState)
    : _wrappedState(std::move(wrappedState)) {
}

std::string PlayerStateDecorator::getStateName() const {
    return _wrappedState ? _wrappedState->getStateName() : "Unknown";
}

std::string PlayerStateDecorator::getAnimationSetId() const {
    return _wrappedState ? _wrappedState->getAnimationSetId() : "mario";
}

std::string PlayerStateDecorator::getTextureAlias() const {
    return _wrappedState ? _wrappedState->getTextureAlias() : "mario_spritesheet";
}

float PlayerStateDecorator::getMoveSpeedMultiplier() const {
    return _wrappedState ? _wrappedState->getMoveSpeedMultiplier() : 1.0f;
}

float PlayerStateDecorator::getJumpSpeedMultiplier() const {
    return _wrappedState ? _wrappedState->getJumpSpeedMultiplier() : 1.0f;
}

sf::Vector2f PlayerStateDecorator::getScaleMultiplier() const {
    return _wrappedState ? _wrappedState->getScaleMultiplier() : sf::Vector2f{1.0f, 1.0f};
}

bool PlayerStateDecorator::canShootFireballs() const {
    return _wrappedState ? _wrappedState->canShootFireballs() : false;
}

bool PlayerStateDecorator::isInvincible() const {
    return _wrappedState ? _wrappedState->isInvincible() : false;
}

std::unique_ptr<IAttackStrategy> PlayerStateDecorator::createAttackStrategy() const {
    return _wrappedState ? _wrappedState->createAttackStrategy() : std::make_unique<NoAttackStrategy>();
}

void PlayerStateDecorator::handleSuperMushroom(Player& player) {
    if (_wrappedState) _wrappedState->handleSuperMushroom(player);
}

void PlayerStateDecorator::handleFireFlower(Player& player) {
    if (_wrappedState) _wrappedState->handleFireFlower(player);
}

void PlayerStateDecorator::handleSuperStar(Player& player) {
    if (_wrappedState) _wrappedState->handleSuperStar(player);
}

void PlayerStateDecorator::update(Player& player, float dt) {
    if (_wrappedState) {
        _wrappedState->update(player, dt);
    }
}

void PlayerStateDecorator::onEnter(Player& player) {
    if (_wrappedState) {
        _wrappedState->onEnter(player);
    }
}

void PlayerStateDecorator::onExit(Player& player) {
    if (_wrappedState) {
        _wrappedState->onExit(player);
    }
}

std::unique_ptr<PlayerState> PlayerStateDecorator::unwrap() {
    return std::move(_wrappedState);
}

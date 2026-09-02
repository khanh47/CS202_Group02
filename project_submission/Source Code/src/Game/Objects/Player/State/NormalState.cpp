#include "Game/Objects/Player/State/NormalState.h"
#include "Game/Objects/Player/Player.h"

#include <utility>

NormalState::NormalState(std::string character)
    : _character(std::move(character)) {}

void NormalState::handleSuperMushroom(Player& player) {
    player.startTransformation(Player::TransformTarget::Super);
}

void NormalState::handleFireFlower(Player& player) {
    player.startTransformation(Player::TransformTarget::Fire);
}

void NormalState::handleSuperStar(Player& player) {
    player.startTransformation(Player::TransformTarget::StarMan);
}

void NormalState::handleEnemy(Player& player) {
    player.destroy();
}

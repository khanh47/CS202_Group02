#pragma once

#include <string>

#include "Game/Objects/Player/State/PlayerState.h"

/**
 * @brief Concrete state representing standard small player (Mario or Luigi).
 */
class NormalState : public PlayerState {
public:
    explicit NormalState(std::string character = "mario");
    ~NormalState() override = default;

    std::string getStateName() const override { return "Normal"; }
    std::string getAnimationSetId() const override { return _character; }
    std::string getTextureAlias() const override { return _character + "_spritesheet"; }

    float getMoveSpeedMultiplier() const override { return 1.0f; }
    float getJumpSpeedMultiplier() const override { return 1.0f; }
    sf::Vector2f getScaleMultiplier() const override { return {1.0f, 1.0f}; }
    bool canShootFireballs() const override { return false; }

    void handleSuperMushroom(Player& player) override;
    void handleFireFlower(Player& player) override;
    void handleSuperStar(Player& player) override;
    void handleEnemy(Player& player) override;

private:
    std::string _character;
};

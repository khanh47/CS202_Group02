#pragma once

#include <string>

#include "Game/Objects/Player/Player.h"
#include "Game/Objects/Player/State/PlayerState.h"

/**
 * @brief Concrete state representing Super player (enlarged size and boosted jump).
 */
class SuperState : public PlayerState {
public:
    explicit SuperState(std::string character = "mario");
    ~SuperState() override = default;

    std::string getStateName() const override { return "Super"; }
    std::string getAnimationSetId() const override { return _character; }
    std::string getTextureAlias() const override { return _character + "_spritesheet"; }

    float getMoveSpeedMultiplier() const override { return 1.1f; }
    float getJumpSpeedMultiplier() const override { return 1.15f; }
    sf::Vector2f getScaleMultiplier() const override { return {1.5f, 1.5f}; }
    bool canShootFireballs() const override { return false; }

    void handleSuperMushroom(Player& player) override;
    void handleFireFlower(Player& player) override;
    void handleSuperStar(Player& player) override;
    void handleEnemy(Player& player) override;

private:
    std::string _character;
};

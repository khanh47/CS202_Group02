#pragma once

#include "Game/Objects/Player/State/PlayerStateDecorator.h"

/**
 * @brief Decorator granting temporary Star Man invincibility.
 * Boosts speed, makes the player invincible (enemies are destroyed on contact),
 * and switches the visual to a rainbow spritesheet. Reverts automatically after
 * the specified duration expires.
 */
class StarManStateDecorator : public PlayerStateDecorator {
public:
    StarManStateDecorator(std::unique_ptr<PlayerState> wrappedState, float durationSeconds = 10.0f);
    ~StarManStateDecorator() override = default;

    std::string getStateName() const override;
    float getMoveSpeedMultiplier() const override;
    float getJumpSpeedMultiplier() const override;
    bool isInvincible() const override { return true; }
    bool isExpired() const override { return _remainingTime <= 0.0f; }
    float getRemainingTime() const noexcept { return _remainingTime; }

    void handleSuperMushroom(Player& player) override;
    void handleFireFlower(Player& player) override;
    void handleSuperStar(Player& player) override;

    void resetTimer(float durationSeconds = 10.0f) { _remainingTime = durationSeconds; }

    void update(Player& player, float dt) override;

private:
    float _remainingTime;
};

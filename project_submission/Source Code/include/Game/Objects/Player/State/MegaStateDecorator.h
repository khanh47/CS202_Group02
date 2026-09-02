#pragma once

#include "Game/Objects/Player/State/PlayerStateDecorator.h"

/**
 * @brief Decorator providing temporary Mega Mushroom status (giant size, speed boost)
 * for a limited duration, automatically reverting to the base state upon expiration.
 */
class MegaStateDecorator : public PlayerStateDecorator {
public:
    static constexpr float scaleMultiplier = 8.0f;

    MegaStateDecorator(
        std::unique_ptr<PlayerState> wrappedState,
        float durationSeconds = 16.0f,
        std::unique_ptr<PlayerState> stateToRestore = nullptr
    );
    ~MegaStateDecorator() override = default;

    std::string getStateName() const override;
    float getMoveSpeedMultiplier() const override;
    float getJumpSpeedMultiplier() const override;
    sf::Vector2f getScaleMultiplier() const override;
    bool isInvincible() const override { return true; }
    bool canShootFireballs() const override { return false; }
    std::unique_ptr<IAttackStrategy> createAttackStrategy() const override {
        return std::make_unique<NoAttackStrategy>();
    }
    bool isExpired() const override { return _remainingTime <= 0.0f; }
    float getRemainingTime() const noexcept { return _remainingTime; }
    const PlayerState* getStateToRestore() const noexcept {
        return _stateToRestore.get();
    }
    void resetTimer(float durationSeconds = 16.0f) noexcept {
        _remainingTime = durationSeconds;
    }

    void handleSuperMushroom(Player&) override;
    void handleFireFlower(Player&) override;
    void handleSuperStar(Player&) override;
    std::unique_ptr<PlayerState> takeStateAfterMega();
    void update(Player& player, float dt) override;

private:
    float _remainingTime;
    std::unique_ptr<PlayerState> _stateToRestore;
};

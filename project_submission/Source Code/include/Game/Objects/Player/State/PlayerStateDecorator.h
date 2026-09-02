#pragma once

#include "Game/Objects/Player/State/PlayerState.h"
#include <memory>

/**
 * @brief Base decorator class for temporary or additive player states using the Decorator Pattern.
 * Delegates base behaviors to an inner wrapped PlayerState instance.
 */
class PlayerStateDecorator : public PlayerState {
public:
    explicit PlayerStateDecorator(std::unique_ptr<PlayerState> wrappedState);
    ~PlayerStateDecorator() override = default;

    std::string getStateName() const override;
    std::string getAnimationSetId() const override;
    std::string getTextureAlias() const override;

    float getMoveSpeedMultiplier() const override;
    float getJumpSpeedMultiplier() const override;
    sf::Vector2f getScaleMultiplier() const override;
    bool canShootFireballs() const override;
    bool isInvincible() const override;
    std::unique_ptr<IAttackStrategy> createAttackStrategy() const override;

    void handleSuperMushroom(Player& player) override;
    void handleFireFlower(Player& player) override;
    void handleSuperStar(Player& player) override;

    void update(Player& player, float dt) override;
    void onEnter(Player& player) override;
    void onExit(Player& player) override;

    PlayerState* getWrappedState() const { return _wrappedState.get(); }
    std::unique_ptr<PlayerState> unwrap();

protected:
    std::unique_ptr<PlayerState> _wrappedState;
};

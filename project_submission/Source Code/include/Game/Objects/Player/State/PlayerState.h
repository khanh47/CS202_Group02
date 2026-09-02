#pragma once

#include "Game/Behaviours/AttackStrategy.h"
#include "Game/Behaviours/NoAttackStrategy.h"
#include <memory>
#include <string>
#include <SFML/System/Vector2.hpp>

class Player;

/**
 * @brief Abstract base class representing a Mario state using the State Pattern.
 * Encapsulates state-specific properties such as animation identifiers, speed/jump modifiers,
 * scale multipliers, attack strategy creation, and special actions (e.g. shooting fireballs).
 */
class PlayerState {
public:
    virtual ~PlayerState() = default;

    virtual std::string getStateName() const = 0;
    virtual std::string getAnimationSetId() const = 0;
    virtual std::string getTextureAlias() const = 0;

    virtual float getMoveSpeedMultiplier() const { return 1.0f; }
    virtual float getJumpSpeedMultiplier() const { return 1.0f; }
    virtual sf::Vector2f getScaleMultiplier() const { return {1.0f, 1.0f}; }
    virtual bool canShootFireballs() const { return false; }
    virtual bool isInvincible() const { return false; }
    virtual std::unique_ptr<IAttackStrategy> createAttackStrategy() const {
        return std::make_unique<NoAttackStrategy>();
    }
    virtual bool isExpired() const { return false; }

    virtual void update(Player& player, float dt) { (void)player; (void)dt; }
    virtual void onEnter(Player& player) { (void)player; }
    virtual void onExit(Player& player) { (void)player; }

    virtual void handleSuperMushroom(Player& player) { (void)player; }
    virtual void handleFireFlower(Player& player) { (void)player; }
    virtual void handleSuperStar(Player& player) { (void)player; }
    virtual void handleEnemy(Player& player) { (void)player; }
};

#pragma once

#include "Game/Behaviours/AttackStrategy.h"

/**
 * @brief Default strategy representing no attack capability (used for Normal and Super Mario).
 */
class NoAttackStrategy : public IAttackStrategy {
public:
    NoAttackStrategy() = default;
    ~NoAttackStrategy() override = default;

    void executeAttack(Player& player, GameWorld& world) override {
        // No attack performed for standard player states
        (void)player;
        (void)world;
    }
};

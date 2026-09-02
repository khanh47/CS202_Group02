#pragma once

#include "Game/Behaviours/AttackStrategy.h"

/**
 * @brief Concrete attack strategy for Fire Mario that spawns bouncing fireballs via the Object Pool.
 */
class FireballAttackStrategy : public IAttackStrategy {
public:
    FireballAttackStrategy() = default;
    ~FireballAttackStrategy() override = default;

    void executeAttack(Player& player, GameWorld& world) override;
};

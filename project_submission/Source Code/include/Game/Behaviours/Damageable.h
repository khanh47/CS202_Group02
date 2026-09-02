#pragma once

#include "Game/Behaviours/Behaviour.h"

class Damageable : public Behaviour {
public:
    Damageable(int initHealth = 100);
    virtual ~Damageable() = default;

    void takeDamage(int amount);
    void heal(int amount);

    bool isAlive() const { return _health > 0; }
    int getCurrentHealth() const { return _health; }
    int getMaxHealth() const { return _maxHealth; }
    void setCurrentHealth(int health) {
        _health = std::clamp(health, 0, _maxHealth);
    }

protected:
    int _maxHealth = 100;
    int _health = 100;
};

#include "Game/Behaviours/Damageable.h"

Damageable::Damageable(int initHealth)
    : _health(initHealth), _maxHealth(initHealth) {
}

void Damageable::takeDamage(int amount) {
    _health -= amount;

    if(_health < 0) {
        _health = 0;
    }
}

void Damageable::heal(int amount) {
    _health += amount;

    if(_health > _maxHealth) {
        _health = _maxHealth;
    }
}
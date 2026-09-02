#pragma once

#include <cstdint>

namespace CollisionFilter {
    // Category bits
    inline constexpr uint64_t ENV = 0x0001;
    inline constexpr uint64_t PLAYER = 0x0002;
    inline constexpr uint64_t FIREBALL = 0x0004;
    inline constexpr uint64_t ENEMY = 0x0008;
    inline constexpr uint64_t PICKUP = 0x0010;
    inline constexpr uint64_t SHELL = 0x0020;

    // Common masks
    inline constexpr uint64_t PLAYER_MASK = ENV | ENEMY | PICKUP | SHELL;
    inline constexpr uint64_t MINIGAME_MASK = PLAYER | FIREBALL;
    inline constexpr uint64_t ENEMY_MASK = ENV | PLAYER | FIREBALL | SHELL;
    inline constexpr uint64_t FIREBALL_MASK = ENV | ENEMY | SHELL;
    inline constexpr uint64_t PICKUP_MASK = PLAYER;
    inline constexpr uint64_t SHELL_MASK = ENV | ENEMY | PLAYER | FIREBALL;
}

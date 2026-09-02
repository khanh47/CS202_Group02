#pragma once

#include <vector>
#include <memory>
#include <SFML/Graphics.hpp>

#include "Game/Objects/Projectile/Fireball.h"

class PhysicsWorld;
class GameWorld; // Forward declaration

/**
 * @brief Implements the Object Pool pattern for Fireball objects.
 * Pre-allocates a fixed pool of 5 Fireballs and enforces a strict limit of 
 * maximum 2 active fireballs on screen to optimize runtime performance.
 */
class FireballPool {
public:
    FireballPool() = default;
    ~FireballPool() = default;

    void initialize(const PhysicsWorld& physicsWorld, sf::Texture& texture);
    bool spawnFireball(sf::Vector2f spawnPos, bool facingRight, int ownerIndex = -1);
    
    void updateSimulation(const float& fixedDt, float maxDistancePixels, float voidYThreshold);
    void updateVisuals(float deltaTime);
    void render(sf::RenderTarget& target);
    void reset();

    int getActiveCount() const;
    const std::vector<std::shared_ptr<Fireball>>& getPool() const { return _pool; }

    void setGameWorld(GameWorld* world);

private:
    static constexpr size_t POOL_CAPACITY = 8;
    static constexpr int MAX_ACTIVE_FIREBALLS = 2;
    std::vector<std::shared_ptr<Fireball>> _pool;
};

#include "Game/Objects/Projectile/FireballPool.h"
#include "Physics/PhysicsWorld.h"

void FireballPool::initialize(const PhysicsWorld& physicsWorld, sf::Texture& texture) {
    _pool.clear();
    _pool.reserve(POOL_CAPACITY);

    const sf::Vector2f fireballHitbox{38.0f, 38.0f};

    for (size_t i = 0; i < POOL_CAPACITY; ++i) {
        auto fireball = std::make_shared<Fireball>(texture);
        fireball->spawn(physicsWorld, sf::Vector2f(0.0f, 0.0f), fireballHitbox);
        fireball->deactivate();
        _pool.push_back(std::move(fireball));
    }
}

bool FireballPool::spawnFireball(sf::Vector2f spawnPos, bool facingRight, int ownerIndex) {
    if (getActiveCount() >= MAX_ACTIVE_FIREBALLS) {
        return false;
    }

    for (auto& fireball : _pool) {
        if (fireball && !fireball->isActive()) {
            // Offset spawn position slightly in front of Mario to prevent self-collision
            const float offsetX = facingRight ? 45.0f : -45.0f;
            sf::Vector2f adjustedSpawn = {spawnPos.x + offsetX, spawnPos.y};
            fireball->activate(adjustedSpawn, facingRight, ownerIndex);
            return true;
        }
    }

    return false;
}

void FireballPool::updateSimulation(const float& fixedDt, float maxDistancePixels, float voidYThreshold) {
    for (auto& fireball : _pool) {
        if (fireball && fireball->isActive()) {
            fireball->updateSimulation(fixedDt);

            // Deactivate fireball if it has traveled 3/4 screen width or fallen into the void
            if (fireball->getDistanceTraveled() >= maxDistancePixels ||
                fireball->getPosition().y >= voidYThreshold) {
                fireball->deactivate();
            }
        }
    }
}

void FireballPool::updateVisuals(float deltaTime) {
    for (auto& fireball : _pool) {
        if (fireball && fireball->isActive()) {
            fireball->updateVisuals(deltaTime);
        }
    }
}

void FireballPool::render(sf::RenderTarget& target) {
    for (auto& fireball : _pool) {
        if (fireball && fireball->isActive()) {
            fireball->render(target);
        }
    }
}

void FireballPool::reset() {
    for (auto& fireball : _pool) {
        if (fireball) {
            fireball->deactivate();
        }
    }
}

int FireballPool::getActiveCount() const {
    int count = 0;
    for (const auto& fireball : _pool) {
        if (fireball && fireball->isActive()) {
            count++;
        }
    }
    return count;
}


void FireballPool::setGameWorld(GameWorld* world) {
    for (auto& fireball : _pool) {
        if (fireball) {
            fireball->setGameWorld(world);
        }
    }
}

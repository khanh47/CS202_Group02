#include "Game/Objects/Enemy/ConcreteEnemy/Koopa.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Enemy/Enemy.h"
#include "Game/World/GameWorld.h"
#include "Physics/CollisionFilter.h"
#include <memory>

Koopa::Koopa() : Enemy() {}

Koopa::Koopa(sf::Texture& texture, const std::string& animationSetId, bool isReviving) : Enemy(texture, animationSetId) {
    if (isReviving) {
        auto* animatable = getBehaviour<Animatable>();
        if (animatable && animatable->getActiveAnimationName() != "revive") {
            animatable->playAnimation("revive");
        }
        _isReviving = true;
    }
}

void Koopa::onUpdateVisuals(float deltaTime) {
    const bool facingLeft = getMoveDirection() > 0;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(deltaTime, _hitboxPixels, facingLeft);
        animatable->setVisualScale({
            Koopa::defaultVisualScaleX,
            Koopa::defaultVisualScaleY
        });
    }
}

void Koopa::onStomp() {
    if (_pendingDestroy || isDying()) {
        return;
    }

    const bool facingRight = !hasValidBody()
        || b2Body_GetLinearVelocity(_body->getId()).x >= 0.0f;
    if (_world) {
        _world->spawnKoopaShell(getPosition(), facingRight);
    }
    _pendingDestroy = true;
}

void Koopa::updateSimulation(const float &fixedDt) {
    if (!isDying() && _isReviving) {
        if (auto* animatable = getBehaviour<Animatable>()) {
            if (animatable->isAnimationDone()) {
                animatable->playAnimation("walk");
                _isReviving = false;
            }
        }
        if (_isReviving) {
            if (hasValidBody()) {
                b2Vec2 vel = b2Body_GetLinearVelocity(_body->getId());
                vel.x = 0.0f;
                b2Body_SetLinearVelocity(_body->getId(), vel);
            }
            return;
        }
    }
    if (isDying()) {
        if (auto* animatable = getBehaviour<Animatable>()) {
            animatable->setVisualScale({0.9f, 0.48f});
        }
    }
    Enemy::updateSimulation(fixedDt);
}

#include "Game/Objects/Enemy/ConcreteEnemy/Goomba.h"
#include "Game/Objects/Enemy/Enemy.h"
#include "Physics/CollisionFilter.h"

Goomba::Goomba() : Enemy() {}

Goomba::Goomba(sf::Texture& texture, const std::string& animationSetId) : Enemy(texture, animationSetId) {
}

void Goomba::updateSimulation(const float &fixedDt) {
    if (_stomped) {
        auto* animatable = getBehaviour<Animatable>();
        if (animatable && animatable->getActiveAnimationName() != "stomped") {
            animatable->playAnimation("stomped");
        }

        _deathTimer += fixedDt;
        if (_deathTimer >= 1.0f) {
            _pendingDestroy = true;
        }
        return;
    }
    Enemy::updateSimulation(fixedDt);
}

void Goomba::onStomp() {
    if (_isDying) {
        return;
    }

    _stomped = true;
    _isDying = true;
    _deathTimer = 0.0f;

    b2ShapeId shape = _body->getHitbox();
    b2Filter filter = b2Shape_GetFilter(shape);
    filter.maskBits ^= CollisionFilter::PLAYER | CollisionFilter::FIREBALL;
    b2Shape_SetFilter(shape, filter);

    b2Vec2 velocity = b2Body_GetLinearVelocity(_body->getId());
    velocity.x = 0.0f;
    b2Body_SetLinearVelocity(_body->getId(), velocity);

    if (auto *animatable = getBehaviour<Animatable>()) {
        animatable->setVisualScale({1.5f, 0.5f});
        animatable->playAnimation("stomped");
    }
}
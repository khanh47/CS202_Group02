#include "Game/Objects/Projectile/KoopaShell.h"

#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Block/Block.h"
#include "Game/Objects/Enemy/Enemy.h"
#include "Game/ScoreManager.h"
#include "Game/World/GameWorld.h"
#include "Physics/CollisionFilter.h"

#include <cmath>

KoopaShell::KoopaShell(sf::Texture& texture) : GameObject() {
    addBehaviour<Animatable>();
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "koopa");
        animatable->playAnimation("dead");
    }
}

void KoopaShell::kick(bool facingRight) {
    _facingRight = facingRight;
    _sliding = true;
    _stopTimer = 0.0f;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->playAnimation("slide");
    }
    if (!hasValidBody()) {
        return;
    }

    const b2BodyId bodyId = _body->getId();
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    vel.x = facingRight ? _slideSpeedMeters : -_slideSpeedMeters;
    b2Body_SetLinearVelocity(bodyId, vel);
}

void KoopaShell::stop() {
    _sliding = false;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->playAnimation("dead");
    }
    if (!hasValidBody()) {
        return;
    }

    b2Vec2 vel = b2Body_GetLinearVelocity(_body->getId());
    vel.x = 0.0f;
    b2Body_SetLinearVelocity(_body->getId(), vel);
}

void KoopaShell::setHeld(bool held) {
    if (_held == held) {
        return;
    }
    _held = held;
    if (!hasValidBody()) {
        return;
    }

    b2ShapeId shape = _body->getHitbox();
    if (!b2Shape_IsValid(shape)) {
        return;
    }

    if (held) {
        _savedFilter = b2Shape_GetFilter(shape);
        _hasSavedFilter = true;
        b2Filter filter = _savedFilter;
        // Don't collide with players while carried, so the shell on the
        // holder's head can't absorb their jump impulses.
        filter.maskBits &= ~CollisionFilter::PLAYER;
        b2Shape_SetFilter(shape, filter);
    } else if (_hasSavedFilter) {
        b2Shape_SetFilter(shape, _savedFilter);
        _hasSavedFilter = false;
    }
}

void KoopaShell::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_dynamicBody;
    def.motionLocks.angularZ = true;
    def.gravityScale = 2.0f;
}

void KoopaShell::onCreateShapeDef(b2ShapeDef& def) {
    def.density = 1.0f;
    def.material.friction = 0.0f;
    def.filter.categoryBits = CollisionFilter::SHELL;
    def.filter.maskBits = CollisionFilter::SHELL_MASK;
    def.enablePreSolveEvents = true;
}

void KoopaShell::updateSimulation(const float& fixedDt) {
    (void)fixedDt;
    if (!hasValidBody()) {
        return;
    }

    auto* animatable = getBehaviour<Animatable>();

    if (_isDying) {
        animatable->playAnimation("dead");

        _deathTimer += fixedDt;
        if (_deathTimer >= 3.0f) {
            _pendingDestroy = true;
        }
        return;
    }

    if (_held) {
        // Held by a player: freeze physics so the shell follows the holder,
        // but keep the revive timer running so the shell shakes and the Koopa
        // revives even while it's being carried.
        const b2BodyId bodyId = _body->getId();
        b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
        vel.x = 0.0f;
        vel.y = 0.0f;
        b2Body_SetLinearVelocity(bodyId, vel);
        _stopTimer += fixedDt;
    } else if (_sliding) {
        // Maintain constant horizontal slide speed in the current direction
        const b2BodyId bodyId = _body->getId();
        b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
        vel.x = vel.x < 0.0f ? -_slideSpeedMeters : _slideSpeedMeters;
        b2Body_SetLinearVelocity(bodyId, vel);
    } else {
        _stopTimer += fixedDt;
    }

    if (_stopTimer >= 10.0f) {
        if (_world) {
            _world->spawnKoopa(getPosition(), _facingRight);
        }
        _pendingDestroy = true;
        if (_body) {
            _body->destroy();
        }
        return;
    }
    if (_stopTimer >= 7.0f && animatable && animatable->getActiveAnimationName() != "shake") {
        // Shake for the 2s before the Koopa pops back out.
        animatable->playAnimation("shake");
    }
}

void KoopaShell::onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) {
    (void)ownShape;

    if (auto* enemy = dynamic_cast<Enemy*>(&other)) {
        if (!_sliding) {
            return;
        }
        if (_world && _world->getScoreManager()) {
            _world->getScoreManager()->handleEvent(ScoreEventType::EnemyStomped, enemy->getPosition());
        }
        enemy->destroy();
        return;
    }

    if (auto* block = dynamic_cast<Block*>(&other)) {
        if (_sliding && contactData.manifold.pointCount > 0) {
            b2Vec2 normal = contactData.manifold.normal;
            if (!B2_ID_EQUALS(contactData.shapeIdA, ownShape)) {
                normal = {-normal.x, -normal.y};
            }
            // Side hit against a wall -> bounce back in the opposite direction
            if (std::abs(normal.x) >= 0.5f) {
                b2Vec2 vel = b2Body_GetLinearVelocity(_body->getId());
                vel.x = -vel.x;
                b2Body_SetLinearVelocity(_body->getId(), vel);
            }
        }
        return;
    }
}

void KoopaShell::destroy() {
    if (_isDying) {
        return;
    }

    _isDying = true;
    _deathTimer = 0.0f;
    _stopTimer = 0.0f;
    
    b2ShapeId shape = _body->getHitbox();
    b2Filter filter = b2Shape_GetFilter(shape);
    filter.categoryBits ^= CollisionFilter::SHELL;
    b2Shape_SetFilter(shape, filter);
}

void KoopaShell::onUpdateVisuals(float deltaTime) {
    bool facingLeft = _facingRight;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(deltaTime, _hitboxPixels, facingLeft);
    }
}

void KoopaShell::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->renderVisualState(target, position, angleDegrees);
    }
}

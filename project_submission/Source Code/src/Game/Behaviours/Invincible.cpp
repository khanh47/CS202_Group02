#include "Game/Behaviours/Invincible.h"

#include "Game/Objects/GameObject.h"
#include "Physics/CollisionFilter.h"

#include <algorithm>

void Invincible::refreshCollisionMask() {
    auto* owner = getOwner();
    if (owner == nullptr) {
        return;
    }

    std::shared_ptr<PhysicsBody> body = owner->getPhysicsBody();
    if (!body || !body->isValid()) {
        return;
    }

    b2ShapeId hitbox = body->getHitbox();
    if (!b2Shape_IsValid(hitbox)) {
        return;
    }

    if (!_hasSavedFilter) {
        _savedFilter = b2Shape_GetFilter(hitbox);
        _hasSavedFilter = true;
    }

    b2Filter filter = _savedFilter;
    filter.maskBits &= ~(CollisionFilter::ENEMY | CollisionFilter::SHELL);
    b2Shape_SetFilter(hitbox, filter);
}

void Invincible::updateSimulation(const float &fixedDt) {
    _time = std::max(0.0f, _time - fixedDt);
}

void Invincible::onAttach() {
    refreshCollisionMask();
}

void Invincible::onDetach() {
    auto* owner = getOwner();
    if (owner == nullptr) {
        return;
    }

    if (_hasSavedFilter) {
        std::shared_ptr<PhysicsBody> body = owner->getPhysicsBody();
        if (body && body->isValid()) {
            b2ShapeId hitbox = body->getHitbox();
            if (b2Shape_IsValid(hitbox)) {
                b2Shape_SetFilter(hitbox, _savedFilter);
            }
        }
        _hasSavedFilter = false;
    }
}

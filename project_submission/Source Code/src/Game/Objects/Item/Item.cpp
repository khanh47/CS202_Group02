#include "Game/Objects/Item/Item.h"
#include <algorithm>

#include "Game/Behaviours/Animatable.h"
#include "box2d/box2d.h"

Item::Item() : GameObject() {
    addBehaviour<Animatable>();
}

Item::Item(sf::Texture& texture) : GameObject() {
    addBehaviour<Animatable>();
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture);
    }
}

Item::~Item() {
}

void Item::startEmerging(sf::Vector2f targetPosition, float durationSeconds) {
    if (!hasValidBody()) {
        return;
    }

    _isEmerging = true;
    _emergenceElapsed = 0.0f;
    _emergenceDuration = std::max(durationSeconds, 0.0f);
    _emergenceStartPosition = getPosition();
    _emergenceTargetPosition = targetPosition;
    _emergencePosition = _emergenceStartPosition;
    _emergenceRenderOffset = {};

    if (_emergenceDuration <= 0.0f) {
        _emergenceElapsed = _emergenceDuration;
    }
}

void Item::updateSimulation(const float& fixedDt) {
    if (_isEmerging) {
        updateEmergence(fixedDt);
    }
}

void Item::finalizeSimulation(const float& fixedDt) {
    GameObject::finalizeSimulation(fixedDt);

    if (!_isEmerging || isPendingDestroy()) {
        return;
    }

    // Dynamic power-ups can be subject to gravity during the physics step.
    // Re-apply the emergence position so they stay in the block until the
    // animation is complete.
    setPosition(_emergencePosition);
    if (hasValidBody()) {
        b2Body_SetLinearVelocity(_body->getId(), {0.0f, 0.0f});
    }

    if (_emergenceElapsed >= _emergenceDuration) {
        _isEmerging = false;
    }
}

void Item::updateEmergence(float fixedDt) {
    _emergenceElapsed = std::min(
        _emergenceElapsed + std::max(fixedDt, 0.0f),
        _emergenceDuration
    );

    const float progress = _emergenceDuration <= 0.0f
        ? 1.0f
        : _emergenceElapsed / _emergenceDuration;
    // Smooth the start and end so the item feels like it is growing out of
    // the block instead of snapping to a constant-speed translation.
    const float easedProgress = progress * progress * (3.0f - 2.0f * progress);
    _emergencePosition = _emergenceStartPosition
        + (_emergenceTargetPosition - _emergenceStartPosition) * easedProgress;
    setPosition(_emergencePosition);

    if (hasValidBody()) {
        b2Body_SetLinearVelocity(_body->getId(), {0.0f, 0.0f});
    }
}

void Item::onUpdateVisuals(float deltaTime) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(deltaTime, _hitboxPixels);
    }
}

void Item::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        const sf::Vector2f renderPosition = _isEmerging
            ? position + _emergenceRenderOffset
            : position;
        animatable->renderVisualState(target, renderPosition, angleDegrees);
    }
}

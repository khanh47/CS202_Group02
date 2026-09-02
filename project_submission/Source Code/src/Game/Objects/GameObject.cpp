#include "Game/Objects/GameObject.h"
#include "Game/GameSettings.h"
#include "Physics/PhysicsUnits.h"
#include "Game/Behaviours/Invincible.h"
#include <cmath>
#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "box2d/id.h"
#include "box2d/math_functions.h"
#include "box2d/types.h"

namespace {
constexpr bool drawFallbackCollisionRect = false;

void drawDebugRect(
    sf::RenderTarget& target,
    const sf::Vector2f& centerPixels,
const sf::Vector2f& sizePixels,
    float angleDegrees,
    const sf::Color& fillColor,
    const sf::Color& outlineColor
) {
    if (sizePixels.x <= 0.f || sizePixels.y <= 0.f) {
        return;
    }

    sf::RectangleShape rect(sizePixels);
    rect.setOrigin({sizePixels.x / 2.f, sizePixels.y / 2.f});
    rect.setPosition(centerPixels);
    rect.setRotation(sf::degrees(angleDegrees));
    rect.setFillColor(fillColor);
    rect.setOutlineThickness(1.f);
    rect.setOutlineColor(outlineColor);

    target.draw(rect);
}
}

GameObject::GameObject() = default;

GameObject::~GameObject() {
    for (auto& behaviour : _behaviours) {
        if (behaviour) {
            behaviour->detach();
        }
    }
}

void GameObject::updateSimulation(const float &fixedDt) {
    (void)fixedDt;
}

void GameObject::finalizeSimulation(const float &fixedDt) {
    (void)fixedDt;
}

sf::Vector2f GameObject::getPosition() const {
    return getBodyPositionPixels();
}

void GameObject::setPosition(sf::Vector2f positionPixels) {
    if (!hasValidBody()) {
        return;
    }

    const b2BodyId bodyId = _body->getId();
    const b2Rot rotation = b2Body_GetRotation(bodyId);
    b2Body_SetTransform(bodyId, PhysicsUnits::toMeters(positionPixels), rotation);
    b2Body_SetAwake(bodyId, true);
}

void GameObject::setVelocity(sf::Vector2f velocityPixels) {
    if (!hasValidBody()) {
        return;
    }

    b2Body_SetLinearVelocity(
        _body->getId(),
        PhysicsUnits::toMeters(velocityPixels)
    );
}

sf::Vector2f GameObject::getVelocity() const {
    // Converts Box2D linear velocity (MKS meters/sec) to SFML pixel coordinates (pixels/sec)
    // for camera tracking and motion anticipation.
    if (hasValidBody()) {
        const b2Vec2 velocityMeters = b2Body_GetLinearVelocity(_body->getId());
        return PhysicsUnits::toPixels(velocityMeters);
    }
    return {0.0f, 0.0f};
}

void GameObject::updateVisuals(float deltaTime) {
    onUpdateVisuals(deltaTime);
}

void GameObject::render(sf::RenderTarget &target) {
    if (!hasValidBody()) return;

    const sf::Vector2f position = getBodyPositionPixels();
    const float angleDegrees = getBodyAngleDegrees();

    onRenderVisual(target, position, angleDegrees);

    if (drawFallbackCollisionRect
        || GameSettings::getInstance().debugDrawHitbox) {
        onRenderDebugHitbox(target);
    }
}

void GameObject::spawn(const PhysicsWorld &physicsWorld, sf::Vector2f spawnPixels, sf::Vector2f hitboxPixels) {
    if (!physicsWorld.isValid())
        throw std::runtime_error("Invalid World!");
    if(_body && _body->isValid())
        throw std::runtime_error("The player has already been spawned!");

    createBody(physicsWorld, spawnPixels);
    createHitbox(hitboxPixels);
    updateVisuals(0.f);
}

void GameObject::destroy() {
    if (auto* invincible = getBehaviour<Invincible>()) return;
    _pendingDestroy = true;
}

void GameObject::onCreateBodyDef(b2BodyDef& def) {
    (void)def;
}

void GameObject::onCreateShapeDef(b2ShapeDef& def) {
    (void)def;
}

void GameObject::onUpdateVisuals(float deltaTime) {
    (void)deltaTime;
}

void GameObject::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    (void)target;
    (void)position;
    (void)angleDegrees;
}

void GameObject::onRenderDebugHitbox(sf::RenderTarget& target) const {
    drawFallbackRect(target);
}

void GameObject::onHitboxRecreated() {
}

b2Polygon GameObject::makeHitbox(sf::Vector2f hitboxPixels) const {
    return b2MakeBox(
        PhysicsUnits::toMeters(hitboxPixels.x * 0.5f),
        PhysicsUnits::toMeters(hitboxPixels.y * 0.5f)
    );
}

bool GameObject::hasValidBody() const {
    return _body && _body->isValid();
}

sf::Vector2f GameObject::getBodyPositionPixels() const {
    if (hasValidBody()) {
        return PhysicsUnits::toPixels(b2Body_GetPosition(_body->getId()));
    }
    return {0.f, 0.f};
}

float GameObject::getBodyAngleDegrees() const {
    if (!hasValidBody()) {
        return 0.f;
    }

    const b2Rot rotation = b2Body_GetRotation(_body->getId());
    return b2Rot_GetAngle(rotation) * (180.f / 3.14159265f);
}

void GameObject::createBody(const PhysicsWorld &physicsWorld, sf::Vector2f spawnPixels) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = PhysicsUnits::toMeters(spawnPixels);
    bodyDef.isBullet = false;
    bodyDef.userData = this;
    onCreateBodyDef(bodyDef);

    _body = std::make_shared<PhysicsBody>(physicsWorld, bodyDef);
}

void GameObject::createHitbox(sf::Vector2f hitboxPixels) {
    _hitboxPixels = hitboxPixels;
    if (_baseHitboxPixels.x <= 0.f || _baseHitboxPixels.y <= 0.f) {
        _baseHitboxPixels = hitboxPixels;
    }

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableContactEvents = true;
    shapeDef.enableSensorEvents = true;
    shapeDef.userData = this;
    onCreateShapeDef(shapeDef);

    const b2Polygon box = makeHitbox(hitboxPixels);
    
    b2ShapeId hitbox = b2CreatePolygonShape(_body->getId(), &shapeDef, &box);
    _body->setHibox(hitbox);
}

void GameObject::updateHitboxSize(sf::Vector2f newHitboxPixels) {
    if (!hasValidBody()) return;
    if (std::abs(_hitboxPixels.x - newHitboxPixels.x) < 0.01f &&
        std::abs(_hitboxPixels.y - newHitboxPixels.y) < 0.01f) {
        return;
    }

    // Shift body position upward so the bottom boundary of the hitbox remains anchored on the ground
    const float deltaYPixels = (newHitboxPixels.y - _hitboxPixels.y) * 0.5f;
    const b2BodyId bodyId = _body->getId();
    b2Vec2 currentPosMeters = b2Body_GetPosition(bodyId);
    const b2Rot currentRot = b2Body_GetRotation(bodyId);

    currentPosMeters.y -= PhysicsUnits::toMeters(deltaYPixels);
    b2Body_SetTransform(bodyId, currentPosMeters, currentRot);

    // Destroy existing shape in Box2D context
    b2ShapeId oldShape = _body->getHitbox();
    if (b2Shape_IsValid(oldShape)) {
        b2DestroyShape(oldShape, true);
    }

    _hitboxPixels = newHitboxPixels;

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableContactEvents = true;
    shapeDef.enableSensorEvents = true;
    shapeDef.userData = this;
    onCreateShapeDef(shapeDef);

    const b2Polygon box = makeHitbox(newHitboxPixels);

    b2ShapeId newHitbox = b2CreatePolygonShape(bodyId, &shapeDef, &box);
    _body->setHibox(newHitbox);

    onHitboxRecreated();
}

void GameObject::drawFallbackRect(sf::RenderTarget& target) const {
    if (!_body || !_body->isValid()) {
        return;
    }

    const b2Vec2 position = b2Body_GetPosition(_body->getId());
    const b2Rot rotation = b2Body_GetRotation(_body->getId());
    const float bodyAngleRad = b2Rot_GetAngle(rotation);
    const float angleDegrees = bodyAngleRad * (180.f / B2_PI);
    const sf::Vector2f sizePixels = _hitboxPixels;
    const sf::Vector2f bodyCenterPixels = PhysicsUnits::toPixels(position);
    drawDebugRect(
        target,
        bodyCenterPixels,
        sizePixels,
        angleDegrees,
        sf::Color(255, 0, 255, 80),
        sf::Color::Magenta
    );
}

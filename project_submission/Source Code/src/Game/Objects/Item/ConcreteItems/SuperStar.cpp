#include "Game/Objects/Item/ConcreteItems/SuperStar.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Player/Player.h"
#include "Game/GameSettings.h"
#include "Physics/CollisionFilter.h"

SuperStar::SuperStar() : Item() {
}

SuperStar::SuperStar(sf::Texture& texture) : Item() {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "super_star");
    }
}

void SuperStar::onCreateBodyDef(b2BodyDef& def) {
    // Dynamic body so gravity and restitution produce bouncing behaviour
    def.type = b2_dynamicBody;
    def.motionLocks.angularZ = true;
}

void SuperStar::onCreateShapeDef(b2ShapeDef& def) {
    // Non-sensor so the star physically bounces off terrain
    def.isSensor = false;
    def.density = 0.5f;
    def.material.friction = 0.0f;
    // High restitution for pronounced bouncing
    def.material.restitution = 0.95f;

    // PICKUP category, interacts with ENV (bouncing) + PLAYER (pickup)
    def.filter.categoryBits = CollisionFilter::PICKUP;
    def.filter.maskBits = CollisionFilter::ENV | CollisionFilter::PLAYER;
}

void SuperStar::updateSimulation(const float& fixedDt) {
    Item::updateSimulation(fixedDt);
    if (isEmerging()) {
        return;
    }

    if (!hasValidBody()) return;

    b2BodyId body = _body->getId();
    b2Vec2 vel = b2Body_GetLinearVelocity(body);

    // Detect wall bounce: if velocity reversed against our expected direction
    if (_movingRight && vel.x < -0.5f) _movingRight = false;
    if (!_movingRight && vel.x > 0.5f) _movingRight = true;

    // Maintain constant horizontal speed; leave vertical velocity to physics
    float dir = _movingRight ? 1.0f : -1.0f;
    if (GameSettings::getInstance().gameMode == GameMode::Minigame) dir = 0.0f;
    b2Body_SetLinearVelocity(body, {_speed * dir, vel.y});
}

void SuperStar::onUpdateVisuals(float deltaTime) {
    Item::onUpdateVisuals(deltaTime);

    if (hasValidBody()) {
        sf::Vector2f pos = getPosition();
        _sparkle.update(deltaTime, pos, {_hitboxPixels.x * 0.6f, _hitboxPixels.y * 0.6f});
    }
}

void SuperStar::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    Item::onRenderVisual(target, position, angleDegrees);
    _sparkle.render(target);
}

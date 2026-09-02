#include "Game/Objects/Enemy/Enemy.h"
#include "Audio/SoundManager.h"
#include "Game/Objects/GameObject.h"
#include "Game/World/TerrainSeamFilter.h"
#include "Physics/CollisionFilter.h"
#include "Physics/PhysicsUnits.h"
#include "box2d/box2d.h"
#include "box2d/id.h"
#include "box2d/types.h"
#include <cmath>


Enemy::Enemy() : GameObject() {
    addBehaviour<Animatable>();
    addBehaviour<Damageable>(50);
}

Enemy::Enemy(sf::Texture& texture) : Enemy() {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture);
    }
}

Enemy::Enemy(sf::Texture &texture, const std::string& animationSetId) : Enemy() {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, animationSetId);
    }
}

Enemy::~Enemy() {
}

void Enemy::setSupportGrid(const TerrainSeamFilter* filter, float cellSize) {
    _supportGrid = filter;
    _supportCellSize = cellSize;
}

void Enemy::setFacingRight(bool facingRight) {
    if (facingRight) {
        _moveSpeed = std::abs(_moveSpeed);
        _moveDirection = 1;
    } else {
        _moveSpeed = -std::abs(_moveSpeed);
        _moveDirection = -1;
    }
}

bool Enemy::isSupportedByGrid() const {
    if (!_supportGrid) {
        return false;
    }

    constexpr float probeBelowFeetPixels = 2.0f;
    const float feetY = getBodyPositionPixels().y + _hitboxPixels.y * 0.5f + probeBelowFeetPixels;

    return _supportGrid->isCellOccupied(probeColumn(), rowAt(feetY));
}

bool Enemy::isBlockedAhead() const {
    if (!_supportGrid) {
        return false;
    }

    const int col = probeColumn();

    const float centerY = getBodyPositionPixels().y;
    const float halfH = _hitboxPixels.y * 0.25f;
    const int topRow = rowAt(centerY - halfH);
    const int bottomRow = rowAt(centerY + halfH);
    if (_supportGrid->isCellOccupied(col, topRow)) return true;
    if (_supportGrid->isCellOccupied(col, bottomRow)) return true;

    return false;
}

int Enemy::probeColumn() const {
    if (!hasValidBody()) {
        return -1;
    }

    const b2Vec2 posMeters = b2Body_GetPosition(_body->getId());
    const sf::Vector2f posPx = PhysicsUnits::toPixels(posMeters);

    constexpr float probeForwardPixels = 2.0f;
    const float probeX = posPx.x + _moveDirection * (_hitboxPixels.x * 0.5f + probeForwardPixels);

    return static_cast<int>(std::floor(probeX / _supportCellSize));
}

int Enemy::rowAt(float pixelY) const {
    return static_cast<int>(std::floor(pixelY / _supportCellSize));
}

void Enemy::turnAround() {
    _moveSpeed = -_moveSpeed;
    flipMoveDirection();
}

void Enemy::flipMoveDirection() {
    _moveDirection = -_moveDirection;
}

void Enemy::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_dynamicBody;
    def.motionLocks.angularZ = true;
}

void Enemy::onCreateShapeDef(b2ShapeDef& def) {
    def.enablePreSolveEvents = true;
    def.density = 1.0f;
    def.material.friction = 0.0f;
    def.filter.maskBits = CollisionFilter::ENEMY_MASK;
    def.filter.categoryBits = CollisionFilter::ENEMY;
}

void Enemy::onUpdateVisuals(float deltaTime) {
    bool facingLeft = hasValidBody() && b2Body_GetLinearVelocity(_body->getId()).x < 0.f;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(deltaTime, _hitboxPixels, facingLeft);
    }
}

void Enemy::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->renderVisualState(target, position, angleDegrees);
    }
}

void Enemy::updateSimulation(const float &fixedDt) {
    (void)fixedDt;

    auto* animatable = getBehaviour<Animatable>();

    if (_isDying) {
        animatable->playAnimation("dead");

        _deathTimer += fixedDt;
        if (_deathTimer >= 3.0f) {
            _pendingDestroy = true;
        }
        return;
    }
    if (_supportGrid && (isBlockedAhead() || !isSupportedByGrid())) {
        turnAround();
    }

    b2Vec2 velocity = b2Body_GetLinearVelocity(_body->getId());
    velocity.x = _moveSpeed;
    b2Body_SetLinearVelocity(_body->getId(), velocity);
}

void Enemy::destroy() {
    if (_isDying) {
        return;
    }

    _isDying = true;
    _deathTimer = 0.0f;
    Audio::SoundManager::getInstance().playEffect("kill");

    b2Body_SetGravityScale(_body->getId(), 2.0f);
    
    b2ShapeId shape = _body->getHitbox();
    b2Filter filter = b2Shape_GetFilter(shape);
    filter.categoryBits ^= CollisionFilter::ENEMY;
    b2Shape_SetFilter(shape, filter);
}

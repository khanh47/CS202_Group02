#include "Game/Objects/Player/Player.h"
#include "Audio/SoundManager.h"
#include "Game/GameSettings.h"
#include "Game/Objects/GameObject.h"
#include "Game/World/GameWorld.h"
#include "Game/Objects/Player/State/NormalState.h"
#include "Game/Objects/Player/State/SuperState.h"
#include "Game/Objects/Player/State/FireState.h"
#include "Game/Objects/Player/State/MegaStateDecorator.h"
#include "Game/Objects/Player/State/StarManStateDecorator.h"
#include "Physics/CollisionFilter.h"
#include "Physics/PhysicsUnits.h"
#include "Game/Objects/Block/CoinBlock.h"
#include "Game/Objects/Block/SlopeBlock.h"
#include "Game/Objects/Enemy/Enemy.h"
#include "Game/Objects/Item/ConcreteItems/FireFlower.h"
#include "Game/Objects/Item/ConcreteItems/SuperMushroom.h"
#include "Game/Objects/Item/ConcreteItems/OneUpMushroom.h"
#include "Game/Objects/Item/ConcreteItems/MegaMushroom.h"
#include "Game/Objects/Item/ConcreteItems/SuperStar.h"
#include "Game/Objects/Item/ConcreteItems/Coin.h"
#include "Game/Objects/Item/ConcreteItems/MegaCoin.h"
#include "Game/Objects/Item/Item.h"
#include "Game/Objects/Projectile/KoopaShell.h"
#include "Game/Objects/Pipe/Pipe.h"
#include "ResourceManager.h"
#include "Game/Objects/Player/PlayerShaders.h"
#include "Game/Behaviours/Invincible.h"
#include "box2d/id.h"

#include <SFML/System/Clock.hpp>
#include <algorithm>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/System/Vector2.hpp>
#include <ctime>
#include <iostream>
#include <iterator>

namespace {
constexpr float highFallBreakDistancePixels =
    25.0f * PhysicsUnits::pixelsPerMeter;
// With the player's current gravity scale, this is approximately the impact
// speed reached after a 20-cell fall. Capture it before Box2D resolves contact.
constexpr float highFallBreakVelocityPixelsPerSecond = 2400.0f;
constexpr float pipeWarpDiveDurationSeconds = 0.65f;
constexpr float pipeWarpTravelDurationSeconds = 0.35f;
constexpr float pipeWarpLandingIdleDurationSeconds = 0.18f;
constexpr float pipeWarpRiseDurationSeconds =
    Player::pipeWarpDurationSeconds
    - pipeWarpDiveDurationSeconds
    - pipeWarpTravelDurationSeconds;
constexpr float footstepIntervalSeconds = 0.28f;
constexpr float deathSoundTailSeconds = 0.10f;

float smoothStep(float progress) {
    progress = std::clamp(progress, 0.0f, 1.0f);
    return progress * progress * (3.0f - 2.0f * progress);
}

float moveToward(float current, float target, float maximumDelta) {
    return current + std::clamp(
        target - current,
        -maximumDelta,
        maximumDelta
    );
}

}

Player::Player() : GameObject() {
    addBehaviour<Animatable>();
    addBehaviour<Damageable>(100);
    addBehaviour<Moveable>();
    addBehaviour<ShellHoldBehaviour>();
    setState(std::make_unique<NormalState>());
}

Player::Player(sf::Texture &texture) : Player(texture, "mario") {
}

Player::Player(sf::Texture &texture, const std::string& animationSetId)
    : GameObject() {
    (void)texture;
    addBehaviour<Animatable>();
    addBehaviour<Damageable>(100);
    addBehaviour<Moveable>();
    addBehaviour<ShellHoldBehaviour>();
    _character = animationSetId.find("luigi") != std::string::npos ? "luigi" : "mario";
    if (animationSetId.rfind("fire_", 0) == 0) {
        setState(std::make_unique<FireState>(_character));
    } else {
        setState(std::make_unique<NormalState>(_character));
    }
}

Player::~Player() = default;

void Player::destroy() {
    if (auto* invincible = getBehaviour<Invincible>()) return;

    if (auto* hold = getBehaviour<ShellHoldBehaviour>()) {
        hold->releaseShell(false);
    }

    b2ShapeId shape = _body->getHitbox();
    b2Filter filter = b2Shape_GetFilter(shape);
    filter.maskBits ^= CollisionFilter::ENEMY | CollisionFilter::SHELL | CollisionFilter::PICKUP;
    if (GameSettings::getInstance().gameMode == GameMode::Minigame) {
      filter.maskBits ^= CollisionFilter::MINIGAME_MASK;
    }
    b2Shape_SetFilter(shape, filter);

    if (_isDying) {
        return;
    }

    _isDying = true;
    _deathSoundElapsedSeconds = 0.0f;
    _deathSoundDurationSeconds = 0.0f;
    try {
        _deathSoundDurationSeconds = ResourceManager::getInstance()
            .getSoundBuffer("dead")
            .getDuration()
            .asSeconds();
    } catch (...) {
    }
    Audio::SoundManager::getInstance().playEffect("dead");

    removeBehaviour<Moveable>();
    auto* animatable = getBehaviour<Animatable>();
    if (animatable) {
        animatable->playAnimation("knockout");
    }
    // _pendingDestroy is set later in updateSimulation once knockout finishes.
}

void Player::finalizeGroundContacts() {
    auto* moveable = getBehaviour<Moveable>();
    if (!moveable) {
        return;
    }

    moveable->finalizeGroundContacts();
    // Reset the stomp combo whenever the player is on solid ground,
    // so the ladder only grows during genuine airborne chains.
    if (moveable->hasGroundSupport()) {
        _fallTrackingActive = false;
        _fallDistancePixels = 0.0f;
        _fallStartY = getPosition().y;
        _maxDownwardVelocityPixelsPerSecond = 0.0f;
    }
    if (!moveable->isAirbone()) {
        if (_world && _world->getScoreManager()) {
            _world->getScoreManager()->handleEvent(ScoreEventType::MarioLanded, {0.f, 0.f}, 0, _character);
        }
    }
}

PlayerMovementStats Player::getMovementStats() const noexcept {
    PlayerMovementStats stats{
        _baseMoveSpeed,
        _baseAcceleration,
        _baseTraction,
        _baseJumpSpeed
    };

    if (_character == "luigi") {
        stats.topSpeedMetersPerSecond *= luigiTopSpeedRatio;
        stats.accelerationMetersPerSecondSquared *= luigiAccelerationRatio;
        stats.tractionMetersPerSecondSquared *= luigiTractionRatio;
        stats.jumpSpeedMetersPerSecond *= luigiJumpSpeedRatio;
    }

    if (_state) {
        stats.topSpeedMetersPerSecond *= _state->getMoveSpeedMultiplier();
        stats.jumpSpeedMetersPerSecond *= _state->getJumpSpeedMultiplier();
    }
    return stats;
}

void Player::setState(std::unique_ptr<PlayerState> newState) {
    if (!newState) return;

    if (_state) {
        _state->onExit(*this);
    }

    _state = std::move(newState);
    _state->onEnter(*this);
    refreshStatePresentation();
}

void Player::refreshStatePresentation() {
    if (!_state) {
        return;
    }

    _attackStrategy = _state->createAttackStrategy();

    // Apply texture and animation set dynamically from the current state
    const std::string& texAlias = _state->getTextureAlias();
    const std::string& animId = _state->getAnimationSetId();
    sf::Texture& tex = ResourceManager::getInstance().getTexture(texAlias);
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(tex, animId);
        // State changes must discard any non-looping attack clip from the
        // previous form immediately. Otherwise a Fire Mario shoot frame can
        // remain visible for one render after returning to Normal.
        animatable->playAnimation("idle", true);
    }
}

std::string Player::getBaseStateNameForSave() const {
    const PlayerState* current = _state.get();
    while (current) {
        if (const auto* star = dynamic_cast<const StarManStateDecorator*>(current)) {
            current = star->getWrappedState();
            continue;
        }
        if (const auto* mega = dynamic_cast<const MegaStateDecorator*>(current)) {
            current = mega->getStateToRestore();
            if (!current) {
                current = mega->getWrappedState();
            }
            continue;
        }
        return current->getStateName();
    }
    return "Normal";
}

float Player::getMegaStateTimeRemaining() const noexcept {
    const PlayerState* current = _state.get();
    while (current) {
        if (const auto* mega = dynamic_cast<const MegaStateDecorator*>(current)) {
            return std::max(mega->getRemainingTime(), 0.0f);
        }
        if (const auto* star = dynamic_cast<const StarManStateDecorator*>(current)) {
            current = star->getWrappedState();
            continue;
        }
        break;
    }
    return 0.0f;
}

float Player::getStarManStateTimeRemaining() const noexcept {
    const PlayerState* current = _state.get();
    while (current) {
        if (const auto* star = dynamic_cast<const StarManStateDecorator*>(current)) {
            return std::max(star->getRemainingTime(), 0.0f);
        }
        if (const auto* mega = dynamic_cast<const MegaStateDecorator*>(current)) {
            current = mega->getWrappedState();
            continue;
        }
        break;
    }
    return 0.0f;
}

void Player::restoreSavedState(
    const std::string& baseStateName,
    float megaTimeRemaining,
    float starManTimeRemaining
) {
    if (baseStateName == "Fire") {
        changeToFireState();
    } else if (baseStateName == "Super") {
        changeToSuperState();
    } else {
        changeToNormalState();
    }

    if (megaTimeRemaining > 0.0f) {
        applyMegaState(megaTimeRemaining);
    }
    if (starManTimeRemaining > 0.0f) {
        applyStarManState(starManTimeRemaining);
    }
}

void Player::attack(GameWorld& world) {
    // Can't shoot fireballs while carrying a shell.
    if (auto* hold = getBehaviour<ShellHoldBehaviour>()) {
        if (hold->isHoldingShell()) {
            return;
        }
    }
    if (_state) {
        _attackStrategy = _state->createAttackStrategy();
    }
    if (_attackStrategy) {
        _attackStrategy->executeAttack(*this, world);
    }
}

bool Player::beginPipeWarp(
    sf::Vector2f sourceOutside,
    sf::Vector2f sourceInside,
    sf::Vector2f targetInside,
    sf::Vector2f targetOutside
) {
    if (_isPipeWarping || !hasValidBody()) {
        return false;
    }

    _isPipeWarping = true;
    _pipeWarpExitSoundPlayed = false;
    _pipeWarpTimer = 0.0f;
    _pipeWarpIdleTimer = 0.0f;
    _pipeWarpSourceOutside = sourceOutside;
    _pipeWarpSourceInside = sourceInside;
    _pipeWarpTargetInside = targetInside;
    _pipeWarpTargetOutside = targetOutside;

    if (auto* moveable = getBehaviour<Moveable>()) {
        moveable->stopMoveLeft();
        moveable->stopMoveRight();
        moveable->stopJump();
        moveable->resetGroundContacts();
    }

    setPosition(_pipeWarpSourceOutside);
    if (auto body = getPhysicsBody()) {
        b2Body_SetLinearVelocity(body->getId(), {0.0f, 0.0f});
    }

    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->playAnimation("idle", true);
    }

    Audio::SoundManager::getInstance().playEffect("pipe");

    return true;
}

void Player::changeToNormalState() {
    setState(std::make_unique<NormalState>(_character));
}

void Player::changeToSuperState() {
    setState(std::make_unique<SuperState>(_character));
}

void Player::changeToFireState() {
    setState(std::make_unique<FireState>(_character));
}

void Player::applyMegaState(float durationSeconds) {
    if (!_state) {
        _state = std::make_unique<NormalState>(_character);
    }

    if (auto* megaState = dynamic_cast<MegaStateDecorator*>(_state.get())) {
        megaState->resetTimer(durationSeconds);
        return;
    }

    std::unique_ptr<PlayerState> wrappedState = std::move(_state);
    std::unique_ptr<PlayerState> stateToRestore;
    if (dynamic_cast<FireState*>(wrappedState.get())) {
        // Mega temporarily presents the normal state and disables fireballs,
        // but keeps the FireState so it can be restored when Mega expires.
        stateToRestore = std::move(wrappedState);
        wrappedState = std::make_unique<NormalState>(_character);
    }

    setState(std::make_unique<MegaStateDecorator>(
        std::move(wrappedState),
        durationSeconds,
        std::move(stateToRestore)
    ));
}

void Player::applyStarManState(float durationSeconds) {
    if (!_state) {
        _state = std::make_unique<NormalState>(_character);
    }
    auto* starDecorator = dynamic_cast<StarManStateDecorator*>(_state.get());
    if (starDecorator) {
        starDecorator->resetTimer(durationSeconds);
        return;
    }
    setState(std::make_unique<StarManStateDecorator>(std::move(_state), durationSeconds));
}

void Player::revertDecoratedState() {
    if (!_state) return;

    auto* decorator = dynamic_cast<PlayerStateDecorator*>(_state.get());
    if (decorator) {
        std::unique_ptr<PlayerState> unwrapped = decorator->unwrap();
        if (unwrapped) {
            setState(std::move(unwrapped));
        }
    }
}

void Player::updateSimulation(const float &fixedDt) {
    if (!_body || !_body->isValid()) {
        return;
    }

    if (_isPipeWarping) {
        return;
    }

    _warpCooldown = std::max(0.0f, _warpCooldown - fixedDt);
    if (_world && !isMegaState() && _warpCooldown <= 0.0f) {
        if (_world->tryWarpPlayer(*this)) {
            _warpCooldown = 0.35f;
            return;
        }
    }

    b2Vec2 velocity = b2Body_GetLinearVelocity(_body->getId());

    auto* invincible = getBehaviour<Invincible>();
    auto* animatable = getBehaviour<Animatable>();
    auto* moveable = getBehaviour<Moveable>();

    
    if (invincible) {
        invincible->updateSimulation(fixedDt);
        if(invincible->getTime() == 0.0){
            removeBehaviour<Invincible>();
            velocity.y -= 2.01;
        };
    }
    

    if (_isDying) {
        _deathSoundElapsedSeconds += fixedDt;
        if (!animatable) {
            _pendingDestroy = true;
            return;
        }

        if (animatable->getActiveAnimationName() != "knockout") {
            animatable->playAnimation("knockout");
        }

        const float deathSoundEndThreshold = std::max(
            0.0f,
            _deathSoundDurationSeconds - deathSoundTailSeconds
        );
        if (animatable->isAnimationDone()
            && _deathSoundElapsedSeconds >= deathSoundEndThreshold) {
            _pendingDestroy = true;
        }
        return;
    }

    if (!moveable) {
        return;
    }

    updateFallTracking();

    if (auto* hold = getBehaviour<ShellHoldBehaviour>()) {
        hold->updateSimulation(fixedDt);
    }

    const PlayerMovementStats movement = getMovementStats();
    const float moveSpeed = movement.topSpeedMetersPerSecond;
    const float acceleration = movement.accelerationMetersPerSecondSquared;
    const float traction = movement.tractionMetersPerSecondSquared;
    const float jumpSpeed = movement.jumpSpeedMetersPerSecond;

    float targetVelocityX = 0.0f;
    if (moveable->isMovingLeft() && !moveable->isMovingRight()) {
        targetVelocityX = -moveSpeed;
    } else if (moveable->isMovingRight() && !moveable->isMovingLeft()) {
        targetVelocityX = moveSpeed;
    }

    const bool reversing = targetVelocityX != 0.0f
        && velocity.x != 0.0f
        && (targetVelocityX > 0.0f) != (velocity.x > 0.0f);
    const float velocityChangeRate =
        targetVelocityX == 0.0f || reversing
            ? traction
            : acceleration;
    velocity.x = moveToward(
        velocity.x,
        reversing ? 0.0f : targetVelocityX,
        velocityChangeRate * fixedDt
    );

    const bool isStartingJump = moveable->isJumping() && !moveable->isAirbone();
    if (isStartingJump) {
        Audio::SoundManager::getInstance().playEffect("jump");
        velocity.y = -jumpSpeed;
        moveable->consumeGroundForJump();
    }

    const bool isWalking = !moveable->isAirbone()
        && (moveable->isMovingLeft() != moveable->isMovingRight());
    if (isWalking) {
        _footstepTimer -= fixedDt;
        if (_footstepTimer <= 0.0f) {
            Audio::SoundManager::getInstance().playEffect("footstep");
            _footstepTimer = footstepIntervalSeconds;
        }
    } else {
        _footstepTimer = 0.0f;
    }


    if (_flyMode) {
        b2Body_SetGravityScale(_body->getId(), 0.0f);
        if (moveable->isJumping() || _moveUpHeld) {
            velocity.y = -moveSpeed;
        } else if (_moveDownHeld) {
            velocity.y = moveSpeed;
        } else {
            velocity.y = 0.0f;
        }
    } else {
        b2Body_SetGravityScale(_body->getId(), 4.0f);
        if (moveable->isAirbone() || moveable->isJumping()) {
            if (velocity.y > 0) b2Body_SetGravityScale(_body->getId(), moveable->isJumping() ? 3.0f : 4.0f);
        }
    }

    b2Body_SetLinearVelocity(_body->getId(), velocity);
}

bool Player::hasFallenFromHighPlace() const noexcept {
    if (!_fallTrackingActive) {
        return false;
    }

    const float currentFallDistance = hasValidBody()
        ? getPosition().y - _fallStartY
        : 0.0f;
    return std::max(_fallDistancePixels, currentFallDistance)
        >= highFallBreakDistancePixels
        || _maxDownwardVelocityPixelsPerSecond
            >= highFallBreakVelocityPixelsPerSecond;
}

void Player::updateFallTracking() {
    auto* moveable = getBehaviour<Moveable>();
    if (!moveable || !hasValidBody()) {
        return;
    }

    const float currentY = getPosition().y;
    if (moveable->hasGroundSupport()) {
        _fallTrackingActive = false;
        _fallDistancePixels = 0.0f;
        _fallStartY = currentY;
        _maxDownwardVelocityPixelsPerSecond = 0.0f;
        return;
    }

    if (!_fallTrackingActive) {
        _fallTrackingActive = true;
        _fallStartY = currentY;
        _fallDistancePixels = 0.0f;
        _maxDownwardVelocityPixelsPerSecond = 0.0f;
    }

    _fallDistancePixels = std::max(
        _fallDistancePixels,
        currentY - _fallStartY
    );

    const b2Vec2 velocityMeters = b2Body_GetLinearVelocity(_body->getId());
    const float downwardVelocityPixels =
        PhysicsUnits::toPixels(velocityMeters.y);
    if (downwardVelocityPixels > 0.0f) {
        _maxDownwardVelocityPixelsPerSecond = std::max(
            _maxDownwardVelocityPixelsPerSecond,
            downwardVelocityPixels
        );
    }
}

void Player::updatePipeWarpVisuals(float deltaTime) {
    _pipeWarpTimer = std::min(
        pipeWarpDurationSeconds,
        _pipeWarpTimer + std::max(deltaTime, 0.0f)
    );

    const float pipeWarpEmergenceStart =
        pipeWarpDiveDurationSeconds + pipeWarpTravelDurationSeconds;
    if (!_pipeWarpExitSoundPlayed
        && _pipeWarpTimer >= pipeWarpEmergenceStart) {
        _pipeWarpExitSoundPlayed = true;
        Audio::SoundManager::getInstance().playEffect("pipe");
    }

    sf::Vector2f position = _pipeWarpSourceInside;
    if (_pipeWarpTimer < pipeWarpDiveDurationSeconds) {
        const float progress = smoothStep(
            _pipeWarpTimer / pipeWarpDiveDurationSeconds
        );
        position = _pipeWarpSourceOutside
            + (_pipeWarpSourceInside - _pipeWarpSourceOutside) * progress;
    } else if (_pipeWarpTimer < pipeWarpDiveDurationSeconds
               + pipeWarpTravelDurationSeconds) {
        // The player stays hidden inside the pipe while the destination is
        // reached. This keeps a distant warp from looking like a teleport.
        position = _pipeWarpTargetInside;
    } else {
        const float riseProgress = smoothStep(
            (_pipeWarpTimer
             - pipeWarpDiveDurationSeconds
             - pipeWarpTravelDurationSeconds)
            / pipeWarpRiseDurationSeconds
        );
        position = _pipeWarpTargetInside
            + (_pipeWarpTargetOutside - _pipeWarpTargetInside) * riseProgress;
    }

    setPosition(position);
    if (auto body = getPhysicsBody()) {
        b2Body_SetLinearVelocity(body->getId(), {0.0f, 0.0f});
    }

    auto* animatable = getBehaviour<Animatable>();
    if (animatable) {
        animatable->playAnimation("idle");
        animatable->setVisualScale({
            Player::defaultVisualScaleX,
            Player::defaultVisualScaleY
        });
        animatable->updateVisualState(
            deltaTime,
            _hitboxPixels,
            isFacingLeft()
        );
    }

    if (_pipeWarpTimer < pipeWarpDurationSeconds) {
        return;
    }

    setPosition(_pipeWarpTargetOutside);
    if (auto body = getPhysicsBody()) {
        b2Body_SetLinearVelocity(body->getId(), {0.0f, 0.0f});
    }
    if (auto* moveable = getBehaviour<Moveable>()) {
        moveable->resetGroundContacts();
    }

    _pipeWarpIdleTimer = pipeWarpLandingIdleDurationSeconds;
    _isPipeWarping = false;
    if (_world) {
        _world->releaseFreeze();
    }
    if (animatable) {
        animatable->playAnimation("idle", true);
    }
}

void Player::finalizeSimulation(const float &fixedDt) {
    (void)fixedDt;

    auto* moveable = getBehaviour<Moveable>();
    auto* animatable = getBehaviour<Animatable>();
    if (!moveable || !animatable) {
        return;
    }

    if (_pipeWarpIdleTimer > 0.0f) {
        animatable->playAnimation("idle");
        return;
    }

    if(!animatable->isLooping() && !animatable->isAnimationDone()) return;

    const auto* hold = getBehaviour<ShellHoldBehaviour>();
    const bool holding = hold && hold->isHoldingShell();
    if (holding) {
        if (!moveable->isMovingLeft() && !moveable->isMovingRight()) {
            animatable->playAnimation("hold_stand");
        } else {
            animatable->playAnimation("hold_walk");
        }
    } else if (moveable->isAirbone() || moveable->isJumping()) {
        animatable->playAnimation("jump");
    } else if (!moveable->isMovingLeft() && !moveable->isMovingRight()) {
        animatable->playAnimation("idle");
    } else {
        animatable->playAnimation("walk");
    }
}

void Player::onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) {
    if (auto* item = dynamic_cast<Item*>(&other)) {
        handleItemContact(*item);
        return;
    }
    if (isMegaState()
        && (dynamic_cast<Block*>(&other) || dynamic_cast<Pipe*>(&other))) {
        handleMegaEnvironmentContact(other, contactData, ownShape);
        return;
    }
    if (auto* shell = dynamic_cast<KoopaShell*>(&other)) {
        handleShellContact(*shell, contactData, ownShape);
        return;
    }
    if (auto* enemy = dynamic_cast<Enemy*>(&other)) {
        handleEnemyContact(*enemy, contactData, ownShape);
        return;
    }
    if (auto* player = dynamic_cast<Player*>(&other)) {
        handlePlayerContact(*player, contactData, ownShape);
    }
}

void Player::handleItemContact(Item& item) {
    if (auto* megaMushroom = dynamic_cast<MegaMushroom*>(&item)) {
        megaMushroom->onPickup(*this);
        awardScore(ScoreEventType::PowerupCollected, getPosition());
    } else if (auto* oneUpMushroom = dynamic_cast<OneUpMushroom*>(&item)) {
        oneUpMushroom->onPickup(*this);
        awardScore(ScoreEventType::OneUpCollected, getPosition());
    } else if (auto* mushroom = dynamic_cast<SuperMushroom*>(&item)) {
        if (_state) _state->handleSuperMushroom(*this);
        awardScore(ScoreEventType::PowerupCollected, getPosition());
        mushroom->destroy();
    } else if (auto* fireFlower = dynamic_cast<FireFlower*>(&item)) {
        if (_state) _state->handleFireFlower(*this);
        awardScore(ScoreEventType::PowerupCollected, getPosition());
        fireFlower->destroy();
    } else if (auto* star = dynamic_cast<SuperStar*>(&item)) {
        if (_state) _state->handleSuperStar(*this);
        awardScore(ScoreEventType::PowerupCollected, getPosition());
        star->destroy();
    } else if (auto* coin = dynamic_cast<Coin*>(&item)) {
        Audio::SoundManager::getInstance().playEffect("coin");
        awardScore(ScoreEventType::CoinCollected, getPosition());
        coin->destroy();
    } else if (auto* megaCoin = dynamic_cast<MegaCoin*>(&item)) {
        Audio::SoundManager::getInstance().playEffect("coin");
        awardScore(ScoreEventType::MegaCoinCollected, getPosition());
        megaCoin->destroy();
    }
}

void Player::beginMegaEndTransformation() {
    auto* megaState = dynamic_cast<MegaStateDecorator*>(_state.get());
    if (!megaState) {
        revertDecoratedState();
        return;
    }

    _stateAfterTransformation = megaState->takeStateAfterMega();
    if (!_stateAfterTransformation) {
        _stateAfterTransformation = std::make_unique<NormalState>(_character);
    }
    _transformEndScale = _stateAfterTransformation->getScaleMultiplier().x;

    if (_world) {
        startTransformation(TransformTarget::MegaEnd, *_world);
    } else {
        setState(std::move(_stateAfterTransformation));
    }
}

void Player::beginStarManEndTransformation() {
    auto* starManState = dynamic_cast<StarManStateDecorator*>(_state.get());
    if (!starManState) {
        revertDecoratedState();
        return;
    }

    if (const auto* wrappedState = starManState->getWrappedState()) {
        _transformEndScale = wrappedState->getScaleMultiplier().x;
    } else {
        _transformEndScale = 1.0f;
    }

    if (_world) {
        // Start while the decorator is still intact so the transformation
        // captures the active StarMan presentation before unwrapping it.
        startTransformation(TransformTarget::StarManEnd, *_world);
        _stateAfterTransformation = starManState->unwrap();
        if (!_stateAfterTransformation) {
            _stateAfterTransformation = std::make_unique<NormalState>(_character);
        }
    } else {
        std::unique_ptr<PlayerState> stateAfterTransformation = starManState->unwrap();
        if (stateAfterTransformation) {
            setState(std::move(stateAfterTransformation));
        } else {
            changeToNormalState();
        }
    }
}

void Player::handleMegaEnvironmentContact(
    GameObject& other,
    const b2ContactData& contactData,
    b2ShapeId ownShape
) {
    // Mega destroys breakable terrain, live block objects (coin/lucky
    // blocks), and unbreakable bricks on contact. Slopes and permanent ground
    // terrain are permanent level geometry and must not be destroyed.
    if (auto* block = dynamic_cast<Block*>(&other)) {
        if (dynamic_cast<SlopeBlock*>(block)) {
            return;
        }

        const bool isLiveBlock = !block->isRenderedByTileMap();
        const bool canMegaBreak = block->isBreakable() || isLiveBlock || block->isBrick();
        if (canMegaBreak
            && !block->isPendingDestroy()) {
            Audio::SoundManager::getInstance().playEffect("break");
            if (_world) {
                block->spawnBreakEffect(*_world);
            }
            block->destroy();
        }
        return;
    }

    if (auto* pipe = dynamic_cast<Pipe*>(&other)) {
        if (!b2Shape_IsValid(ownShape)) {
            return;
        }

        const b2ShapeId pipeShape =
            B2_ID_EQUALS(contactData.shapeIdA, ownShape)
            ? contactData.shapeIdB
            : contactData.shapeIdA;

        if (_world) {
            if (const auto breakData = pipe->getSegmentBreakData(pipeShape)) {
                Audio::SoundManager::getInstance().playEffect("break");
                _world->spawnBlockBreakEffect(
                    breakData->position,
                    breakData->size,
                    pipe->getTexture(),
                    breakData->textureRect
                );
            }
        }
        pipe->breakSegment(pipeShape);
        // Shape destruction invalidates the future Box2D end event, so clear
        // this player's ground tracker explicitly if this segment supported it.
        endGroundContact(pipeShape);
    }
}

void Player::handleShellContact(
    KoopaShell& shell,
    const b2ContactData& contactData,
    b2ShapeId ownShape
) {
    if (shell.isHeld()) return;

    if (_state && _state->isInvincible()) {
        awardScore(ScoreEventType::EnemyStomped, shell.getPosition());
        shell.destroy();
        return;
    }

    if (!b2Shape_IsValid(ownShape)) return;

    if (isTopContact(contactData, ownShape)) {
        if (shell.isSliding()) shell.stop();
        else shell.kick(!isFacingLeft());
        bounce();
    } else if (shell.isSliding() && _state) {
        _state->handleEnemy(*this);
    } else {
        shell.kick(!isFacingLeft());
    }
}

void Player::handleEnemyContact(
    Enemy& enemy,
    const b2ContactData& contactData,
    b2ShapeId ownShape
) {
    if (_state && _state->isInvincible()) {
        awardScore(ScoreEventType::EnemyStomped, enemy.getPosition());
        enemy.destroy();
        return;
    }

    if (isTopContact(contactData, ownShape) && enemy.canBeStomped()) {
        Audio::SoundManager::getInstance().playEffect("kill");
        awardScore(ScoreEventType::EnemyStomped, enemy.getPosition());
        enemy.onStomp();
        bounce();
    } else if (_state) {
        _state->handleEnemy(*this);
    }
}

void Player::handlePlayerContact(
    Player& player,
    const b2ContactData& contactData,
    b2ShapeId ownShape
) {
    if (GameSettings::getInstance().gameMode != GameMode::Minigame
        || !b2Shape_IsValid(ownShape)) {
        return;
    }

    if (isTopContact(contactData, ownShape)) {
        player.destroy();
        bounce();
    } else if (auto* damageable = getBehaviour<Damageable>()) {
        damageable->takeDamage(50);
    }
}

bool Player::isTopContact(
    const b2ContactData& contactData,
    b2ShapeId ownShape
) const {
    if (!b2Shape_IsValid(ownShape) || contactData.manifold.pointCount == 0) {
        return false;
    }

    b2Vec2 normal = contactData.manifold.normal;
    if (!B2_ID_EQUALS(contactData.shapeIdA, ownShape)) {
        normal = {-normal.x, -normal.y};
    }
    return normal.y >= 0.5f;
}

void Player::bounce(float verticalVelocity) {
    if (!_body) return;

    const b2BodyId bodyId = b2Shape_GetBody(_body->getHitbox());
    b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
    velocity.y = verticalVelocity;
    b2Body_SetLinearVelocity(bodyId, velocity);
}

void Player::awardScore(ScoreEventType event, sf::Vector2f position) {
    if (_world && _world->getScoreManager()) {
        _world->getScoreManager()->handleEvent(event, position, 0, _character);
    }
}

bool Player::isMegaState() const noexcept {
    return dynamic_cast<const MegaStateDecorator*>(_state.get()) != nullptr;
}

bool Player::isStarManState() const noexcept {
    return dynamic_cast<const StarManStateDecorator*>(_state.get()) != nullptr;
}

void Player::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_dynamicBody;
    def.motionLocks.angularZ = true;
}

void Player::onCreateShapeDef(b2ShapeDef& def) {
    def.density = 1.0f;
    def.material.friction = 0.0f;
    def.enablePreSolveEvents = true;

    def.filter.categoryBits = CollisionFilter::PLAYER;
    def.filter.maskBits = CollisionFilter::PLAYER_MASK;
    if (GameSettings::getInstance().gameMode == GameMode::Minigame)
        def.filter.maskBits |= CollisionFilter::MINIGAME_MASK;
}

b2Polygon Player::makeHitbox(sf::Vector2f hitboxPixels) const {
    constexpr float cornerRadiusPixels = 4.0f;
    const float halfWidthPixels =
        std::max(0.0f, hitboxPixels.x * 0.5f - cornerRadiusPixels);
    const float halfHeightPixels =
        std::max(0.0f, hitboxPixels.y * 0.5f - cornerRadiusPixels);
    return b2MakeRoundedBox(
        PhysicsUnits::toMeters(halfWidthPixels),
        PhysicsUnits::toMeters(halfHeightPixels),
        PhysicsUnits::toMeters(cornerRadiusPixels)
    );
}

void Player::startTransformation(TransformTarget target, float duration) {
    if (_world) {
        startTransformation(target, *_world, duration);
    }
}

void Player::startTransformation(TransformTarget target, GameWorld& world, float duration) {
    if (_isTransforming) return;

    const bool isPowerDownTransformation =
        target == TransformTarget::MegaEnd
        || target == TransformTarget::StarManEnd
        || (target == TransformTarget::Normal
            && (dynamic_cast<FireState*>(_state.get())
                || dynamic_cast<SuperState*>(_state.get())));
    if (isPowerDownTransformation) {
        Audio::SoundManager::getInstance().playEffect("power_down");
    } else if (target == TransformTarget::Mega) {
        Audio::SoundManager::getInstance().playEffect("mega_up");
    } else if (target == TransformTarget::Super
               || target == TransformTarget::Fire
               || target == TransformTarget::StarMan) {
        Audio::SoundManager::getInstance().playEffect("power_up");
    }

    _isTransforming = true;
    _transformTimer = duration;
    _transformDuration = duration > 0.0f ? duration : 1.0f;
    _transformTarget = target;

    // Snapshot current scale so the animation can lerp to the target state
    // scale, including the colossal Mega scale.
    _transformStartScale = _state ? _state->getScaleMultiplier().x : 1.0f;

    // Stop movement inputs and clear horizontal physics velocity when transformation starts
    if (auto* moveable = getBehaviour<Moveable>()) {
        moveable->stopMoveLeft();
        moveable->stopMoveRight();
        moveable->stopJump();
    }
    if (hasValidBody()) {
        const b2BodyId bodyId = _body->getId();
        b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
        vel.x = 0.0f;
        b2Body_SetLinearVelocity(bodyId, vel);
    }

    // Freeze physics simulation & world updates for the duration of transformation
    world.freeze(duration);

    // The blink shader handles the visual flash effect, so no spritesheet swap needed.
    // Reset effect clock so the blink starts cleanly from t=0.
    _effectTime = 0.0f;
}

void Player::onUpdateVisuals(float deltaTime) {
    _effectTime += deltaTime;
    PlayerShaders::getInstance().update(deltaTime);
    if (!_isPipeWarping && _pipeWarpIdleTimer > 0.0f) {
        _pipeWarpIdleTimer = std::max(
            0.0f,
            _pipeWarpIdleTimer - std::max(deltaTime, 0.0f)
        );
    }
    auto* moveable = getBehaviour<Moveable>();
    const bool facingLeft = moveable ? moveable->isFacingLeft() : false;

    if (auto* hold = getBehaviour<ShellHoldBehaviour>()) {
        hold->updateVisuals(deltaTime);
    }

    if (_isPipeWarping) {
        updatePipeWarpVisuals(deltaTime);
        return;
    }

    if (_isTransforming) {
        _transformTimer -= deltaTime;
        if (_transformTimer <= 0.0f) {
            _isTransforming = false;
            if (auto* moveable = getBehaviour<Moveable>()) {
                moveable->stopMoveLeft();
                moveable->stopMoveRight();
                moveable->stopJump();
            }
            if (hasValidBody()) {
                const b2BodyId bodyId = _body->getId();
                b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
                vel.x = 0.0f;
                b2Body_SetLinearVelocity(bodyId, vel);
            }
            // Transformation finished -> enter the correct target state
            if (_transformTarget == TransformTarget::Normal) {
                changeToNormalState();
            }
            else if (_transformTarget == TransformTarget::Fire) {
                changeToFireState();
            } else if (_transformTarget == TransformTarget::StarMan) {
                applyStarManState(10.0f);
            } else if (_transformTarget == TransformTarget::Super) {
                changeToSuperState();
            } else if (_transformTarget == TransformTarget::Mega) {
                applyMegaState(megaStateDurationSeconds);
            } else if (_transformTarget == TransformTarget::MegaEnd
                       || _transformTarget == TransformTarget::StarManEnd) {
                if (_stateAfterTransformation) {
                    setState(std::move(_stateAfterTransformation));
                } else {
                    changeToNormalState();
                }
            } else if (_transformTarget == TransformTarget::None) {
                if (_state) {
                    sf::Texture& tex = ResourceManager::getInstance().getTexture(_state->getTextureAlias());
                    if (auto* animatable = getBehaviour<Animatable>()) {
                        animatable->configureVisuals(tex, _state->getAnimationSetId());
                        animatable->playAnimation("idle", true);
                    }
                }
            }
            return;
        }

        // Lerp from the pre-transformation scale to target scale
        float targetScale = 1.5f;
        if (_transformTarget == TransformTarget::MegaEnd
            || _transformTarget == TransformTarget::StarManEnd) {
            targetScale = _transformEndScale;
        } else if (_transformTarget == TransformTarget::StarMan) {
            targetScale = _transformStartScale;
        } else if (_transformTarget == TransformTarget::Mega) {
            targetScale = MegaStateDecorator::scaleMultiplier;
        }
        float progress = 1.0f - (_transformTimer / _transformDuration);
        progress = std::max(0.0f, std::min(1.0f, progress));
        float currentScale = _transformStartScale + (targetScale - _transformStartScale) * progress;

        sf::Vector2f scaledHitbox = {_baseHitboxPixels.x * currentScale, _baseHitboxPixels.y * currentScale};
        updateHitboxSize(scaledHitbox);
        if (auto* animatable = getBehaviour<Animatable>()) {
            const sf::Vector2f scale =
                animatable->getActiveAnimationName() == "shoot"
                ? sf::Vector2f{3.75f, 1.1f}
                : sf::Vector2f{
                    Player::defaultVisualScaleX,
                    Player::defaultVisualScaleY
                };
            animatable->setVisualScale(scale);
            animatable->updateVisualState(deltaTime, scaledHitbox, facingLeft);
        }
        return;
    }

    sf::Vector2f scaleMult{1.0f, 1.0f};
    if (_state) {
        _state->update(*this, deltaTime);
        // State replacement is deferred until update() returns, so a decorator
        // never destroys itself while one of its member functions is active.
        if (_state->isExpired()) {
            if (isMegaState()) {
                beginMegaEndTransformation();
                return;
            } else if (isStarManState()) {
                beginStarManEndTransformation();
                return;
            }
            revertDecoratedState();
        }
        if (_state) {
            scaleMult = _state->getScaleMultiplier();
        }
    }

    sf::Vector2f scaledHitbox = {_baseHitboxPixels.x * scaleMult.x, _baseHitboxPixels.y * scaleMult.y};
    updateHitboxSize(scaledHitbox);
    if (auto* animatable = getBehaviour<Animatable>()) {
        const sf::Vector2f scale =
            animatable->getActiveAnimationName() == "shoot"
            ? sf::Vector2f{3.75f, 1.1f}
            : sf::Vector2f{
                Player::defaultVisualScaleX,
                Player::defaultVisualScaleY
            };
        animatable->setVisualScale(scale);
        animatable->updateVisualState(deltaTime, scaledHitbox, facingLeft);
    }

    // Sparkles belong only to StarMan; Mega uses its dedicated glow shader.
    if (isStarManState() && hasValidBody()) {
        sf::Vector2f pos = getPosition();
        _starSparkle.update(deltaTime, pos, {scaledHitbox.x * 0.5f, scaledHitbox.y * 0.5f});
    }
}

void Player::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    (void)angleDegrees;

    // Select the appropriate shader effect for the current visual state
    sf::Shader* activeShader = nullptr;

    
    
    if (_isTransforming) {
        activeShader = PlayerShaders::getInstance().getBlinkShader();
    } else if (isMegaState()) {
        activeShader = PlayerShaders::getInstance().getMegaGlowShader();
    } else if (_state && _state->isInvincible()) {
        activeShader = PlayerShaders::getInstance().getRainbowShader();
    }
    else if (auto* tempInvincible = getBehaviour<Invincible>()) {
        activeShader = PlayerShaders::getInstance().getGhostShader();
    }

    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->renderVisualState(target, position, 0.0f, activeShader);
    }

    // Overlay sparkle particles during StarMan invincibility only.
    if (isStarManState()) {
        _starSparkle.render(target);
    }

    // Render character name and pointing triangle above player's head (e.g. "MARIO", "LUIGI")
    // Gated: CharacterSelect hover (unified hover+keyboard via characterSelectHovered) and Coop both; otherwise hidden
    {
        auto &gs = GameSettings::getInstance();
        bool shouldDrawTag = false;
        if (!_isDying && !isPipeWarping()) {
            if (gs.isCharacterSelectActive) {
                if (gs.gameMode == GameMode::Coop) {
                    shouldDrawTag = true; // Coop: both
                } else {
                    shouldDrawTag = (getCharacter() == gs.characterSelectHovered);
                }
            } else if (gs.isLevelSelectActive) {
                if (gs.gameMode == GameMode::Coop) {
                    shouldDrawTag = true; // Coop: both
                } else {
                    shouldDrawTag = (getCharacter() == gs.characterSelectHovered);
                }
            }
        }
        if (shouldDrawTag) {
            std::string displayName = _character;
            for (char& c : displayName) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }

            sf::Color markerColor;
            if (_character == "luigi") {
                markerColor = sf::Color(60, 255, 100);
            } else if (_character == "mario") {
                markerColor = sf::Color(255, 65, 65);
            } else {
                markerColor = sf::Color(255, 235, 70);
            }

            const sf::Vector2f hitbox = getHitboxPixels();
            const float topY = position.y - hitbox.y * 0.5f - 4.0f;

            sf::ConvexShape pointer(3);
            pointer.setPoint(0, sf::Vector2f(-6.0f, -8.0f));
            pointer.setPoint(1, sf::Vector2f(6.0f, -8.0f));
            pointer.setPoint(2, sf::Vector2f(0.0f,  0.0f));
            pointer.setPosition({position.x, topY});
            pointer.setFillColor(markerColor);
            pointer.setOutlineColor(sf::Color::Black);
            pointer.setOutlineThickness(1.5f);

            const sf::Font& font = ResourceManager::getInstance().getFont("SuperMario");
            sf::Text nameText(font, displayName, 24);
            nameText.setFillColor(markerColor);
            nameText.setOutlineColor(sf::Color::Black);
            nameText.setOutlineThickness(2.0f);

            const sf::FloatRect bounds = nameText.getLocalBounds();
            nameText.setOrigin({bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y});
            nameText.setPosition({position.x, topY - 10.0f});

            target.draw(pointer);
            target.draw(nameText);
        }
    }
}

void Player::onHitboxRecreated() {
    if (auto* moveable = getBehaviour<Moveable>()) {
        moveable->resetGroundContacts();
    }

    if (auto* invincible = getBehaviour<Invincible>()) {
        invincible->refreshCollisionMask();
    }
}

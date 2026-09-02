#pragma once

#include <box2d/box2d.h>
#include <SFML/System.hpp>
#include <string>
#include <memory>

#include "Game/Behaviours/Moveable.h"
#include "Game/Behaviours/SparkleEffect.h"
#include "Game/Behaviours/ShellHoldBehaviour.h"
#include "Game/Objects/GameObject.h"
#include "Game/Objects/Player/State/PlayerState.h"

class GameWorld;
class Block;
class Enemy;
class Item;
class KoopaShell;
class Pipe;
enum class ScoreEventType;

struct PlayerMovementStats {
    float topSpeedMetersPerSecond = 0.0f;
    float accelerationMetersPerSecondSquared = 0.0f;
    float tractionMetersPerSecondSquared = 0.0f;
    float jumpSpeedMetersPerSecond = 0.0f;
};

class Player: public GameObject {
public:
    // The normal player sprite is intentionally presented larger than its
    // collision body. The map editor uses these values for an accurate
    // preview of the in-game player.
    static constexpr float defaultVisualScaleX = 2.5f;
    static constexpr float defaultVisualScaleY = 1.1f;

    Player();
    Player(sf::Texture &texture);
    Player(sf::Texture &texture, const std::string& animationSetId);
    ~Player() override;
    void destroy() override;

    void setState(std::unique_ptr<PlayerState> newState);
    void changeToNormalState();
    void changeToSuperState();
    void changeToFireState();
    static constexpr float megaStateDurationSeconds = 16.0f;
    void applyMegaState(float durationSeconds = megaStateDurationSeconds);
    void applyStarManState(float durationSeconds = 10.0f);
    void revertDecoratedState();

    enum class TransformTarget {
        Normal,
        Super,
        Fire,
        Mega,
        MegaEnd,
        StarMan,
        StarManEnd,
        None
    };

    void attack(GameWorld& world);
    void startTransformation(TransformTarget target, GameWorld& world, float duration = 1.0f);
    void startTransformation(TransformTarget target, float duration = 1.0f);

    static constexpr float pipeWarpDurationSeconds = 1.80f;
    bool beginPipeWarp(
        sf::Vector2f sourceOutside,
        sf::Vector2f sourceInside,
        sf::Vector2f targetInside,
        sf::Vector2f targetOutside
    );
    bool isPipeWarping() const noexcept { return _isPipeWarping; }

    void setGameWorld(GameWorld& world) { _world = &world; }
    GameWorld* getGameWorld() { return _world; }
    const std::string& getCharacter() const { return _character; }
    void refreshStatePresentation();
    std::string getBaseStateNameForSave() const;
    float getMegaStateTimeRemaining() const noexcept;
    float getStarManStateTimeRemaining() const noexcept;
    void restoreSavedState(
        const std::string& baseStateName,
        float megaTimeRemaining,
        float starManTimeRemaining
    );
    void onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) override;
    void finalizeGroundContacts() override;
    bool hasFallenFromHighPlace() const noexcept;
    /// Returns true while the player is under Mega Mushroom effect.
    bool isMegaState() const noexcept;
    /// Returns true while the player has the temporary Star Man effect.
    bool isStarManState() const noexcept;

    PlayerState* getState() const { return _state.get(); }
    bool isTransforming() const { return _isTransforming; }
    bool isEliminated() const noexcept {
        return _pendingDestroy || _isDying;
    }
    PlayerMovementStats getMovementStats() const noexcept;

    // Moveable forwarding (called externally)
    bool isFacingLeft() const {
        if (const auto* moveable = getBehaviour<Moveable>()) {
            return moveable->isFacingLeft();
        }
        return false;
    }
    void setFacingLeft(bool facingLeft) {
        if (auto* moveable = getBehaviour<Moveable>()) {
            moveable->setFacingLeft(facingLeft);
        }
    }
    bool isFlyMode() const noexcept { return _flyMode; }
    void setFlyMode(bool enabled) noexcept { _flyMode = enabled; }
    void startMoveLeft() {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->startMoveLeft();
    }
    void startMoveRight() {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->startMoveRight();
    }
    void startJump() {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->startJump();
    }
    void stopMoveLeft() {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->stopMoveLeft();
    }
    void stopMoveRight() {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->stopMoveRight();
    }
    void stopJump() {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->stopJump();
    }
    void setInteractHeld(bool held) {
        _flyMode = held && _flyMode ? false : _flyMode; // if holding interact isn't needed here, just add toggle
        _interactHeld = held;
        if (auto* hold = getBehaviour<ShellHoldBehaviour>()) hold->setInteractHeld(held);
    }
    void toggleFlyMode() { _flyMode = !_flyMode; }
    void setMoveDownHeld(bool held) { _moveDownHeld = held; }
    void setMoveUpHeld(bool held) { _moveUpHeld = held; }
    bool isMoveDownHeld() const { return _moveDownHeld; }
    bool isMoveUpHeld() const { return _moveUpHeld; }
    void beginGroundContact(b2ShapeId visitor) {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->beginGroundContact(visitor);
    }
    void endGroundContact(b2ShapeId visitor) {
        if (auto* moveable = getBehaviour<Moveable>()) moveable->endGroundContact(visitor);
    }

protected:
    void updateSimulation(const float &fixedDt) override;
    void finalizeSimulation(const float &fixedDt) override;
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onUpdateVisuals(float deltaTime) override;
    void onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) override;
    void onHitboxRecreated() override;
    b2Polygon makeHitbox(sf::Vector2f hitboxPixels) const override;

private:
    void handleItemContact(Item& item);
    void handleShellContact(
        KoopaShell& shell,
        const b2ContactData& contactData,
        b2ShapeId ownShape
    );
    void handleEnemyContact(
        Enemy& enemy,
        const b2ContactData& contactData,
        b2ShapeId ownShape
    );
    void handleMegaEnvironmentContact(
        GameObject& other,
        const b2ContactData& contactData,
        b2ShapeId ownShape
    );
    void handlePlayerContact(
        Player& player,
        const b2ContactData& contactData,
        b2ShapeId ownShape
    );
    bool isTopContact(
        const b2ContactData& contactData,
        b2ShapeId ownShape
    ) const;
    void updateFallTracking();
    void updatePipeWarpVisuals(float deltaTime);
    void bounce(float verticalVelocity = -12.0f);
    void awardScore(ScoreEventType event, sf::Vector2f position);
    void beginMegaEndTransformation();
    void beginStarManEndTransformation();

    static constexpr float luigiTopSpeedRatio = 0.9f;
    static constexpr float luigiAccelerationRatio = 0.80f;
    static constexpr float luigiTractionRatio = 0.60f;
    static constexpr float luigiJumpSpeedRatio = 1.1f;

    float _baseMoveSpeed = 8.0f;
    float _baseAcceleration = 256.0f;
    float _baseTraction = 128.0f;
    float _baseJumpSpeed = 20.0f;
    std::unique_ptr<PlayerState> _state;
    std::unique_ptr<IAttackStrategy> _attackStrategy;
    std::string _character = "mario";

    bool _isDying = false;
    bool _isTransforming = false;
    float _transformTimer = 0.0f;
    float _transformDuration = 1.0f;
    float _transformStartScale = 1.0f;
    float _transformEndScale = 1.5f;
    TransformTarget _transformTarget = TransformTarget::Fire;
    std::unique_ptr<PlayerState> _stateAfterTransformation;
    SparkleEffect _starSparkle{30.0f, 0.5f};
    float _effectTime = 0.0f;
    GameWorld* _world = nullptr;
    bool _interactHeld = false;
    bool _moveDownHeld = false;
    bool _moveUpHeld = false;
    bool _flyMode = false;
    float _warpCooldown = 0.0f;
    bool _isPipeWarping = false;
    bool _pipeWarpExitSoundPlayed = false;
    float _pipeWarpTimer = 0.0f;
    float _pipeWarpIdleTimer = 0.0f;
    sf::Vector2f _pipeWarpSourceOutside{0.0f, 0.0f};
    sf::Vector2f _pipeWarpSourceInside{0.0f, 0.0f};
    sf::Vector2f _pipeWarpTargetInside{0.0f, 0.0f};
    sf::Vector2f _pipeWarpTargetOutside{0.0f, 0.0f};
    float _fallStartY = 0.0f;
    float _fallDistancePixels = 0.0f;
    float _maxDownwardVelocityPixelsPerSecond = 0.0f;
    bool _fallTrackingActive = false;
    float _footstepTimer = 0.0f;
    float _deathSoundElapsedSeconds = 0.0f;
    float _deathSoundDurationSeconds = 0.0f;
};

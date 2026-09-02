#include "Game/Objects/Enemy/ConcreteEnemy/PiranhaPlant.h"
#include "Game/Behaviours/Animatable.h"
#include "Physics/CollisionFilter.h"
#include <algorithm>
#include <cmath>
#include "box2d/box2d.h"

namespace {
constexpr float kTravelSpeedPixelsPerSecond = 72.0f;
constexpr float kHiddenDurationSeconds = 1.0f;
constexpr float kExposedDurationSeconds = 1.5f;
constexpr float kSineRetractionWindow = 1.00f;
}

PiranhaPlant::PiranhaPlant() : Enemy() {}

PiranhaPlant::PiranhaPlant(sf::Texture& texture, const std::string& animationSetId) : Enemy(texture, animationSetId) {
}

void PiranhaPlant::setPipeTravel(float hiddenYPixels, float emergedYPixels) {
    const sf::Vector2f currentPosition = getPosition();
    setPipeTravel(
        {currentPosition.x, hiddenYPixels},
        {currentPosition.x, emergedYPixels}
    );
}

void PiranhaPlant::setPipeTravel(
    sf::Vector2f hiddenPosition,
    sf::Vector2f emergedPosition
) {
    _hasPipeTravel = true;
    _hiddenPosition = hiddenPosition;
    _emergedPosition = emergedPosition;

    if (auto* animatable = getBehaviour<Animatable>()) {
        sf::Vector2f visualScale = animatable->getVisualScale();
        const float scaleMagnitude = std::abs(visualScale.y);
        visualScale.y = emergedPosition.y > hiddenPosition.y
            ? -scaleMagnitude
            : scaleMagnitude;
        animatable->setVisualScale(visualScale);
    }

    beginPhase(PipePhase::Hidden);
    setPosition(_hiddenPosition);
}

void PiranhaPlant::configureSineWave(
    float periodSeconds,
    float phaseOffset
) {
    _usesSineWave = periodSeconds > 0.0f;
    _wavePeriodSeconds = std::max(periodSeconds, 0.01f);
    _wavePhaseOffset = phaseOffset - std::floor(phaseOffset);
    _waveElapsedSeconds = 0.0f;
}

void PiranhaPlant::onCreateBodyDef(b2BodyDef& def) {
    Enemy::onCreateBodyDef(def);
    def.gravityScale = 0.0f;
}

void PiranhaPlant::onCreateShapeDef(b2ShapeDef& def) {
    Enemy::onCreateShapeDef(def);
    // The plant overlaps the pipe while hidden, so it must not collide with
    // environment shapes. It remains hazardous to players and projectiles.
    def.filter.maskBits = CollisionFilter::PLAYER | CollisionFilter::FIREBALL
        | CollisionFilter::SHELL;
}

void PiranhaPlant::updateSimulation(const float &fixedDt) {
    if (_isDying) {
        _deathTimer += fixedDt;
        if (_deathTimer >= 1.0f) {
            _pendingDestroy = true;
        }
        return;
    }

    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->playAnimation("bite");
    }

    b2Vec2 velocity = b2Body_GetLinearVelocity(_body->getId());
    velocity.x = 0.0f;
    velocity.y = 0.0f;

    if (!_hasPipeTravel) {
        b2Body_SetLinearVelocity(_body->getId(), velocity);
        return;
    }

    if (_usesSineWave) {
        _waveElapsedSeconds = std::fmod(
            _waveElapsedSeconds + fixedDt,
            _wavePeriodSeconds
        );
        const float angle = 2.0f * B2_PI
            * (_waveElapsedSeconds / _wavePeriodSeconds
               + _wavePhaseOffset);
        // Keep most of the wave hazardous and smoothly retract only near the
        // sine trough. With evenly spaced phases, this creates one traveling
        // safe opening instead of leaving half the hallway retracted.
        const float troughProgress = std::clamp(
            (std::sin(angle) + 1.0f) / kSineRetractionWindow,
            0.0f,
            1.0f
        );
        const float extension = troughProgress * troughProgress
            * (3.0f - 2.0f * troughProgress);
        setPosition({
            _hiddenPosition.x
                + (_emergedPosition.x - _hiddenPosition.x) * extension,
            _hiddenPosition.y
                + (_emergedPosition.y - _hiddenPosition.y) * extension
        });
        b2Body_SetLinearVelocity(_body->getId(), velocity);
        return;
    }

    if (_pipePhase == PipePhase::Hidden || _pipePhase == PipePhase::Exposed) {
        _phaseTimer -= fixedDt;
        if (_phaseTimer <= 0.0f) {
            beginPhase(_pipePhase == PipePhase::Hidden
                ? PipePhase::Rising
                : PipePhase::Retracting);
        }
    }

    const sf::Vector2f currentPosition = getPosition();
    sf::Vector2f targetPosition = currentPosition;
    const auto moveTowards = [](float current, float target, float distance) {
        if (std::abs(target - current) <= distance) {
            return target;
        }
        return current + (target > current ? distance : -distance);
    };
    if (_pipePhase == PipePhase::Rising) {
        const float distance = kTravelSpeedPixelsPerSecond * fixedDt;
        targetPosition.x = moveTowards(
            currentPosition.x,
            _emergedPosition.x,
            distance
        );
        targetPosition.y = moveTowards(
            currentPosition.y,
            _emergedPosition.y,
            distance
        );
        if (targetPosition == _emergedPosition) {
            beginPhase(PipePhase::Exposed);
        }
    } else if (_pipePhase == PipePhase::Retracting) {
        const float distance = kTravelSpeedPixelsPerSecond * fixedDt;
        targetPosition.x = moveTowards(
            currentPosition.x,
            _hiddenPosition.x,
            distance
        );
        targetPosition.y = moveTowards(
            currentPosition.y,
            _hiddenPosition.y,
            distance
        );
        if (targetPosition == _hiddenPosition) {
            beginPhase(PipePhase::Hidden);
        }
    }

    setPosition(targetPosition);
    b2Body_SetLinearVelocity(_body->getId(), velocity);
}

void PiranhaPlant::onStomp() {}

bool PiranhaPlant::canBeStomped() const {
    return false;
}

void PiranhaPlant::beginPhase(PipePhase phase) {
    _pipePhase = phase;
    if (phase == PipePhase::Hidden) {
        _phaseTimer = kHiddenDurationSeconds;
    } else if (phase == PipePhase::Exposed) {
        _phaseTimer = kExposedDurationSeconds;
    } else {
        _phaseTimer = 0.0f;
    }
}

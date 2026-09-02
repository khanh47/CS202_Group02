#include "Game/AI/HeuristicAiController.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Game/Objects/Player/Player.h"
#include "Game/World/GameWorld.h"

HeuristicAiController::HeuristicAiController(
    Player& self,
    Player& opponent,
    const GameWorld& world
) : _self(self),
    _opponent(opponent),
    _world(world) {}

void HeuristicAiController::fixedUpdate(float fixedDt) {
    if (_self.isEliminated() || _opponent.isEliminated()) {
        _self.stopMoveLeft();
        _self.stopMoveRight();
        _self.stopJump();
        return;
    }

    const AiAction action = decide(
        AiPlayerController::observe(_self, _opponent, _world),
        fixedDt
    );
    if (action.horizontal < 0) {
        _self.stopMoveRight();
        _self.startMoveLeft();
    } else if (action.horizontal > 0) {
        _self.stopMoveLeft();
        _self.startMoveRight();
    } else {
        _self.stopMoveLeft();
        _self.stopMoveRight();
    }

    if (action.jump) {
        _self.startJump();
    } else {
        _self.stopJump();
    }
}

bool HeuristicAiController::isPlayerEliminated() const {
    return _self.isEliminated();
}

float HeuristicAiController::stoppingDistance(
    float velocity,
    float traction
) {
    if (traction <= 0.0f) {
        return std::numeric_limits<float>::infinity();
    }
    return velocity * velocity / (2.0f * traction);
}

int HeuristicAiController::directionToward(float value) {
    if (value > 0.0f) return 1;
    if (value < 0.0f) return -1;
    return 0;
}

int HeuristicAiController::pdSteer(
    float positionError,
    float errorVelocity,
    float kp,
    float kd,
    int previousHorizontal
) {
    const float control = kp * positionError + kd * errorVelocity;
    if (control >= pdEngageThreshold) return 1;
    if (control <= -pdEngageThreshold) return -1;
    if (previousHorizontal > 0 && control >= pdReleaseThreshold) return 1;
    if (previousHorizontal < 0 && control <= -pdReleaseThreshold) return -1;
    return 0;
}

void HeuristicAiController::updateKinematicEstimates(
    const AiObservation& observation,
    float fixedDt
) {
    const float opponentVelocityX = observation.opponent.velocityX;
    const float relativeVelocityY =
        observation.opponent.velocityY - observation.self.velocityY;

    if (_hasPreviousObservation && fixedDt > 0.0f) {
        const float horizontalLimit =
            std::max(
                observation.opponent.acceleration,
                observation.opponent.traction
            );
        const float rawOpponentAccelerationX = std::clamp(
            (opponentVelocityX - _previousOpponentVelocityX) / fixedDt,
            -horizontalLimit,
            horizontalLimit
        );
        const float rawAccelerationY = std::clamp(
            (relativeVelocityY - _previousRelativeVelocityY) / fixedDt,
            -playerGravity * 2.0f,
            playerGravity * 2.0f
        );
        const float blend = std::clamp(
            fixedDt * accelerationFilterRate,
            0.0f,
            1.0f
        );
        _opponentAccelerationX +=
            (rawOpponentAccelerationX - _opponentAccelerationX) * blend;
        _relativeAccelerationY +=
            (rawAccelerationY - _relativeAccelerationY) * blend;
    }

    _previousOpponentVelocityX = opponentVelocityX;
    _previousRelativeVelocityY = relativeVelocityY;
    _hasPreviousObservation = true;
}

float HeuristicAiController::predictionTime(
    const AiObservation& observation
) const {
    const float relativeX = observation.opponent.x - observation.self.x;
    const float relativeVelocityX =
        observation.opponent.velocityX - observation.self.velocityX;
    const float reachableSpeed = std::max(
        1.0f,
        observation.self.topSpeed + std::abs(relativeVelocityX)
    );
    return std::clamp(
        std::abs(relativeX) / reachableSpeed,
        minimumPredictionTime,
        maximumPredictionTime
    );
}

float HeuristicAiController::projectedRelativeX(
    const AiObservation& observation,
    float seconds
) const {
    const float position = observation.opponent.x - observation.self.x;
    const float projectedVelocity = projectedOpponentVelocityX(
        observation,
        seconds
    );
    const float averageVelocity =
        (observation.opponent.velocityX + projectedVelocity) * 0.5f;
    return position + averageVelocity * seconds;
}

float HeuristicAiController::projectedOpponentVelocityX(
    const AiObservation& observation,
    float seconds
) const {
    return std::clamp(
        observation.opponent.velocityX + _opponentAccelerationX * seconds,
        -observation.opponent.topSpeed,
        observation.opponent.topSpeed
    );
}

float HeuristicAiController::projectedRelativeY(
    const AiObservation& observation,
    float seconds
) const {
    const float position = observation.opponent.y - observation.self.y;
    const float velocity =
        observation.opponent.velocityY - observation.self.velocityY;
    return position
        + velocity * seconds
        + 0.5f * _relativeAccelerationY * seconds * seconds;
}

AiAction HeuristicAiController::decide(
    const AiObservation& observation,
    float fixedDt
) {
    AiAction action;
    updateKinematicEstimates(observation, fixedDt);
    _evadeTimeRemaining = std::max(
        0.0f,
        _evadeTimeRemaining - fixedDt
    );

    const AiPlayerKinematics& self = observation.self;
    const AiPlayerKinematics& opponent = observation.opponent;
    const float relativeX = opponent.x - self.x;
    const float relativeY = opponent.y - self.y;
    const float relativeVelocityX = opponent.velocityX - self.velocityX;

    const float dangerMargin = std::max(
        0.0f,
        observation.arenaHalfWidth - self.halfWidth - edgeSafetyInset
    );
    const float recoveredMargin = std::max(
        0.0f,
        dangerMargin - edgeRecoveryDistance
    );
    const float rightEdgeDistance =
        observation.arenaHalfWidth - self.halfWidth - self.x;
    const float leftEdgeDistance =
        self.x + observation.arenaHalfWidth - self.halfWidth;
    const float edgeStoppingDistance = stoppingDistance(
        self.velocityX,
        self.traction
    );

    if (_edgeRecovery == EdgeRecovery::None) {
        if (self.velocityX > 0.0f
            && rightEdgeDistance <= edgeStoppingDistance + edgeSafetyInset) {
            _edgeRecovery = EdgeRecovery::MoveLeft;
        } else if (self.velocityX < 0.0f
                   && leftEdgeDistance <= edgeStoppingDistance + edgeSafetyInset) {
            _edgeRecovery = EdgeRecovery::MoveRight;
        } else if (self.x >= dangerMargin) {
            _edgeRecovery = EdgeRecovery::MoveLeft;
        } else if (self.x <= -dangerMargin) {
            _edgeRecovery = EdgeRecovery::MoveRight;
        }
    } else if (_edgeRecovery == EdgeRecovery::MoveLeft
               && self.x <= recoveredMargin
               && self.velocityX <= 0.0f) {
        _edgeRecovery = EdgeRecovery::None;
    } else if (_edgeRecovery == EdgeRecovery::MoveRight
               && self.x >= -recoveredMargin
               && self.velocityX >= 0.0f) {
        _edgeRecovery = EdgeRecovery::None;
    }

    if (_edgeRecovery != EdgeRecovery::None) {
        _attackPhase = AttackPhase::Approach;
        _attackBecameAirborne = false;
        _attackSawDescent = false;
        _attackHasVerticalClearance = false;
        _attackHorizontalLocked = false;
        _evadeTimeRemaining = 0.0f;
        action.horizontal =
            _edgeRecovery == EdgeRecovery::MoveLeft ? -1 : 1;
        _lastHorizontal = action.horizontal;
        return action;
    }

    const float defensiveRelativeX = projectedRelativeX(
        observation,
        defensivePredictionTime
    );
    const float defensiveRelativeY = projectedRelativeY(
        observation,
        defensivePredictionTime
    );
    const float defensiveHorizontalRange =
        self.halfWidth
        + opponent.halfWidth
        + stompThreatHorizontalPadding;
    const bool opponentAbove =
        relativeY < -stompThreatVerticalDistance
        || defensiveRelativeY < -stompThreatVerticalDistance;
    const bool opponentLaunchingNearby =
        opponent.velocityY < stompThreatRiseSpeed
        && relativeY < stompThreatLaunchHeightTolerance;
    const bool selfAboveOpponent = self.y < opponent.y;
    const bool stompThreat =
        !selfAboveOpponent
        && std::min(std::abs(relativeX), std::abs(defensiveRelativeX))
            < defensiveHorizontalRange
        && (opponentAbove || opponentLaunchingNearby);

    if (selfAboveOpponent) {
        _evadeTimeRemaining = 0.0f;
    }

    if (stompThreat) {
        const float threatDirection =
            std::abs(defensiveRelativeX) > launchPositionTolerance
                ? defensiveRelativeX
                : relativeX;
        const int towardThreat = directionToward(threatDirection);
        _evadeDirection = towardThreat != 0
            ? -towardThreat
            : (self.x >= 0.0f ? -1 : 1);
        _evadeTimeRemaining = evadeHoldSeconds;
    }

    if (_evadeTimeRemaining > 0.0f) {
        _attackPhase = AttackPhase::Approach;
        _attackBecameAirborne = false;
        _attackSawDescent = false;
        _attackHasVerticalClearance = false;
        _attackHorizontalLocked = false;
        action.horizontal = _evadeDirection;
        _lastHorizontal = action.horizontal;
        return action;
    }

    if (_attackPhase == AttackPhase::AirborneAttack) {
        if (!self.grounded
            || std::abs(self.velocityY) >= airborneVelocityThreshold) {
            _attackBecameAirborne = true;
        }
        if (self.velocityY >= descendingVelocityThreshold) {
            _attackSawDescent = true;
        }
        const bool hasLanded =
            _attackBecameAirborne
            && _attackSawDescent
            && self.grounded
            && std::abs(self.velocityY) < airborneVelocityThreshold;
        if (hasLanded) {
            _attackPhase = AttackPhase::Approach;
            _attackBecameAirborne = false;
            _attackSawDescent = false;
            _attackHasVerticalClearance = false;
            _attackHorizontalLocked = false;
        }

        if (_attackPhase == AttackPhase::AirborneAttack) {
            const float requiredVerticalSeparation =
                self.halfHeight
                + opponent.halfHeight
                + verticalClearancePadding;
            if (relativeY >= requiredVerticalSeparation) {
                _attackHasVerticalClearance = true;
            }
            const float predictionHorizon = _attackHasVerticalClearance
                ? airbornePredictionTime
                : minimumPredictionTime;
            const float predictedX = projectedRelativeX(
                observation,
                predictionHorizon
            );
            const float predictedVelocityX =
                projectedOpponentVelocityX(observation, predictionHorizon)
                - self.velocityX;
            const float stagingDistance =
                self.halfWidth
                + opponent.halfWidth
                + ascentStagingPadding;
            const float targetError = _attackHasVerticalClearance
                ? predictedX
                : predictedX
                    - static_cast<float>(_attackDirection) * stagingDistance;
            if (_attackHasVerticalClearance) {
                if (_attackHorizontalLocked
                    && std::abs(predictedX) > horizontalReleaseRadius) {
                    _attackHorizontalLocked = false;
                } else if (!_attackHorizontalLocked
                           && std::abs(predictedX) <= horizontalCaptureRadius) {
                    _attackHorizontalLocked = true;
                }
            }
            action.horizontal = _attackHorizontalLocked
                ? 0
                : pdSteer(
                    targetError,
                    predictedVelocityX,
                    airborneKp,
                    airborneKd,
                    _lastHorizontal
                );
            action.jump = self.grounded && !_attackBecameAirborne;
            if (action.horizontal != 0) {
                _lastHorizontal = action.horizontal;
            }
            return action;
        }
    }

    const float horizon = predictionTime(observation);
    const float predictedX = projectedRelativeX(observation, horizon);
    int attackDirection = directionToward(predictedX);
    if (attackDirection == 0) {
        attackDirection = directionToward(relativeX);
    }
    if (attackDirection == 0) {
        attackDirection = -_lastHorizontal;
    }

    const float closingSpeed = std::max(
        0.0f,
        -relativeVelocityX * static_cast<float>(attackDirection)
    );
    const float horizontalStoppingDistance = stoppingDistance(
        closingSpeed,
        self.traction
    );
    const float stagingDistance =
        self.halfWidth
        + opponent.halfWidth
        + ascentStagingPadding;
    const float stagingError =
        predictedX - static_cast<float>(attackDirection) * stagingDistance;
    const float predictedVelocityX =
        projectedOpponentVelocityX(observation, horizon) - self.velocityX;
    const bool inLaunchPosition =
        std::abs(stagingError) <= launchPositionTolerance;
    const bool horizontallySeparated =
        std::abs(relativeX)
        >= self.halfWidth
            + opponent.halfWidth
            + launchSeparationPadding
            + horizontalStoppingDistance;

    if (self.grounded && inLaunchPosition && horizontallySeparated) {
        _attackPhase = AttackPhase::AirborneAttack;
        _attackBecameAirborne = false;
        _attackSawDescent = false;
        _attackHasVerticalClearance = false;
        _attackHorizontalLocked = false;
        _attackDirection = attackDirection;
        action.horizontal = pdSteer(
            stagingError,
            predictedVelocityX,
            approachKp,
            approachKd,
            _lastHorizontal
        );
        action.jump = true;
    } else {
        action.horizontal = pdSteer(
            stagingError,
            predictedVelocityX,
            approachKp,
            approachKd,
            _lastHorizontal
        );
    }

    if (action.horizontal != 0) {
        _lastHorizontal = action.horizontal;
    }
    return action;
}

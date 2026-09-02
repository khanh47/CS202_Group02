#pragma once

#include "Game/AI/AiPlayerController.h"

class GameWorld;
class Player;

class HeuristicAiController {
public:
    HeuristicAiController(
        Player& self,
        Player& opponent,
        const GameWorld& world
    );

    void fixedUpdate(float fixedDt);
    bool isPlayerEliminated() const;

    AiAction decide(const AiObservation& observation, float fixedDt);

private:
    enum class EdgeRecovery {
        None,
        MoveLeft,
        MoveRight
    };

    enum class AttackPhase {
        Approach,
        AirborneAttack
    };

    static constexpr float playerGravity = 9.8f * 4.0f * 64.0f;
    static constexpr float minimumPredictionTime = 0.10f;
    static constexpr float maximumPredictionTime = 0.45f;
    static constexpr float accelerationFilterRate = 12.0f;
    static constexpr float approachKp = 8.0f;
    static constexpr float approachKd = 0.65f;
    static constexpr float airborneKp = 12.0f;
    static constexpr float airborneKd = 0.45f;
    static constexpr float pdEngageThreshold = 96.0f;
    static constexpr float pdReleaseThreshold = 48.0f;
    static constexpr float ascentStagingPadding = 12.0f;
    static constexpr float launchPositionTolerance = 12.0f;
    static constexpr float launchSeparationPadding = 4.0f;
    static constexpr float verticalClearancePadding = 6.0f;
    static constexpr float airbornePredictionTime = 0.16f;
    static constexpr float horizontalCaptureRadius = 8.0f;
    static constexpr float horizontalReleaseRadius = 24.0f;
    static constexpr float airborneVelocityThreshold = 30.0f;
    static constexpr float descendingVelocityThreshold = 60.0f;
    static constexpr float stompThreatHorizontalPadding = 64.0f;
    static constexpr float stompThreatVerticalDistance = 16.0f;
    static constexpr float stompThreatLaunchHeightTolerance = 48.0f;
    static constexpr float stompThreatRiseSpeed = -80.0f;
    static constexpr float defensivePredictionTime = 0.32f;
    static constexpr float evadeHoldSeconds = 0.28f;
    static constexpr float edgeSafetyInset = 32.0f;
    static constexpr float edgeRecoveryDistance = 128.0f;

    static float stoppingDistance(float velocity, float traction);
    static int directionToward(float value);
    static int pdSteer(
        float positionError,
        float errorVelocity,
        float kp,
        float kd,
        int previousHorizontal
    );
    void updateKinematicEstimates(
        const AiObservation& observation,
        float fixedDt
    );
    float predictionTime(const AiObservation& observation) const;
    float projectedRelativeX(
        const AiObservation& observation,
        float seconds
    ) const;
    float projectedOpponentVelocityX(
        const AiObservation& observation,
        float seconds
    ) const;
    float projectedRelativeY(
        const AiObservation& observation,
        float seconds
    ) const;

    Player& _self;
    Player& _opponent;
    const GameWorld& _world;
    EdgeRecovery _edgeRecovery = EdgeRecovery::None;
    AttackPhase _attackPhase = AttackPhase::Approach;
    bool _attackBecameAirborne = false;
    bool _attackSawDescent = false;
    bool _attackHasVerticalClearance = false;
    bool _attackHorizontalLocked = false;
    int _attackDirection = 1;
    float _previousOpponentVelocityX = 0.0f;
    float _previousRelativeVelocityY = 0.0f;
    float _opponentAccelerationX = 0.0f;
    float _relativeAccelerationY = 0.0f;
    bool _hasPreviousObservation = false;
    float _evadeTimeRemaining = 0.0f;
    int _evadeDirection = -1;
    int _lastHorizontal = -1;
};

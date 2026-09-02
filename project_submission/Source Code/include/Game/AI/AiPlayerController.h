#pragma once

class GameWorld;
class Player;

struct AiAction {
    int horizontal = 0;
    bool jump = false;
};

struct AiPlayerKinematics {
    float x = 0.0f;
    float y = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float topSpeed = 0.0f;
    float acceleration = 0.0f;
    float traction = 0.0f;
    float jumpSpeed = 0.0f;
    float halfWidth = 0.0f;
    float halfHeight = 0.0f;
    bool grounded = false;
};

struct AiObservation {
    AiPlayerKinematics self;
    AiPlayerKinematics opponent;
    float arenaHalfWidth = 0.0f;
};

class AiPlayerController {
public:
    static AiObservation observe(
        const Player& self,
        const Player& opponent,
        const GameWorld& world
    );
};

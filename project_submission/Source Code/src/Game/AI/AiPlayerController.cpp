#include "Game/AI/AiPlayerController.h"

#include "Game/Behaviours/Moveable.h"
#include "Game/Objects/Player/Player.h"
#include "Game/World/GameWorld.h"
#include "Physics/PhysicsUnits.h"

namespace {
AiPlayerKinematics observePlayer(
    const Player& player,
    float arenaCenterX
) {
    const sf::Vector2f position = player.getPosition();
    const sf::Vector2f velocity = player.getVelocity();
    const sf::Vector2f hitbox = player.getHitboxPixels();
    const PlayerMovementStats movement = player.getMovementStats();
    const Moveable* moveable = player.getBehaviour<Moveable>();

    return {
        position.x - arenaCenterX,
        position.y,
        velocity.x,
        velocity.y,
        PhysicsUnits::toPixels(movement.topSpeedMetersPerSecond),
        PhysicsUnits::toPixels(
            movement.accelerationMetersPerSecondSquared
        ),
        PhysicsUnits::toPixels(
            movement.tractionMetersPerSecondSquared
        ),
        PhysicsUnits::toPixels(movement.jumpSpeedMetersPerSecond),
        hitbox.x * 0.5f,
        hitbox.y * 0.5f,
        moveable && !moveable->isAirbone()
    };
}
}

AiObservation AiPlayerController::observe(
    const Player& self,
    const Player& opponent,
    const GameWorld& world
) {
    const sf::FloatRect bounds = world.getBounds();
    const float arenaCenterX = bounds.position.x + bounds.size.x * 0.5f;
    return {
        observePlayer(self, arenaCenterX),
        observePlayer(opponent, arenaCenterX),
        bounds.size.x * 0.5f
    };
}

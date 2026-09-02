#pragma once

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

#include "Game/Objects/GameObject.h"

class GameWorld; // Forward declaration — full include is in KoopaShell.cpp

/**
 * @brief A kickable shell left behind when a Koopa is stomped.
 * Slides at constant speed when kicked, kills enemies on contact, bounces
 * off walls, and can hurt the player from the side.
 */
class KoopaShell : public GameObject {
public:
    KoopaShell(sf::Texture& texture);
    ~KoopaShell() override = default;

    void kick(bool facingRight);
    void stop();
    bool isSliding() const { return _sliding; }
    void setFacingRight(bool facingRight) { _facingRight = facingRight; }

    void setHeld(bool held);
    bool isHeld() const { return _held; }
    bool isDying() const { return _isDying; }
    void resetReviveTimer() { _stopTimer = 0.0f; }

    void updateSimulation(const float& fixedDt) override;
    void onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) override;

    void setGameWorld(GameWorld* world) { _world = world; }
    void destroy() override;

protected:
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onUpdateVisuals(float deltaTime) override;
    void onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) override;

private:
    bool _sliding = false;
    bool _held = false;
    float _slideSpeedMeters = 8.0f;
    bool _facingRight = true;

    bool _isDying = false;
    float _deathTimer = 0.0f;
    float _stopTimer = 0.0f;

    b2Filter _savedFilter = {};
    bool _hasSavedFilter = false;

    GameWorld* _world = nullptr; // Pointer to the game world for score management
};

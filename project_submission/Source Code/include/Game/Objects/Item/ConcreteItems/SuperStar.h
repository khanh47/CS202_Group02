#pragma once

#include "Game/Behaviours/SparkleEffect.h"
#include "../Item.h"

class Player;

/**
 * @brief Bouncing Super Star pickup item.
 * Moves horizontally, bounces off terrain via Box2D restitution.
 * Sparkles with rainbow glitter while airborne.
 * On player contact, grants the StarMan invincibility state.
 */
class SuperStar : public Item {
public:
    SuperStar();
    SuperStar(sf::Texture& texture);
    ~SuperStar() override = default;

protected:
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void updateSimulation(const float& fixedDt) override;
    void onUpdateVisuals(float deltaTime) override;
    void onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) override;

private:
    SparkleEffect _sparkle{25.0f, 0.4f};
    bool _movingRight = true;
    static constexpr float _speed = 3.0f;
};

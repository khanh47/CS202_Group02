#pragma once

#include <SFML/System.hpp>

#include "Game/Objects/GameObject.h"

class Item : public GameObject {
public:
    Item();
    Item(sf::Texture& texture);
    ~Item();

    // Starts the item inside a block and moves it to the target position.
    void startEmerging(sf::Vector2f targetPosition, float durationSeconds = 0.4f);
    void setEmergenceRenderOffset(sf::Vector2f offset) noexcept {
        _emergenceRenderOffset = offset;
    }
    bool isEmerging() const noexcept { return _isEmerging; }

    void updateSimulation(const float& fixedDt) override;
    void finalizeSimulation(const float& fixedDt) override;

protected:
    void onUpdateVisuals(float deltaTime) override;
    void onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) override;

private:
    void updateEmergence(float fixedDt);

    bool _isEmerging = false;
    float _emergenceElapsed = 0.0f;
    float _emergenceDuration = 0.0f;
    sf::Vector2f _emergenceStartPosition{};
    sf::Vector2f _emergenceTargetPosition{};
    sf::Vector2f _emergencePosition{};
    sf::Vector2f _emergenceRenderOffset{};
};

#pragma once
#include <box2d/box2d.h>
#include <SFML/System.hpp>

namespace PhysicsUnits
{
    constexpr float pixelsPerMeter = 64.0f;

    inline float toMeters(float pixels) {
        return pixels / pixelsPerMeter;
    }

    inline float toPixels(float meters) {
        return meters * pixelsPerMeter;
    }

    inline b2Vec2 toMeters(sf::Vector2f pixels) {
        return b2Vec2{toMeters(pixels.x), toMeters(pixels.y)};
    }

    inline sf::Vector2f toPixels(b2Vec2 meters) {
        return sf::Vector2f(toPixels(meters.x), toPixels(meters.y));
    }
}
#pragma once

#include <SFML/Graphics.hpp>

struct AnimationFrame {
    AnimationFrame(sf::IntRect _rect, float _duration) : rect(_rect), duration(_duration) {}

    sf::IntRect rect;
    float duration;
};
#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

class BlockBreakEffect {
public:
    void spawn(
        sf::Vector2f position,
        sf::Vector2f blockSize,
        const sf::Texture* texture,
        sf::IntRect textureRect
    );

    void update(float deltaTime);
    void render(sf::RenderTarget& target) const;

    bool isFinished() const noexcept { return !_active; }

private:
    struct Fragment {
        std::optional<sf::Sprite> sprite;
        sf::RectangleShape fallback;
        sf::Vector2f position{0.0f, 0.0f};
        sf::Vector2f velocity{0.0f, 0.0f};
        float rotationDegrees = 0.0f;
        float angularVelocity = 0.0f;
    };

    static constexpr float lifetimeSeconds = 0.85f;
    static constexpr float gravityPixelsPerSecondSquared = 900.0f;

    std::array<Fragment, 4> _fragments;
    float _age = 0.0f;
    bool _active = false;
};

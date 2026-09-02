#include "Game/World/BlockBreakEffect.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {
sf::Color withAlpha(sf::Color color, std::uint8_t alpha) {
    color.a = alpha;
    return color;
}
}

void BlockBreakEffect::spawn(
    sf::Vector2f position,
    sf::Vector2f blockSize,
    const sf::Texture* texture,
    sf::IntRect textureRect
) {
    _age = 0.0f;
    _active = true;

    const sf::Vector2f safeBlockSize{
        std::max(blockSize.x, 1.0f),
        std::max(blockSize.y, 1.0f)
    };

    if (texture
        && (textureRect.size.x <= 0 || textureRect.size.y <= 0)) {
        const sf::Vector2u textureSize = texture->getSize();
        textureRect = sf::IntRect(
            {0, 0},
            {
                static_cast<int>(textureSize.x),
                static_cast<int>(textureSize.y)
            }
        );
    }

    const bool hasTexturePieces = texture
        && textureRect.size.x >= 2
        && textureRect.size.y >= 2;
    const int leftWidth = textureRect.size.x / 2;
    const int topHeight = textureRect.size.y / 2;
    const int rightWidth = textureRect.size.x - leftWidth;
    const int bottomHeight = textureRect.size.y - topHeight;

    const std::array<sf::Vector2f, 4> velocities = {{
        {-180.0f, -360.0f},
        {180.0f, -330.0f},
        {-150.0f, -280.0f},
        {150.0f, -310.0f}
    }};
    const std::array<float, 4> angularVelocities = {
        -230.0f, 230.0f, -300.0f, 280.0f
    };

    const sf::Color fallbackColor(177, 91, 42);
    const sf::Color fallbackOutline(95, 43, 21);

    for (std::size_t index = 0; index < _fragments.size(); ++index) {
        Fragment& fragment = _fragments[index];
        const bool onRight = index % 2 == 1;
        const bool onBottom = index >= 2;
        const float xSign = onRight ? 1.0f : -1.0f;
        const float ySign = onBottom ? 1.0f : -1.0f;

        fragment.position = position + sf::Vector2f{
            xSign * safeBlockSize.x * 0.25f,
            ySign * safeBlockSize.y * 0.25f
        };
        fragment.velocity = velocities[index];
        fragment.rotationDegrees = 0.0f;
        fragment.angularVelocity = angularVelocities[index];
        fragment.sprite.reset();

        const sf::Vector2f pieceSize{
            safeBlockSize.x * 0.5f,
            safeBlockSize.y * 0.5f
        };
        fragment.fallback = sf::RectangleShape(pieceSize);
        fragment.fallback.setOrigin({pieceSize.x * 0.5f, pieceSize.y * 0.5f});
        fragment.fallback.setPosition(fragment.position);
        fragment.fallback.setFillColor(fallbackColor);
        fragment.fallback.setOutlineColor(fallbackOutline);
        fragment.fallback.setOutlineThickness(1.0f);

        if (hasTexturePieces) {
            const int x = textureRect.position.x
                + (onRight ? leftWidth : 0);
            const int y = textureRect.position.y
                + (onBottom ? topHeight : 0);
            const int width = onRight ? rightWidth : leftWidth;
            const int height = onBottom ? bottomHeight : topHeight;
            const sf::IntRect pieceRect({x, y}, {width, height});

            fragment.sprite.emplace(*texture, pieceRect);
            fragment.sprite->setOrigin({
                static_cast<float>(width) * 0.5f,
                static_cast<float>(height) * 0.5f
            });
            fragment.sprite->setScale({
                safeBlockSize.x / static_cast<float>(textureRect.size.x),
                safeBlockSize.y / static_cast<float>(textureRect.size.y)
            });
            fragment.sprite->setPosition(fragment.position);
        }
    }
}

void BlockBreakEffect::update(float deltaTime) {
    if (!_active) {
        return;
    }

    const float dt = std::max(deltaTime, 0.0f);
    _age += dt;

    for (Fragment& fragment : _fragments) {
        fragment.velocity.y += gravityPixelsPerSecondSquared * dt;
        fragment.position += fragment.velocity * dt;
        fragment.rotationDegrees += fragment.angularVelocity * dt;

        if (fragment.sprite) {
            fragment.sprite->setPosition(fragment.position);
            fragment.sprite->setRotation(sf::degrees(fragment.rotationDegrees));
            const float lifeRatio = std::clamp(
                1.0f - (_age / lifetimeSeconds),
                0.0f,
                1.0f
            );
            fragment.sprite->setColor(withAlpha(
                sf::Color::White,
                static_cast<std::uint8_t>(255.0f * lifeRatio)
            ));
        } else {
            fragment.fallback.setPosition(fragment.position);
            fragment.fallback.setRotation(sf::degrees(fragment.rotationDegrees));
            const float lifeRatio = std::clamp(
                1.0f - (_age / lifetimeSeconds),
                0.0f,
                1.0f
            );
            const std::uint8_t alpha = static_cast<std::uint8_t>(255.0f * lifeRatio);
            fragment.fallback.setFillColor(withAlpha(
                sf::Color(177, 91, 42),
                alpha
            ));
            fragment.fallback.setOutlineColor(withAlpha(
                sf::Color(95, 43, 21),
                alpha
            ));
        }
    }

    if (_age >= lifetimeSeconds) {
        _active = false;
    }
}

void BlockBreakEffect::render(sf::RenderTarget& target) const {
    if (!_active) {
        return;
    }

    for (const Fragment& fragment : _fragments) {
        if (fragment.sprite) {
            target.draw(*fragment.sprite);
        } else {
            target.draw(fragment.fallback);
        }
    }
}

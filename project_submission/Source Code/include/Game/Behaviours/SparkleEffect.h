#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <random>

/**
 * @brief Lightweight particle system emitting small rainbow-colored sparkle dots.
 * Designed to be composable — attach to any entity that needs a glitter trail
 * (e.g. Super Star item, StarMan player state).
 */
class SparkleEffect {
public:
    explicit SparkleEffect(float emitRate = 20.0f, float particleLifetime = 0.5f);

    void update(float dt, sf::Vector2f center, sf::Vector2f halfExtents);
    void render(sf::RenderTarget& target) const;

private:
    struct Particle {
        sf::Vector2f position;
        sf::Vector2f velocity;
        sf::Color color;
        float lifetime;
        float maxLifetime;
    };

    void emitParticle(sf::Vector2f center, sf::Vector2f halfExtents);
    static sf::Color hueToColor(float hue);

    float _emitRate;
    float _particleLife;
    float _emitAccumulator = 0.0f;
    float _hueOffset = 0.0f;
    std::vector<Particle> _particles;
    std::mt19937 _rng;
};

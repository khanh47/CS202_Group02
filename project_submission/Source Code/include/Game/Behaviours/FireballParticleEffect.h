#pragma once

#include <SFML/Graphics.hpp>

#include <random>
#include <vector>

/**
 * @brief Small fiery trail emitted by an active fireball.
 */
class FireballParticleEffect {
public:
    explicit FireballParticleEffect(
        float emitRate = 36.0f,
        float particleLifetime = 0.32f
    );

    void update(float deltaTime, sf::Vector2f center, bool movingRight);
    void render(sf::RenderTarget& target) const;
    void clear();

private:
    struct Particle {
        sf::Vector2f position{0.0f, 0.0f};
        sf::Vector2f velocity{0.0f, 0.0f};
        sf::Color color = sf::Color::White;
        float lifetime = 0.0f;
        float maxLifetime = 0.0f;
        float radius = 2.0f;
    };

    void emitParticle(sf::Vector2f center, bool movingRight);

    float _emitRate;
    float _particleLifetime;
    float _emitAccumulator = 0.0f;
    std::vector<Particle> _particles;
    std::mt19937 _rng;
};

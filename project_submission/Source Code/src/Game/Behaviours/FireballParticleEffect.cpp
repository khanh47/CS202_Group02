#include "Game/Behaviours/FireballParticleEffect.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace {
sf::Color withAlpha(sf::Color color, std::uint8_t alpha) {
    color.a = alpha;
    return color;
}
}

FireballParticleEffect::FireballParticleEffect(
    float emitRate,
    float particleLifetime
)
    : _emitRate(emitRate),
      _particleLifetime(particleLifetime),
      _rng(std::random_device{}()) {
    _particles.reserve(24);
}

void FireballParticleEffect::update(
    float deltaTime,
    sf::Vector2f center,
    bool movingRight
) {
    const float dt = std::max(deltaTime, 0.0f);

    for (Particle& particle : _particles) {
        particle.lifetime -= dt;
        particle.position += particle.velocity * dt;
        particle.velocity.y -= 18.0f * dt;
    }

    std::erase_if(
        _particles,
        [](const Particle& particle) { return particle.lifetime <= 0.0f; }
    );

    _emitAccumulator += dt * _emitRate;
    while (_emitAccumulator >= 1.0f) {
        _emitAccumulator -= 1.0f;
        emitParticle(center, movingRight);
    }
}

void FireballParticleEffect::render(sf::RenderTarget& target) const {
    for (const Particle& particle : _particles) {
        const float lifeRatio = std::clamp(
            particle.lifetime / particle.maxLifetime,
            0.0f,
            1.0f
        );
        const float radius = particle.radius * (0.55f + 0.45f * lifeRatio);
        sf::CircleShape puff(radius);
        puff.setOrigin({radius, radius});
        puff.setPosition(particle.position);
        puff.setFillColor(withAlpha(
            particle.color,
            static_cast<std::uint8_t>(255.0f * lifeRatio)
        ));
        target.draw(puff);
    }
}

void FireballParticleEffect::clear() {
    _particles.clear();
    _emitAccumulator = 0.0f;
}

void FireballParticleEffect::emitParticle(
    sf::Vector2f center,
    bool movingRight
) {
    static constexpr std::array<sf::Color, 3> palette = {{
        sf::Color(255, 245, 145),
        sf::Color(255, 166, 45),
        sf::Color(235, 55, 25)
    }};

    std::uniform_real_distribution<float> distance(5.0f, 14.0f);
    std::uniform_real_distribution<float> verticalOffset(-7.0f, 7.0f);
    std::uniform_real_distribution<float> velocityAlongTrail(8.0f, 34.0f);
    std::uniform_real_distribution<float> velocityY(-28.0f, 12.0f);
    std::uniform_real_distribution<float> lifetimeScale(0.7f, 1.15f);
    std::uniform_real_distribution<float> radius(1.5f, 3.5f);
    std::uniform_int_distribution<std::size_t> colorIndex(0, palette.size() - 1);

    const float trailDirection = movingRight ? -1.0f : 1.0f;
    Particle particle;
    particle.position = {
        center.x + trailDirection * distance(_rng),
        center.y + verticalOffset(_rng)
    };
    particle.velocity = {
        trailDirection * velocityAlongTrail(_rng),
        velocityY(_rng)
    };
    particle.color = palette[colorIndex(_rng)];
    particle.maxLifetime = _particleLifetime * lifetimeScale(_rng);
    particle.lifetime = particle.maxLifetime;
    particle.radius = radius(_rng);
    _particles.push_back(particle);
}

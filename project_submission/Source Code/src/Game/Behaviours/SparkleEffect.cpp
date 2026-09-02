#include "Game/Behaviours/SparkleEffect.h"

#include <algorithm>
#include <cmath>

SparkleEffect::SparkleEffect(float emitRate, float particleLifetime)
    : _emitRate(emitRate),
      _particleLife(particleLifetime),
      _rng(std::random_device{}()) {
}

void SparkleEffect::update(float dt, sf::Vector2f center, sf::Vector2f halfExtents) {
    // Advance hue for continuous rainbow cycling
    _hueOffset += dt * 120.0f;
    if (_hueOffset >= 360.0f) _hueOffset -= 360.0f;

    // Update existing particles
    for (auto& p : _particles) {
        p.lifetime -= dt;
        p.position += p.velocity * dt;
        // Gentle upward drift simulating rising glitter
        p.velocity.y -= 30.0f * dt;
    }

    // Remove expired particles
    std::erase_if(_particles, [](const Particle& p) { return p.lifetime <= 0.0f; });

    // Emit new particles based on rate
    _emitAccumulator += dt * _emitRate;
    while (_emitAccumulator >= 1.0f) {
        _emitAccumulator -= 1.0f;
        emitParticle(center, halfExtents);
    }
}

void SparkleEffect::render(sf::RenderTarget& target) const {
    for (const auto& p : _particles) {
        float lifeRatio = std::max(0.0f, p.lifetime / p.maxLifetime);
        // Fade out alpha over lifetime, with a slight size pulse
        auto alpha = static_cast<uint8_t>(255.0f * lifeRatio);
        float radius = 2.0f + 2.0f * lifeRatio;

        sf::CircleShape dot(radius);
        dot.setOrigin({radius, radius});
        dot.setPosition(p.position);
        dot.setFillColor(sf::Color(p.color.r, p.color.g, p.color.b, alpha));
        target.draw(dot);
    }
}

void SparkleEffect::emitParticle(sf::Vector2f center, sf::Vector2f halfExtents) {
    std::uniform_real_distribution<float> distX(-halfExtents.x, halfExtents.x);
    std::uniform_real_distribution<float> distY(-halfExtents.y, halfExtents.y);
    std::uniform_real_distribution<float> distVel(-40.0f, 40.0f);
    std::uniform_real_distribution<float> distHue(0.0f, 360.0f);
    std::uniform_real_distribution<float> distLife(0.6f, 1.0f);

    Particle p;
    p.position = {center.x + distX(_rng), center.y + distY(_rng)};
    p.velocity = {distVel(_rng), distVel(_rng) - 20.0f};
    p.color = hueToColor(std::fmod(_hueOffset + distHue(_rng), 360.0f));
    p.maxLifetime = _particleLife * distLife(_rng);
    p.lifetime = p.maxLifetime;

    _particles.push_back(p);
}

sf::Color SparkleEffect::hueToColor(float hue) {
    // HSL-to-RGB with full saturation and 60% lightness for vivid pastel sparkles
    float c = 0.8f;
    float h = hue / 60.0f;
    float x = c * (1.0f - std::abs(std::fmod(h, 2.0f) - 1.0f));

    float r = 0.0f, g = 0.0f, b = 0.0f;
    if (h < 1.0f)      { r = c; g = x; }
    else if (h < 2.0f)  { r = x; g = c; }
    else if (h < 3.0f)  { g = c; b = x; }
    else if (h < 4.0f)  { g = x; b = c; }
    else if (h < 5.0f)  { r = x; b = c; }
    else                 { r = c; b = x; }

    float m = 0.2f;
    return sf::Color(
        static_cast<uint8_t>((r + m) * 255),
        static_cast<uint8_t>((g + m) * 255),
        static_cast<uint8_t>((b + m) * 255),
        255
    );
}

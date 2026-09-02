#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Angle.hpp>
#include <memory>
#include <optional>
#include <string>

#include "Game/Behaviours/Behaviour.h"
#include "Animation/Animator.h"

class Animatable : public Behaviour {
public:
    Animatable() = default;
    Animatable(sf::Texture &texture);
    Animatable(sf::Texture &texture, const std::string& animationSetId);
    virtual ~Animatable() = default;

    void configureVisuals(sf::Texture& texture);
    void configureVisuals(sf::Texture& texture, const std::string& animationSetId);
    void setTextureRect(const sf::IntRect& rect);

    void updateVisualState(float deltaTime, const sf::Vector2f& hitboxPixels, bool facingLeft = false);
    void renderVisualState(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees = 0, sf::Shader* shader = nullptr) const;

    void setVisualScale(sf::Vector2f scale);
    sf::Vector2f getVisualScale() const;
    void playAnimation(const std::string& name, bool replay = false);
    void stopAnimation();
    std::string getActiveAnimationName() const;
    bool isAnimationDone() const;
    bool isLooping() const;
    bool hasSprite() const;
    const sf::Texture* getTexture() const noexcept { return _spritesheet.get(); }
    sf::IntRect getTextureRect() const {
        return _sprite ? _sprite->getTextureRect() : sf::IntRect{};
    }

private:
    void bindTexture(sf::Texture& texture);
    void syncSpriteLayout(const sf::Vector2f& hitboxPixels, bool facingLeft = false);

    std::shared_ptr<sf::Texture> _spritesheet;
    Animator _animator;
    std::string _animationSetId;
    std::optional<sf::Sprite> _sprite;
    sf::Vector2f _visualScale{1.0f, 1.0f};
    float _spriteOffsetY{0.0f};
};

#include "Game/Behaviours/Animatable.h"
#include  "iostream"

#include "Animation/Animation.h"
#include "Animation/AnimationLibrary.h"

#include <SFML/System/Angle.hpp>
#include <memory>

Animatable::Animatable(sf::Texture &texture) {
    configureVisuals(texture);
}

Animatable::Animatable(sf::Texture &texture, const std::string& animationSetId) {
    configureVisuals(texture, animationSetId);
}

void Animatable::configureVisuals(sf::Texture& texture) {
    _animationSetId.clear();
    bindTexture(texture);
    _animator = Animator();
}

void Animatable::setTextureRect(const sf::IntRect& rect) {
    if (_sprite) {
        _sprite->setTextureRect(rect);
    }
}

void Animatable::configureVisuals(sf::Texture& texture, const std::string& animationSetId) {
    _animationSetId = animationSetId;
    bindTexture(texture);
    try {
        std::shared_ptr<AnimationSet> animationSet = std::make_shared<AnimationSet>(
            AnimationLibrary::getInstance().getAnimationSet(animationSetId)
        );
        _animator = Animator(animationSet);
        _animator.play(animationSet->defaultClip);

        if (_sprite.has_value() && _animator.hasActiveAnimation()) {
            _sprite->setTextureRect(_animator.getCurrentTextureRect());
        } else {
            std::cout << "Animation FAILED: no active animation for " << animationSetId << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Animation EXCEPTION: " << e.what() << std::endl;
        _animationSetId.clear();
        _animator = Animator();
    }
}

void Animatable::updateVisualState(float deltaTime, const sf::Vector2f& hitboxPixels, bool facingLeft) {
    const bool usesSynchronizedBrickAnimation =
        (_animationSetId == "brick" || _animationSetId == "coin_block")
        && _animator.getActiveAnimationName() == "shining";
    const bool frameChanged = _animator.update(deltaTime);

    if (_sprite && _animator.hasActiveAnimation()) {
        const sf::IntRect currentRect = usesSynchronizedBrickAnimation
            ? Animation::getBrickAnimationFrameRect()
            : _animator.getCurrentTextureRect();
        if (frameChanged || _sprite->getTextureRect() != currentRect) {
            _sprite->setTextureRect(currentRect);
        }
    }

    syncSpriteLayout(hitboxPixels, facingLeft);
}

void Animatable::renderVisualState(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees, sf::Shader* shader) const {
    if (!_sprite.has_value()) {
        return;
    }

    sf::Sprite sprite = *_sprite;
    sprite.setPosition({position.x, position.y + _spriteOffsetY});
    sprite.setRotation(sf::degrees(angleDegrees));

    if (shader) {
        sf::RenderStates states;
        states.shader = shader;
        target.draw(sprite, states);
    } else {
        target.draw(sprite);
    }
}

void Animatable::playAnimation(const std::string& name, bool replay) {
    if (!replay && _animator.getActiveAnimationName() == name && !_animator.isAnimationDone()) return;
    _animator.play(name);

    if (_sprite && _animator.hasActiveAnimation()) {
        _sprite->setTextureRect(_animator.getCurrentTextureRect());
    }
}

void Animatable::stopAnimation() {
    _animator.stop();
}

std::string Animatable::getActiveAnimationName() const {
    return _animator.getActiveAnimationName();
}

bool Animatable::isAnimationDone() const {
    return _animator.isAnimationDone();
}

bool Animatable::isLooping() const {
    return _animator.isLooping();
}

bool Animatable::hasSprite() const {
    return _sprite.has_value();
}

void Animatable::bindTexture(sf::Texture& texture) {
    _spritesheet.reset(&texture, [](sf::Texture*) {});
    _sprite = sf::Sprite(*_spritesheet);
}

void Animatable::setVisualScale(sf::Vector2f scale) {
    _visualScale = scale;
}

sf::Vector2f Animatable::getVisualScale() const {
    return _visualScale;
}

void Animatable::syncSpriteLayout(const sf::Vector2f& hitboxPixels, bool facingLeft) {
    if (!_sprite || hitboxPixels.x <= 0.f || hitboxPixels.y <= 0.f) {
        return;
    }

    const sf::Vector2i frameSize = _sprite->getTextureRect().size;
    if (frameSize.x <= 0 || frameSize.y <= 0) {
        return;
    }

    // A vertically flipped sprite must anchor from its top texture edge so
    // its world-space bounds remain above the object's feet.
    const float originY = _visualScale.y < 0.0f
        ? 0.0f
        : static_cast<float>(frameSize.y);
    _sprite->setOrigin({frameSize.x / 2.f, originY});

    _spriteOffsetY = hitboxPixels.y / 2.0f;

    float xOrientation = facingLeft ? -1.0f : 1.0f;
    float intendedScaleX = hitboxPixels.x / static_cast<float>(frameSize.x);
    float intendedScaleY = hitboxPixels.y / static_cast<float>(frameSize.y);
    _sprite->setScale({xOrientation * intendedScaleX * _visualScale.x, intendedScaleY * _visualScale.y});
}

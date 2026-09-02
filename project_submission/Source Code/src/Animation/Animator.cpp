#include "Animation/Animator.h"
#include "Animation/AnimationClip.h"
#include "Animation/AnimationSet.h"

#include <SFML/Graphics/Rect.hpp>

#include <memory>
#include <string>

Animator::Animator() {
    _animations = std::make_shared<AnimationSet>();
}

Animator::Animator(std::shared_ptr<AnimationSet> animationSet): _animations(animationSet) {
    if (!_animations) {
        _animations = std::make_shared<AnimationSet>();
    }
}

void Animator::play(const std::string& name, bool replay) {
    if (!_animations) return;
    if (!replay && _animations->clips.find(name) == _animations->clips.end()) return;

    _currentAnimationName = name;
    _currentFrameId = 0;
    _elapsedTime = 0.f;
    _paused = false;
    _animationDone = false;
}

void Animator::stop() {
    _currentAnimationName.clear();
    _currentFrameId = 0;
    _elapsedTime = 0.f;
    _paused = false;
    _animationDone = false;
}

void Animator::pause() {
    if (hasActiveAnimation()) {
        _paused = true;
    }
}

void Animator::resume() {
    if (hasActiveAnimation()) {
        _paused = false;
    }
}

bool Animator::update(float deltaTime) {
    if (!_animations) return false;
    if (!isPlaying() || deltaTime <= 0.f) {
        return false;
    }

    AnimationClip& animation = _animations->clips.at(_currentAnimationName);
    if (animation.isEmpty()) {
        return false;
    }

    _elapsedTime += deltaTime;
    bool frameChanged = false;

    while (true) {
        const AnimationFrame& currentFrame = animation.getFrame(_currentFrameId);
        if (currentFrame.duration <= 0.f || _elapsedTime < currentFrame.duration) {
            break;
        }

        _elapsedTime -= currentFrame.duration;

        if (_currentFrameId + 1 < animation.getFrameCount()) {
            ++_currentFrameId;
            frameChanged = true;
            continue;
        }

        if (animation.isLooping()) {
            _currentFrameId = 0;
            frameChanged = true;
            continue;
        }

        _currentFrameId = animation.getFrameCount() - 1;
        _paused = true;
        _animationDone = true;
        frameChanged = true;
        break;
    }

    return frameChanged;
}

sf::IntRect Animator::getCurrentTextureRect() const {
    if (!_animations) {
        return {};
    }

    if (!hasActiveAnimation()) {
        return {};
    }

    const AnimationClip& animation = _animations->clips.at(_currentAnimationName);
    if (animation.isEmpty()) {
        return {};
    }

    return animation.getFrame(_currentFrameId).rect;
}

bool Animator::hasActiveAnimation() const {
    return _animations && !_currentAnimationName.empty() && _animations->clips.find(_currentAnimationName) != _animations->clips.end();
}

bool Animator::isPlaying() const {
    return hasActiveAnimation() && !_paused;
}

bool Animator::isPaused() const {
    return hasActiveAnimation() && _paused;
}

bool Animator::isLooping() const {
    return hasActiveAnimation() && _animations->clips.at(_currentAnimationName).isLooping();
}

bool Animator::isAnimationDone() const {
    return hasActiveAnimation() && _animationDone;
}

std::string Animator::getActiveAnimationName() const {
    return _currentAnimationName;
}

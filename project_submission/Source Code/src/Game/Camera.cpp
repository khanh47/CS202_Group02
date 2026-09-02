#include "Game/Camera.h"
#include "Game/Objects/GameObject.h"
#include "Game/GameSettings.h"

#include <cmath>
#include <algorithm>
#include <limits>

Camera::Camera() : Camera(sf::Vector2f(1920.0f, 1080.0f)) {
}

Camera::Camera(const sf::Vector2f& size) {
    _view.setSize(size);
    _currentCenter = {size.x / 2.0f, size.y / 2.0f};
    _currentSize = size;
    _baseSize = size;
    _view.setCenter(_currentCenter);
}

void Camera::update(float deltaTime) {
    // Prune destroyed targets
    for (auto it = _targets.begin(); it != _targets.end();) {
        if (!(*it) || (*it)->isPendingDestroy()) {
            it = _targets.erase(it);
        } else {
            ++it;
        }
    }
    if (_target && _target->isPendingDestroy()) {
        _target.reset();
    }
    if (!_target && !_targets.empty()) {
        _target = _targets.front();
    }

    if (deltaTime <= 0.0f) {
        return;
    }

    if (GameSettings::getInstance().freeCameraMove) {
        // Free camera movement (manual WASD / Arrow keys control)
        float offsetX = 0.0f;
        float offsetY = 0.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
            offsetX -= _freeMoveSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
            offsetX += _freeMoveSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
            offsetY -= _freeMoveSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
            offsetY += _freeMoveSpeed * deltaTime;
        }

        _currentCenter += sf::Vector2f(offsetX, offsetY);
        _currentCenter = clampToBounds(_currentCenter);
        _view.setCenter(_currentCenter);
    } else if (_targets.size() >= 2) {
        // Multi-target: center on the midpoint of all targets and zoom out so
        // every target stays on screen.
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        for (const std::shared_ptr<GameObject>& target : _targets) {
            if (!target) {
                continue;
            }
            const sf::Vector2f pos = target->getPosition();
            minX = std::min(minX, pos.x);
            maxX = std::max(maxX, pos.x);
            minY = std::min(minY, pos.y);
            maxY = std::max(maxY, pos.y);
        }

        constexpr float padding = 180.0f;

        const float spanX = std::max(0.0f, (maxX - minX) + padding * 2.0f);
        const float spanY = std::max(0.0f, (maxY - minY) + padding * 2.0f);

        // Unlimited zoom out — keep aspect ratio, only floor at 1.0 (never zoom in).
        const float zoom = std::max(1.0f, std::max(spanX / _baseSize.x, spanY / _baseSize.y));

        const sf::Vector2f desiredSize = {_baseSize.x * zoom, _baseSize.y * zoom};

        const sf::Vector2f midpoint = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f};

        const float factorCenter = 1.0f - std::exp(-_config.dampingX * deltaTime);
        _currentCenter.x += (midpoint.x - _currentCenter.x) * factorCenter;
        _currentCenter.y += (midpoint.y - _currentCenter.y) * factorCenter;

        const float factorSize = 1.0f - std::exp(-2.0f * deltaTime);
        _currentSize.x += (desiredSize.x - _currentSize.x) * factorSize;
        _currentSize.y += (desiredSize.y - _currentSize.y) * factorSize;

        _currentCenter = clampToBounds(_currentCenter);
        _view.setSize(_currentSize);
        _view.setCenter(_currentCenter);
    } else if (_target) {
        const sf::Vector2f targetPos = _target->getPosition();
        const sf::Vector2f targetVel = _target->getVelocity();

        // Smoothly restore the view size back to base (in case a previous
        // multi-target session had zoomed out).
        const float factorSize = 1.0f - std::exp(-2.0f * deltaTime);
        _currentSize.x += (_baseSize.x - _currentSize.x) * factorSize;
        _currentSize.y += (_baseSize.y - _currentSize.y) * factorSize;
        _view.setSize(_currentSize);

        // 1. Deadzone & Y-Stabilization: Calculate base target camera focus position
        sf::Vector2f targetFocus = calculateTargetFocus(targetPos, targetVel);

        // 2. Lookahead (Forward Focus): Anticipate horizontal movement direction
        float targetLookahead = 0.0f;
        if (targetVel.x > 10.0f) {
            targetLookahead = _config.lookaheadDistance;
        } else if (targetVel.x < -10.0f) {
            targetLookahead = -_config.lookaheadDistance;
        }

        // Smoothly interpolate lookahead offset over time using exponential decay
        const float lookaheadFactor = 1.0f - std::exp(-_config.lookaheadSpeed * deltaTime);
        _currentLookaheadX += (targetLookahead - _currentLookaheadX) * lookaheadFactor;
        targetFocus.x += _currentLookaheadX;

        // 3. Smooth Damping (Interpolation): Frame-rate independent exponential lerp
        const float factorX = 1.0f - std::exp(-_config.dampingX * deltaTime);
        const float factorY = 1.0f - std::exp(-_config.dampingY * deltaTime);

        _currentCenter.x += (targetFocus.x - _currentCenter.x) * factorX;
        _currentCenter.y += (targetFocus.y - _currentCenter.y) * factorY;

        // 5. Boundary Clamping: Restrict view center to level boundaries
        _currentCenter = clampToBounds(_currentCenter);

        _view.setCenter(_currentCenter);
    }
}

sf::Vector2f Camera::calculateTargetFocus(const sf::Vector2f& targetPos, const sf::Vector2f& targetVel) const {
    (void)targetVel;
    sf::Vector2f focus = _currentCenter;

    const float dx = targetPos.x - _currentCenter.x;
    const float dy = targetPos.y - _currentCenter.y;
    const float halfW = _config.deadzoneSize.x * 0.5f;
    const float halfH = _config.deadzoneSize.y * 0.5f;

    // Horizontal Deadzone: Camera shifts only when player pushes against box boundaries
    if (dx > halfW) {
        focus.x = targetPos.x - halfW;
    } else if (dx < -halfW) {
        focus.x = targetPos.x + halfW;
    } else {
        focus.x = _currentCenter.x;
    }

    // Vertical Stabilization vs Deadzone: Ignores minor jumps unless vertical threshold is exceeded
    if (_config.yStabilizationEnabled) {
        if (std::abs(dy) > _config.yThreshold) {
            focus.y = targetPos.y - (dy > 0.0f ? _config.yThreshold : -_config.yThreshold);
        } else {
            focus.y = _currentCenter.y;
        }
    } else {
        if (dy > halfH) {
            focus.y = targetPos.y - halfH;
        } else if (dy < -halfH) {
            focus.y = targetPos.y + halfH;
        } else {
            focus.y = _currentCenter.y;
        }
    }

    return focus;
}

sf::Vector2f Camera::clampToBounds(const sf::Vector2f& center) const {
    if (!_config.useBounds) {
        return center;
    }

    const sf::Vector2f viewHalfSize = _view.getSize() * 0.5f;
    sf::Vector2f clamped = center;

    const float minX = _config.levelBounds.position.x + viewHalfSize.x;
    const float maxX = _config.levelBounds.position.x + _config.levelBounds.size.x - viewHalfSize.x;
    const float minY = _config.levelBounds.position.y + viewHalfSize.y;
    const float maxY = _config.levelBounds.position.y + _config.levelBounds.size.y - viewHalfSize.y;

    if (maxX >= minX) {
        clamped.x = std::clamp(clamped.x, minX, maxX);
    } else {
        // Level is narrower than view width: center view horizontally in level
        clamped.x = _config.levelBounds.position.x + _config.levelBounds.size.x * 0.5f;
    }

    if (maxY >= minY) {
        clamped.y = std::clamp(clamped.y, minY, maxY);
    } else {
        // Level is shorter than view height: center view vertically in level
        clamped.y = _config.levelBounds.position.y + _config.levelBounds.size.y * 0.5f;
    }

    return clamped;
}

void Camera::setTarget(std::shared_ptr<GameObject> target) {
    _targets.clear();
    if (target) {
        _targets.push_back(target);
    }
    _target = target;
    if (_target) {
        // Center camera immediately on target when set
        _currentCenter = clampToBounds(_target->getPosition());
        _view.setCenter(_currentCenter);
    }
}

void Camera::setTargets(const std::vector<std::shared_ptr<GameObject>>& targets) {
    _targets.clear();
    for (const std::shared_ptr<GameObject>& target : targets) {
        if (target) {
            _targets.push_back(target);
        }
    }
    _target = _targets.empty() ? nullptr : _targets.front();

    if (_targets.empty()) {
        return;
    }

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (const std::shared_ptr<GameObject>& target : _targets) {
        const sf::Vector2f pos = target->getPosition();
        minX = std::min(minX, pos.x);
        maxX = std::max(maxX, pos.x);
        minY = std::min(minY, pos.y);
        maxY = std::max(maxY, pos.y);
    }

    const sf::Vector2f midpoint = {(minX + maxX) * 0.5f, (minY + maxY) * 0.5f};
    _currentCenter = clampToBounds(midpoint);
    _view.setCenter(_currentCenter);
}

void Camera::setConfig(const CameraConfig& config) {
    _config = config;
    _currentCenter = clampToBounds(_currentCenter);
    _view.setCenter(_currentCenter);
}

CameraConfig& Camera::getConfig() {
    return _config;
}

const CameraConfig& Camera::getConfig() const {
    return _config;
}

void Camera::setDeadzone(const sf::Vector2f& size) {
    _config.deadzoneSize = size;
}

void Camera::setLookahead(float distance, float speed) {
    _config.lookaheadDistance = distance;
    _config.lookaheadSpeed = speed;
}

void Camera::setDamping(float dampingX, float dampingY) {
    _config.dampingX = dampingX;
    _config.dampingY = dampingY;
}

void Camera::setYStabilization(bool enabled, float threshold) {
    _config.yStabilizationEnabled = enabled;
    _config.yThreshold = threshold;
}

void Camera::setLevelBounds(const sf::FloatRect& bounds) {
    _config.levelBounds = bounds;
    _config.useBounds = true;
    _currentCenter = clampToBounds(_currentCenter);
    _view.setCenter(_currentCenter);
}

void Camera::setMoveSpeed(float speed) {
    _freeMoveSpeed = speed;
}

void Camera::move(float offsetX, float offsetY) {
    _currentCenter += sf::Vector2f(offsetX, offsetY);
    _currentCenter = clampToBounds(_currentCenter);
    _view.setCenter(_currentCenter);
}

void Camera::setCenter(const sf::Vector2f& center) {
    _currentCenter = clampToBounds(center);
    _view.setCenter(_currentCenter);
}

void Camera::setSize(const sf::Vector2f& size) {
    _view.setSize(size);
    _baseSize = size;
    _currentSize = size;
    _currentCenter = clampToBounds(_currentCenter);
    _view.setCenter(_currentCenter);
}

const sf::View& Camera::getView() const {
    return _view;
}

void Camera::renderDebug(sf::RenderTarget& target) const {
    // 1. Draw Deadzone Box Outline
    sf::RectangleShape deadzoneRect(_config.deadzoneSize);
    deadzoneRect.setOrigin(_config.deadzoneSize * 0.5f);
    deadzoneRect.setPosition(_currentCenter);
    deadzoneRect.setFillColor(sf::Color(255, 255, 0, 40));
    deadzoneRect.setOutlineColor(sf::Color::Yellow);
    deadzoneRect.setOutlineThickness(2.0f);
    target.draw(deadzoneRect);

    // 2. Draw Y Stabilization Threshold Lines
    if (_config.yStabilizationEnabled) {
        sf::VertexArray yLines(sf::PrimitiveType::Lines);
        const float halfWidth = _view.getSize().x * 0.4f;

        // Top threshold line
        yLines.append(sf::Vertex({_currentCenter.x - halfWidth, _currentCenter.y - _config.yThreshold}, sf::Color::Magenta));
        yLines.append(sf::Vertex({_currentCenter.x + halfWidth, _currentCenter.y - _config.yThreshold}, sf::Color::Magenta));

        // Bottom threshold line
        yLines.append(sf::Vertex({_currentCenter.x - halfWidth, _currentCenter.y + _config.yThreshold}, sf::Color::Magenta));
        yLines.append(sf::Vertex({_currentCenter.x + halfWidth, _currentCenter.y + _config.yThreshold}, sf::Color::Magenta));

        target.draw(yLines);
    }

    // 3. Draw Target Position Marker & Lookahead Offset
    if (_target) {
        const sf::Vector2f targetPos = _target->getPosition();

        sf::CircleShape targetPoint(6.0f);
        targetPoint.setOrigin({6.0f, 6.0f});
        targetPoint.setPosition(targetPos);
        targetPoint.setFillColor(sf::Color::Green);
        target.draw(targetPoint);

        // Line representing lookahead offset vector
        sf::VertexArray lookaheadLine(sf::PrimitiveType::Lines);
        lookaheadLine.append(sf::Vertex(targetPos, sf::Color::Cyan));
        lookaheadLine.append(sf::Vertex(targetPos + sf::Vector2f(_currentLookaheadX, 0.0f), sf::Color::Cyan));
        target.draw(lookaheadLine);
    }

    // 4. Draw Level Bounds Outline (if active)
    if (_config.useBounds) {
        sf::RectangleShape boundsRect(_config.levelBounds.size);
        boundsRect.setPosition(_config.levelBounds.position);
        boundsRect.setFillColor(sf::Color::Transparent);
        boundsRect.setOutlineColor(sf::Color::Cyan);
        boundsRect.setOutlineThickness(3.0f);
        target.draw(boundsRect);
    }
}

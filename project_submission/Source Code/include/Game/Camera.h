#pragma once

#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>

class GameObject;

/**
 * @brief Configuration parameters for 2D Platformer Camera mechanics.
 */
struct CameraConfig {
    sf::Vector2f deadzoneSize{200.0f, 150.0f};          // Invisible rectangular deadzone box (pixels)
    float lookaheadDistance = 150.0f;                   // Forward focus offset magnitude in direction of movement
    float lookaheadSpeed = 3.0f;                       // Interpolation speed multiplier for lookahead transitions
    float dampingX = 5.0f;                              // Horizontal smooth damping coefficient (frame-rate independent lerp)
    float dampingY = 4.0f;                              // Vertical smooth damping coefficient
    bool yStabilizationEnabled = true;                  // Enable Y-axis stabilization for platformer jumping
    float yThreshold = 120.0f;                          // Vertical displacement limit before Y tracking triggers
    sf::FloatRect levelBounds{{0.0f, 0.0f}, {0.0f, 0.0f}}; // Level boundary limits (position, size)
    bool useBounds = false;                             // Whether level boundary clamping is active
};

/**
 * @brief 2D Platformer Camera System.
 *
 * Implements Deadzone (Window Box), Lookahead (Forward Focus), Smooth Damping,
 * Y-Axis Stabilization, and Level Boundary Clamping.
 */
class Camera {
public:
    Camera();
    explicit Camera(const sf::Vector2f& size);

    void update(float deltaTime);

    // Target & Configuration
    void setTarget(std::shared_ptr<GameObject> target);
    std::shared_ptr<GameObject> getTarget() const { return _target; }
    void setTargets(const std::vector<std::shared_ptr<GameObject>>& targets);
    void setConfig(const CameraConfig& config);
    CameraConfig& getConfig();
    const CameraConfig& getConfig() const;

    // Helper Configuration Methods
    void setDeadzone(const sf::Vector2f& size);
    void setLookahead(float distance, float speed);
    void setDamping(float dampingX, float dampingY);
    void setYStabilization(bool enabled, float threshold = 120.0f);
    void setLevelBounds(const sf::FloatRect& bounds);
    void setMoveSpeed(float speed);

    // View Manipulation
    void move(float offsetX, float offsetY);
    void setCenter(const sf::Vector2f& center);
    void setSize(const sf::Vector2f& size);
    const sf::View& getView() const;

    // Debug Visualization
    void renderDebug(sf::RenderTarget& target) const;

private:
    sf::View _view;
    std::shared_ptr<GameObject> _target;
    std::vector<std::shared_ptr<GameObject>> _targets;
    CameraConfig _config;

    sf::Vector2f _currentCenter{0.0f, 0.0f};
    sf::Vector2f _currentSize{0.0f, 0.0f};
    sf::Vector2f _baseSize{0.0f, 0.0f};
    float _currentLookaheadX = 0.0f;
    float _freeMoveSpeed = 2000.0f;

    // Internal pipeline helpers
    sf::Vector2f calculateTargetFocus(const sf::Vector2f& targetPos, const sf::Vector2f& targetVel) const;
    sf::Vector2f clampToBounds(const sf::Vector2f& center) const;
};

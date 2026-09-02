#pragma once
#include "../Enemy.h"

class GameWorld;

class Koopa : public Enemy {
public:
    static constexpr float defaultVisualScaleX = 1.5f;
    static constexpr float defaultVisualScaleY = 1.2f;

    Koopa();
    Koopa(sf::Texture& texture, const std::string& animationSetId = "koopa", bool isReviving = false);
    ~Koopa() override = default;
    void onStomp() override;
    void updateSimulation(const float &fixedDt) override;

    void setGameWorld(GameWorld* world) { _world = world; }
private:
    void onUpdateVisuals(float deltaTime) override;
    bool _isReviving = false;

    GameWorld* _world = nullptr;
};

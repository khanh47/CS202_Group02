#pragma once
#include "../Enemy.h"

class Goomba : public Enemy {
public:
    Goomba();
    Goomba(sf::Texture& texture, const std::string& animationSetId = "goomba");
    ~Goomba() override = default;
    void updateSimulation(const float &fixedDt) override;
    void onStomp() override;

private:
    bool _stomped = false;
};
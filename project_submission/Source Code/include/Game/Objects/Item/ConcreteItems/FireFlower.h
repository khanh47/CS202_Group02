#pragma once

#include "../Item.h"

class Player;

/**
 * @brief Represents a Fire Flower pickup item.
 * When touched by Mario, changes Mario's state to FireState.
 */
class FireFlower : public Item {
public:
    FireFlower();
    FireFlower(sf::Texture& texture);
    ~FireFlower() override = default;

    void onPickup(Player& player);

protected:
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onUpdateVisuals(float deltaTime) override;
};

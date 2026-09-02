#pragma once

#include "../Item.h"

class Player;

/**
 * @brief Represents a 1-Up Mushroom pickup that grants an extra life.
 */
class OneUpMushroom : public Item {
public:
    OneUpMushroom();
    OneUpMushroom(sf::Texture& texture);
    ~OneUpMushroom() override = default;

    void onPickup(Player& player);

protected:
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onUpdateVisuals(float deltaTime) override;
};

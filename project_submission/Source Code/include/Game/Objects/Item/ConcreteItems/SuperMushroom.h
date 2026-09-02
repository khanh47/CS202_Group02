#pragma once

#include "../Item.h"

class Player;

/**
 * @brief Represents a Super Mushroom pickup item.
 * When touched by a Normal-state player, transitions them to Super state.
 * Has no effect on Super or Fire state players.
 */
class SuperMushroom : public Item {
public:
    SuperMushroom();
    SuperMushroom(sf::Texture& texture);
    ~SuperMushroom() override = default;

    void onPickup(Player& player);

protected:
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onUpdateVisuals(float deltaTime) override;
};

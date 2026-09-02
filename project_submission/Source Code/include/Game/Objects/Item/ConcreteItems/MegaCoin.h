#pragma once

#include "../Item.h"

class Player;

/**
 * @brief Represents a Mega Coin pickup item.
 * When touched by Mario, grants a large number of points.
 */
class MegaCoin : public Item {
public:
    MegaCoin();
    MegaCoin(sf::Texture& texture);
    ~MegaCoin() override = default;

    void onPickup(Player& player);

protected:
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onUpdateVisuals(float deltaTime) override;
};

#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <string>

#include "Game/Objects/Projectile/FireballPool.h"

class WorldMap;
class WorldObjectStore;

class WorldRenderer {
public:
    void render(
        sf::RenderTarget& target,
        WorldMap& worldMap,
        const WorldObjectStore& objectStore,
        std::array<FireballPool, 2>& fireballPools
    ) const;

private:
    void renderBackground(
        sf::RenderTarget& target,
        const std::string& backgroundKey
    ) const;
    void renderDebugGrid(
        sf::RenderTarget& target,
        const WorldMap& worldMap
    ) const;
};

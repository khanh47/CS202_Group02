#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <box2d/box2d.h>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Game/Objects/Projectile/FireballPool.h"
#include "Game/World/AutotileResolver.h"
#include "Game/World/AutotileTilesetDef.h"
#include "Game/World/LevelDataLoader.h"
#include "Game/World/TerrainSeamFilter.h"
#include "Game/World/TileMap.h"

class GameObject;
class GameObjectFactory;
class GameWorld;
class PhysicsWorld;
class WorldObjectStore;

class WorldMap {
public:
    static constexpr float defaultCellSize = 64.0f;
    static constexpr int defaultGridWidth = 500;
    static constexpr int defaultGridHeight = 60;

    WorldMap(
        int gridWidth = defaultGridWidth,
        int gridHeight = defaultGridHeight,
        float cellSize = defaultCellSize
    );
    ~WorldMap();

    void rebuild(
        const LevelData& levelData,
        PhysicsWorld& physicsWorld,
        GameObjectFactory& objectFactory,
        std::array<FireballPool, 2>& fireballPools,
        WorldObjectStore& objectStore,
        GameWorld& gameWorld
    );

    const TerrainSeamFilter& getTerrainSeamFilter() const noexcept {
        return _terrainSeamFilter;
    }
    TerrainSeamFilter& getTerrainSeamFilter() noexcept {
        return _terrainSeamFilter;
    }

    void renderTiles(sf::RenderTarget& target);
    void updateVisuals(float deltaTime);
    void cleanupDestroyedTiles();
    const std::vector<sf::Vector2i>& getDestroyedTileCells() const noexcept {
        return _destroyedTileCells;
    }
    void restoreDestroyedTileCells(
        const std::vector<sf::Vector2i>& cells
    );
    sf::FloatRect getBounds() const;
    sf::Vector2f mapCellCenter(int column, int mapRow) const;
    static int screenRowFor(
        int mapRow,
        int loadedRows,
        int gridHeight
    ) noexcept {
        return gridHeight - (loadedRows - mapRow);
    }

    int getGridWidth() const noexcept { return _gridWidth; }
    int getGridHeight() const noexcept { return _gridHeight; }
    float getCellSize() const noexcept { return _cellSize; }
    int getLoadedRows() const noexcept { return _loadedRows; }
    int getLoadedColumns() const noexcept { return _loadedColumns; }
    std::size_t getPlayerCount() const noexcept { return _playerCount; }
    const std::string& getBackground() const noexcept { return _background; }

private:
    int screenYForMapRow(int mapRow) const noexcept;
    void loadAutotileDefs(const std::filesystem::path& path);

    void createBoundaryWalls(PhysicsWorld& physicsWorld);
    void destroyBoundaryWalls();
    void destroyTileCollisionObjects();
    void createTileCollision(
        PhysicsWorld& physicsWorld,
        int column,
        int screenRow,
        bool breakable,
        bool isBrick,
        const sf::Texture* texture,
        sf::IntRect textureRect = {}
    );

    void setTileCollisionBreakTexture(
        int column,
        int screenRow,
        const sf::Texture* texture,
        sf::IntRect textureRect
    );

    struct TileCollision {
        std::shared_ptr<GameObject> object;
        int column;
        int screenRow;
    };

    float _cellSize;
    int _gridWidth;
    int _gridHeight;
    std::size_t _playerCount = 0;
    int _loadedColumns = 0;
    int _loadedRows = 0;
    std::string _background;
    TileMap _tileMap;
    std::unordered_map<std::string, AutotileTilesetDef> _autotileDefs;
    AutotileResolver _autotileResolver;
    TerrainSeamFilter _terrainSeamFilter;
    std::vector<TileCollision> _tileCollisionObjects;
    std::vector<sf::Vector2i> _destroyedTileCells;
    std::vector<b2BodyId> _boundaryWalls;
};

#include "Game/World/WorldMap.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <nlohmann/json.hpp>

#include "Game/Objects/Block/Block.h"
#include "Game/Objects/GameObject.h"
#include "Game/Objects/GameObjectFactory.h"
#include "Game/Objects/Projectile/FireballPool.h"
#include "Game/World/GameWorld.h"
#include "Game/World/PrefabSpawner.h"
#include "Game/World/ThemeAssets.h"
#include "Physics/CollisionFilter.h"
#include "Physics/PhysicsUnits.h"
#include "Physics/PhysicsWorld.h"
#include "ResourceManager.h"

WorldMap::WorldMap(int gridWidth, int gridHeight, float cellSize)
    : _cellSize(cellSize),
      _gridWidth(gridWidth),
      _gridHeight(gridHeight) {}

WorldMap::~WorldMap() {
    destroyBoundaryWalls();
    destroyTileCollisionObjects();
}

void WorldMap::loadAutotileDefs(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error(
            "Unable to open autotile definitions: " + path.string()
        );
    }

    nlohmann::json document;
    input >> document;
    const auto definitionsIt = document.find("autotile_defs");
    if (definitionsIt == document.end() || !definitionsIt->is_array()) {
        throw std::runtime_error(
            "Autotile definition file must contain an autotile_defs array"
        );
    }

    _autotileDefs.clear();
    for (const nlohmann::json& definition : *definitionsIt) {
        if (!definition.is_object()
            || !definition.contains("id")
            || !definition["id"].is_string()) {
            throw std::runtime_error(
                "Every autotile definition must contain a string id"
            );
        }

        _autotileDefs.insert_or_assign(
            definition["id"].get<std::string>(),
            AutotileTilesetDef::fromJson(definition)
        );
    }
}

void WorldMap::rebuild(
    const LevelData& levelData,
    PhysicsWorld& physicsWorld,
    GameObjectFactory& objectFactory,
    std::array<FireballPool, 2>& fireballPools,
    WorldObjectStore& objectStore,
    GameWorld& gameWorld
) {
    const std::vector<std::string>& layer = levelData.layer;

    auto& resources = ResourceManager::getInstance();
    sf::Texture& itemsTexture = resources.getTexture("mario_and_items");

    _background = levelData.background;
    _autotileDefs.clear();
    _destroyedTileCells.clear();
    objectStore.clear();
    for (FireballPool& pool : fireballPools) {
        pool.initialize(physicsWorld, itemsTexture);
    }

    _terrainSeamFilter.clear();
    destroyBoundaryWalls();
    destroyTileCollisionObjects();
    _terrainSeamFilter.install(physicsWorld);

    _loadedRows = std::min(
        static_cast<int>(layer.size()),
        _gridHeight
    );
    _loadedColumns = 0;
    for (int row = 0; row < _loadedRows; ++row) {
        _loadedColumns = std::max(
            _loadedColumns,
            std::min(static_cast<int>(layer[row].size()), _gridWidth)
        );
    }

    _tileMap.initialize(_gridWidth, _gridHeight, _cellSize);
    createBoundaryWalls(physicsWorld);
    _terrainSeamFilter.setBoundaryColumns(-1, _loadedColumns);

    std::unordered_map<char, SpawnSpec> specsBySymbol;
    bool hasAutotiles = false;
    for (const auto& [symbol, prefabId] : levelData.tileMapping) {
        SpawnSpec spec = levelData.prefabs.resolve(prefabId);
        spec.textureKey = ThemeAssets::textureAliasFor(
            spec,
            levelData.theme
        );
        hasAutotiles = hasAutotiles || !spec.autotileId.empty();
        specsBySymbol.insert_or_assign(symbol, std::move(spec));
    }
    std::unordered_map<int, const SpawnSpec*> placementByCell;
    for (const LevelData::Placement& placement : levelData.placements) {
        placementByCell.insert_or_assign(
            placement.row * _gridWidth + placement.column,
            &placement.spec
        );
    }

    _playerCount = 0;
    for (int mapRow = 0; mapRow < _loadedRows; ++mapRow) {
        const int columns = std::min(
            static_cast<int>(layer[mapRow].size()),
            _gridWidth
        );
        for (int column = 0; column < columns; ++column) {
            const char symbol = layer[mapRow][column];
            const auto specIt = specsBySymbol.find(symbol);
            if (specIt == specsBySymbol.end()) {
                continue;
            }

            const SpawnSpec* spec = &specIt->second;
            const auto placementIt = placementByCell.find(
                mapRow * _gridWidth + column
            );
            if (placementIt != placementByCell.end()) {
                spec = placementIt->second;
            }
            if (spec->objectKind
                && *spec->objectKind == ObjectKind::Player) {
                ++_playerCount;
            }
        }
    }

    PrefabSpawner spawner(
        physicsWorld,
        objectFactory,
        objectStore,
        gameWorld,
        _terrainSeamFilter,
        _cellSize,
        _playerCount
    );

    if (hasAutotiles) {
        loadAutotileDefs("assets/datas/autotile_defs.json");
    }

    const auto tileIdFor = [](char symbol) {
        return static_cast<int>(static_cast<unsigned char>(symbol)) + 1;
    };
    std::vector<std::vector<int>> screenGrid(
        static_cast<std::size_t>(_gridHeight),
        std::vector<int>(static_cast<std::size_t>(_gridWidth), 0)
    );
    std::unordered_set<int> solidIds;

    // The single dense layer can describe either a static tile or a live
    // object. The mapped prefab decides which path the cell takes.
    for (int mapRow = 0; mapRow < _loadedRows; ++mapRow) {
        const int screenRow = screenYForMapRow(mapRow);
        const int columns = std::min(
            static_cast<int>(layer[mapRow].size()),
            _gridWidth
        );

        for (int column = 0; column < columns; ++column) {
            const char symbol = layer[mapRow][column];
            const auto specIt = specsBySymbol.find(symbol);
            if (specIt == specsBySymbol.end()) {
                continue;
            }

            SpawnSpec resolvedSpec = specIt->second;
            const auto placementIt = placementByCell.find(
                mapRow * _gridWidth + column
            );
            if (placementIt != placementByCell.end()) {
                resolvedSpec = *placementIt->second;
            }
            resolvedSpec.textureKey = ThemeAssets::textureAliasFor(
                resolvedSpec,
                levelData.theme
            );
            const SpawnSpec& spec = resolvedSpec;
            const sf::Vector2f cellCenter = mapCellCenter(column, mapRow);

            const int tileId = tileIdFor(symbol);
            screenGrid[screenRow][column] = tileId;
            if (spec.solid
                || (spec.objectKind
                    && *spec.objectKind == ObjectKind::Block)) {
                solidIds.insert(tileId);
            }

            if (!spec.objectKind) {
                sf::Texture* texture = nullptr;
                if (!spec.textureKey.empty()) {
                    texture = &resources.getTexture(spec.textureKey);
                }
                const bool isAnimatedBrick =
                    ThemeAssets::isBrickTextureAlias(spec.textureKey);
                _tileMap.setTile(
                    column,
                    screenRow,
                    symbol,
                    texture,
                    isAnimatedBrick
                        ? sf::IntRect({0, 0}, {64, 64})
                        : sf::IntRect{},
                    isAnimatedBrick
                        ? TileMap::TileAnimation::Brick
                        : TileMap::TileAnimation::None
                );

                if (spec.solid) {
                    createTileCollision(
                        physicsWorld,
                        column,
                        screenRow,
                        spec.breakable,
                        isAnimatedBrick,
                        texture,
                        isAnimatedBrick
                            ? sf::IntRect({0, 0}, {64, 64})
                            : sf::IntRect{}
                    );
                }
                continue;
            }

            spawner.spawnAtGrid(
                spec,
                column,
                screenRow,
                cellCenter
            );
        }
    }

    // Autotile definitions affect only static tiles. Dynamic blocks keep
    // their own sprites and physics bodies; static terrain remains entirely
    // owned by TileMap plus the one collision body created above.
    if (hasAutotiles) {
        for (const auto& [defId, definition] : _autotileDefs) {
            std::unordered_set<int> autotileSolidIds;
            for (const auto& [symbol, spec] : specsBySymbol) {
                if (spec.autotileId == defId) {
                    autotileSolidIds.insert(tileIdFor(symbol));
                }
            }

            _autotileResolver.precompute(screenGrid, _gridWidth, _gridHeight, autotileSolidIds);

            for (int mapRow = 0; mapRow < _loadedRows; ++mapRow) {
                const int screenRow = screenYForMapRow(mapRow);
                const int columns = std::min(
                    static_cast<int>(layer[mapRow].size()),
                    _gridWidth
                );

                for (int column = 0; column < columns; ++column) {
                    const char symbol = layer[mapRow][column];
                    const auto specIt = specsBySymbol.find(symbol);
                    if (specIt == specsBySymbol.end()) {
                        continue;
                    }

                    const SpawnSpec& spec = specIt->second;
                    if (spec.objectKind || spec.autotileId != defId) {
                        continue;
                    }

                    sf::Texture* fallbackTexture = nullptr;
                    if (!spec.textureKey.empty()) {
                        fallbackTexture = &resources.getTexture(
                            spec.textureKey
                        );
                    }
                    if (fallbackTexture == nullptr) {
                        continue;
                    }

                    sf::Texture& autotileTexture = resources.getTexture(
                        definition.textureAlias
                    );
                    const AutotileResult autoRes = _autotileResolver.resolveDetailed(
                        screenGrid,
                        column,
                        screenRow,
                        _gridWidth,
                        _gridHeight,
                        autotileSolidIds,
                        definition
                    );
                    _tileMap.setTile(
                        column,
                        screenRow,
                        symbol,
                        &autotileTexture,
                        autoRes.texRect,
                        TileMap::TileAnimation::None,
                        autoRes.rotationDeg
                    );
                    if (autoRes.hasOverlay) {
                        _tileMap.setOverlayTile(
                            column,
                            screenRow,
                            symbol,
                            &autotileTexture,
                            autoRes.overlayRect
                        );
                    }

                    if (spec.solid && spec.breakable) {
                        setTileCollisionBreakTexture(
                            column,
                            screenRow,
                            &autotileTexture,
                            autoRes.texRect
                        );
                    }
                }
            }
        }
    }
}

void WorldMap::renderTiles(sf::RenderTarget& target) {
    _tileMap.updateVisibleVertices(target.getView());
    target.draw(_tileMap);
}

void WorldMap::updateVisuals(float deltaTime) {
    _tileMap.update(deltaTime);
}

void WorldMap::cleanupDestroyedTiles() {
    std::vector<TileCollision> liveTiles;
    liveTiles.reserve(_tileCollisionObjects.size());

    for (TileCollision& tile : _tileCollisionObjects) {
        if (!tile.object || tile.object->isPendingDestroy()) {
            _tileMap.setTile(tile.column, tile.screenRow, '.', nullptr);
            const bool alreadyRecorded = std::any_of(
                _destroyedTileCells.begin(),
                _destroyedTileCells.end(),
                [&tile](const sf::Vector2i& cell) {
                    return cell.x == tile.column
                        && cell.y == tile.screenRow;
                }
            );
            if (!alreadyRecorded) {
                _destroyedTileCells.emplace_back(
                    tile.column,
                    tile.screenRow
                );
            }
            continue;
        }
        liveTiles.push_back(std::move(tile));
    }

    _tileCollisionObjects = std::move(liveTiles);
}

void WorldMap::restoreDestroyedTileCells(
    const std::vector<sf::Vector2i>& cells
) {
    for (const sf::Vector2i& cell : cells) {
        for (TileCollision& tile : _tileCollisionObjects) {
            if (tile.column == cell.x && tile.screenRow == cell.y) {
                if (tile.object) {
                    tile.object->destroy();
                }
                break;
            }
        }
    }
    cleanupDestroyedTiles();
}

sf::FloatRect WorldMap::getBounds() const {
    const float width = (_loadedColumns > 0 ? _loadedColumns : _gridWidth)
        * _cellSize;
    return {{0.0f, 0.0f}, {width, _gridHeight * _cellSize}};
}

sf::Vector2f WorldMap::mapCellCenter(int column, int mapRow) const {
    return {
        column * _cellSize + _cellSize * 0.5f,
        screenYForMapRow(mapRow) * _cellSize + _cellSize * 0.5f
    };
}

int WorldMap::screenYForMapRow(int mapRow) const noexcept {
    return screenRowFor(mapRow, _loadedRows, _gridHeight);
}

void WorldMap::createTileCollision(
    PhysicsWorld& physicsWorld,
    int column,
    int screenRow,
    bool breakable,
    bool isBrick,
    const sf::Texture* texture,
    sf::IntRect textureRect
) {
    // Keep one static body per occupied cell. Besides making tile collision
    // ownership explicit, this lets TerrainSeamFilter remove internal seams
    // exactly at the same coordinates as the dense layer.
    auto collisionTile = std::make_shared<Block>();
    collisionTile->spawn(
        physicsWorld,
        {
            column * _cellSize + _cellSize * 0.5f,
            screenRow * _cellSize + _cellSize * 0.5f
        },
        {_cellSize, _cellSize}
    );
    collisionTile->setBreakable(breakable);
    collisionTile->setBrick(isBrick);
    collisionTile->setBreakEffectTexture(texture, textureRect);
    _terrainSeamFilter.addBlock(
        collisionTile,
        column,
        screenRow,
        column * _cellSize,
        (column + 1) * _cellSize
    );
    _tileCollisionObjects.push_back({
        std::move(collisionTile),
        column,
        screenRow
    });
}

void WorldMap::setTileCollisionBreakTexture(
    int column,
    int screenRow,
    const sf::Texture* texture,
    sf::IntRect textureRect
) {
    for (TileCollision& tile : _tileCollisionObjects) {
        if (tile.column != column || tile.screenRow != screenRow) {
            continue;
        }

        if (auto block = std::dynamic_pointer_cast<Block>(tile.object)) {
            block->setBreakEffectTexture(texture, textureRect);
        }
        return;
    }
}

void WorldMap::createBoundaryWalls(PhysicsWorld& physicsWorld) {
    b2BodyDef wallDef = b2DefaultBodyDef();
    wallDef.type = b2_staticBody;

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.filter.categoryBits = CollisionFilter::ENV;
    shapeDef.enableContactEvents = false;
    shapeDef.enableSensorEvents = false;

    const b2Polygon box = b2MakeBox(
        PhysicsUnits::toMeters(_cellSize * 0.25f),
        PhysicsUnits::toMeters(_gridHeight * _cellSize * 0.5f)
    );

    const float rightEdge = _loadedColumns * _cellSize;
    const float midY = _gridHeight * _cellSize * 0.5f;

    for (float edgeX : {
             -_cellSize * 0.25f,
             rightEdge + _cellSize * 0.25f
         }) {
        wallDef.position = PhysicsUnits::toMeters({edgeX, midY});
        const b2BodyId body = b2CreateBody(physicsWorld.getId(), &wallDef);
        b2CreatePolygonShape(body, &shapeDef, &box);
        _boundaryWalls.push_back(body);
    }
}

void WorldMap::destroyTileCollisionObjects() {
    _tileCollisionObjects.clear();
}

void WorldMap::destroyBoundaryWalls() {
    for (const b2BodyId body : _boundaryWalls) {
        if (b2Body_IsValid(body)) {
            b2DestroyBody(body);
        }
    }
    _boundaryWalls.clear();
}

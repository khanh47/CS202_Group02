#pragma once

#include <cstddef>
#include <memory>

#include <SFML/System/Vector2.hpp>

#include "Game/World/SpawnSpec.h"

class GameObject;
class GameObjectFactory;
class GameWorld;
class PhysicsWorld;
class TerrainSeamFilter;
class WorldObjectStore;

// Converts resolved prefab definitions into live game objects. The spawner is
// deliberately independent from the JSON loader: by the time it is called,
// a prefab has already been resolved from the shared or map-local registry.
class PrefabSpawner {
public:
    PrefabSpawner(
        PhysicsWorld& physicsWorld,
        GameObjectFactory& objectFactory,
        WorldObjectStore& objectStore,
        GameWorld& gameWorld,
        TerrainSeamFilter& terrainSeamFilter,
        float cellSize,
        std::size_t playerCount
    );

    std::shared_ptr<GameObject> spawnAtGrid(
        const SpawnSpec& spec,
        int column,
        int screenRow,
        const sf::Vector2f& cellCenter
    );

    // Used by checkpoint respawning when the object's saved position takes
    // precedence over its original grid cell.
    std::shared_ptr<GameObject> spawnAtPosition(
        const SpawnSpec& spec,
        const sf::Vector2f& position
    );

private:
    std::shared_ptr<GameObject> spawnPipe(
        const SpawnSpec& spec,
        int column,
        int screenRow,
        const sf::Vector2f& cellCenter,
        const sf::Vector2f& spawnPosition
    );

    std::shared_ptr<GameObject> spawnObjectAtPosition(
        const SpawnSpec& spec,
        const sf::Vector2f& position,
        int column,
        int screenRow
    );

    PhysicsWorld& _physicsWorld;
    GameObjectFactory& _objectFactory;
    WorldObjectStore& _objectStore;
    GameWorld& _gameWorld;
    TerrainSeamFilter& _terrainSeamFilter;
    float _cellSize;
    std::size_t _playerCount;
};

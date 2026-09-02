#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Game/Objects/GameObjectFactory.h"
#include "Game/Objects/Projectile/FireballPool.h"
#include "Game/World/WorldInteractionSystem.h"
#include "Game/World/WorldMap.h"
#include "Game/World/WorldObjectStore.h"
#include "Game/World/WorldRenderer.h"
#include "Game/World/BlockBreakEffect.h"
#include "Game/ScoreManager.h"
#include "Physics/PhysicsWorld.h"

class Player;

class GameWorld {
public:
    explicit GameWorld(
        int gridWidth = WorldMap::defaultGridWidth,
        int gridHeight = WorldMap::defaultGridHeight,
        float cellSize = WorldMap::defaultCellSize
    );
    ~GameWorld();

    void handleInput(const sf::Event& event);
    void updateSimulation(const float& fixedDt);
    void updateVisuals(float deltaTime);
    void render(sf::RenderTarget& target);
    void handleContacts(b2ContactEvents contactEvents);
    void handleSensors(b2SensorEvents sensorEvents);

    void loadLevel(const std::string& levelPath);
    void loadMap(const LevelData& levelData);
    void saveCheckpoint(sf::Vector2f position);
    void respawnPlayer(
        const std::string& targetCharacter = "",
        std::optional<sf::Vector2f> customSpawnPos = std::nullopt
    );
    void reachFlagpole(sf::Vector2f position);

    bool spawnFireball(sf::Vector2f spawnPos, bool facingRight, int playerIndex);
    bool spawnKoopaShell(sf::Vector2f spawnPos, bool facingRight);
    bool spawnKoopa(sf::Vector2f spawnPos, bool facingRight);
    bool tryWarpPlayer(Player& player);

    struct LevelWarpRequest {
        std::string targetLevel;
        int targetWarpID = -1;
        float delaySeconds = 0.70f;
    };
    std::optional<LevelWarpRequest> takeLevelWarpRequest();
    bool emergePlayerFromPipe(int targetWarpID);
    void spawnBlockBreakEffect(
        sf::Vector2f position,
        sf::Vector2f blockSize,
        const sf::Texture* texture,
        sf::IntRect textureRect
    );
    static sf::Vector2f defaultItemSize(
        const std::string& itemTypeKey
    ) noexcept;
    std::shared_ptr<GameObject> spawnItem(
        const std::string& itemTypeKey,
        sf::Vector2f position,
        sf::Vector2f size = {0.0f, 0.0f}
    );
    void freeze(float durationSeconds);
    void releaseFreeze();
    bool isFrozen() const { return _freezeTimer > 0.0f; }
    void syncPlayerControllers();
    void playVictoryAnimation();
    std::string nextRuntimeSaveId();
    void removeObject(const std::shared_ptr<GameObject>& object);
    const std::vector<sf::Vector2i>& getDestroyedTileCells() const noexcept {
        return _worldMap.getDestroyedTileCells();
    }
    void restoreDestroyedTileCells(const std::vector<sf::Vector2i>& cells) {
        _worldMap.restoreDestroyedTileCells(cells);
    }
    std::optional<sf::Vector2f> getCheckpointPosition() const noexcept {
        return _checkpointPos ? std::optional<sf::Vector2f>{*_checkpointPos}
                              : std::nullopt;
    }
    void restoreCheckpoint(std::optional<sf::Vector2f> position) {
        _checkpointPos = position
            ? std::make_shared<sf::Vector2f>(*position)
            : nullptr;
    }
    void restoreLevelCleared(bool cleared) noexcept { _levelCleared = cleared; }

    int getGridWidth() const { return _worldMap.getGridWidth(); }
    int getGridHeight() const { return _worldMap.getGridHeight(); }
    float getCellSize() const { return _worldMap.getCellSize(); }
    std::shared_ptr<GameObject> getPrimaryPlayer() const;
    bool hasLivingPlayers() const;
    std::vector<std::shared_ptr<Player>> getPlayers() const;
    std::vector<std::shared_ptr<Player>> getLivingPlayers() const;
    bool hasWon() const { return _levelCleared; }
    sf::FloatRect getBounds() const;
    const std::string& getLevelMusic() const { return _currentLevelData.music; }
    const std::string& getLevelTheme() const { return _currentLevelData.theme; }
    const std::vector<std::shared_ptr<GameObject>>& objects() const { return _objectStore.objects(); }

    // Add getter & setter for ScoreManager
    void setScoreManager(ScoreManager* scoreManager) { _scoreManager = scoreManager; }
    ScoreManager* getScoreManager() const { return _scoreManager; }
    int getLives() const { return _scoreManager ? _scoreManager->getLives() : 0; }

private:
    LevelData _currentLevelData;
    ScoreManager* _scoreManager = nullptr;
    float _freezeTimer = 0.0f;
    bool _levelCleared = false;
    std::shared_ptr<sf::Vector2f> _checkpointPos = nullptr;
    std::optional<LevelWarpRequest> _pendingLevelWarp;
    std::uint64_t _nextRuntimeObjectId = 0;
    PhysicsWorld _physicsWorld;
    GameObjectFactory _objectFactory;
    std::array<FireballPool, 2> _fireballPools;
    std::vector<BlockBreakEffect> _blockBreakEffects;
    WorldObjectStore _objectStore;
    WorldMap _worldMap;
    WorldInteractionSystem _interactionSystem;
    WorldRenderer _renderer;
};

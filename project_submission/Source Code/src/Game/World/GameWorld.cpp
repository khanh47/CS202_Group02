#include "Game/World/GameWorld.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <stdexcept>

#include "Game/Behaviours/Animatable.h"
#include "Game/AI/HeuristicAiController.h"
#include "Game/Behaviours/Invincible.h"
#include "Game/GameSettings.h"
#include "Game/Objects/Player/Player.h"
#include "Game/Objects/Pipe/Pipe.h"
#include "Game/Objects/Projectile/KoopaShell.h"
#include "Game/Objects/Enemy/ConcreteEnemy/Koopa.h"
#include "Game/World/LevelDataLoader.h"
#include "Game/World/PrefabSpawner.h"

namespace {
struct PipeWarpPoints {
    sf::Vector2f outside;
    sf::Vector2f inside;
};

PipeWarpPoints makePipeWarpPoints(
    const Pipe& pipe,
    const sf::Vector2f& playerSize
) {
    const sf::Vector2f pipePosition = pipe.getPosition();
    const sf::Vector2f pipeSize = pipe.getHitboxPixels();
    sf::Vector2f opening = pipePosition;
    sf::Vector2f inward{0.0f, 0.0f};

    switch (pipe.getEndSide()) {
        case Pipe::EndSide::Top:
            opening.y -= pipeSize.y * 0.5f;
            inward.y = 1.0f;
            break;
        case Pipe::EndSide::Bottom:
            opening.y += pipeSize.y * 0.5f;
            inward.y = -1.0f;
            break;
        case Pipe::EndSide::Left:
            opening.x -= pipeSize.x * 0.5f;
            inward.x = 1.0f;
            break;
        case Pipe::EndSide::Right:
            opening.x += pipeSize.x * 0.5f;
            inward.x = -1.0f;
            break;
    }

    const bool vertical = pipe.getOrientation() == Pipe::Orientation::Vertical;
    const float playerHalfAxis = vertical
        ? playerSize.y * 0.5f
        : playerSize.x * 0.5f;
    constexpr float outsideGapPixels = 4.0f;
    constexpr float insideGapPixels = 16.0f;

    return {
        opening - inward * (playerHalfAxis + outsideGapPixels),
        opening + inward * (playerHalfAxis + insideGapPixels)
    };
}
}

GameWorld::GameWorld(int gridWidth, int gridHeight, float cellSize)
    : _worldMap(gridWidth, gridHeight, cellSize) {}

GameWorld::~GameWorld() = default;

void GameWorld::handleInput(const sf::Event& event) {
    if (GameSettings::getInstance().freeCameraMove || isFrozen()) {
        return;
    }
    _objectStore.handleInput(event);
}

void GameWorld::updateSimulation(const float& fixedDt) {
    if (isFrozen()) {
        _freezeTimer -= fixedDt;
        if (_freezeTimer <= 0.0f) {
            _freezeTimer = 0.0f;
            syncPlayerControllers();
        }
        return;
    }

    if (GameSettings::getInstance().freeCameraMove) {
        _objectStore.suspendPlayerMotion();
    }

    if (_levelCleared) {
        return;
    }

    _objectStore.updateSimulation(fixedDt);

    // A player can start a pipe warp from inside updateSimulation. Do not
    // advance Box2D or the remaining world objects on that same tick.
    if (isFrozen()) {
        return;
    }

    constexpr float maximumFireballDistance = 1280.0f;
    const float voidThreshold =
        _worldMap.getGridHeight() * _worldMap.getCellSize();
    for (FireballPool& pool : _fireballPools) {
        pool.updateSimulation(
            fixedDt,
            maximumFireballDistance,
            voidThreshold
        );
    }

    _physicsWorld.updateSimulation(fixedDt);

    // Events must be consumed while every fixture owner is still alive.
    handleSensors(_physicsWorld.getSensorEvents());
    handleContacts(_physicsWorld.getContactEvents());

    // Pipe segments are marked inactive during contact callbacks, but their
    // Box2D shapes must be destroyed after the buffered events are consumed.
    // Flush now so the broken segment has no physical hitbox before the next
    // simulation step.
    for (const std::shared_ptr<GameObject>& object : _objectStore.objects()) {
        if (auto pipe = std::dynamic_pointer_cast<Pipe>(object)) {
            pipe->flushBrokenSegments();
        }
    }

    // Falling past the loaded world is a lethal void hazard for every state,
    // including Mega. This deliberately uses Player::destroy() rather than
    // the state-level enemy immunity path.
    for (const std::shared_ptr<GameObject>& object : _objectStore.objects()) {
        if (auto player = std::dynamic_pointer_cast<Player>(object);
            player && player->getPosition().y > voidThreshold) {
            player->destroy();
        }
    }

    if (_levelCleared) {
        _objectStore.cleanupDestroyed();
        _worldMap.cleanupDestroyedTiles();
        return;
    }

    _objectStore.finalizeSimulation(fixedDt);
    _objectStore.cleanupDestroyed();
    _worldMap.cleanupDestroyedTiles();
}

void GameWorld::updateVisuals(float deltaTime) {
    if (_pendingLevelWarp) {
        _pendingLevelWarp->delaySeconds -= deltaTime;
    }

    _worldMap.updateVisuals(deltaTime);

    bool pipeWarpActive = false;
    for (const std::shared_ptr<GameObject>& object : _objectStore.objects()) {
        if (const auto player = std::dynamic_pointer_cast<Player>(object);
            player && player->isPipeWarping()) {
            pipeWarpActive = true;
            break;
        }
    }

    if (pipeWarpActive) {
        // Keep the world visually still while allowing the warping player's
        // dive, hidden transfer, and emergence to continue.
        for (const std::shared_ptr<GameObject>& object : _objectStore.objects()) {
            if (const auto player = std::dynamic_pointer_cast<Player>(object);
                player && player->isPipeWarping()) {
                player->updateVisuals(deltaTime);
            }
        }
        return;
    }

    _objectStore.updateVisuals(deltaTime);
    for (FireballPool& pool : _fireballPools) {
        pool.updateVisuals(deltaTime);
    }
    for (BlockBreakEffect& effect : _blockBreakEffects) {
        effect.update(deltaTime);
    }
    std::erase_if(
        _blockBreakEffects,
        [](const BlockBreakEffect& effect) { return effect.isFinished(); }
    );
}

void GameWorld::render(sf::RenderTarget& target) {
    _renderer.render(
        target,
        _worldMap,
        _objectStore,
        _fireballPools
    );
    for (const BlockBreakEffect& effect : _blockBreakEffects) {
        effect.render(target);
    }
}

void GameWorld::handleContacts(b2ContactEvents contactEvents) {
    _interactionSystem.processContacts(contactEvents);
}

void GameWorld::handleSensors(b2SensorEvents sensorEvents) {
    _interactionSystem.processSensors(sensorEvents);
}

#include "ResourceManager.h"
#include "Audio/MusicManager.h"
#include "Game/UserInput/PlayerController.h"

void GameWorld::loadMap(const LevelData& levelData) {
    _currentLevelData = levelData;
    _nextRuntimeObjectId = 0;
    _levelCleared = false;
    _checkpointPos = nullptr;
    _blockBreakEffects.clear();
    _worldMap.rebuild(
        levelData,
        _physicsWorld,
        _objectFactory,
        _fireballPools,
        _objectStore,
        *this
    );
}

void GameWorld::spawnBlockBreakEffect(
    sf::Vector2f position,
    sf::Vector2f blockSize,
    const sf::Texture* texture,
    sf::IntRect textureRect
) {
    BlockBreakEffect effect;
    effect.spawn(position, blockSize, texture, textureRect);
    _blockBreakEffects.push_back(std::move(effect));
}

void GameWorld::loadLevel(const std::string& levelPath) {
    loadMap(LevelDataLoader::load(
        levelPath,
        _worldMap.getGridWidth() * 2,
        _worldMap.getGridHeight() * 2
    ));
    for (const std::shared_ptr<GameObject>& object : _objectStore.objects()) {
        if (auto player = std::dynamic_pointer_cast<Player>(object)) {
            player->setGameWorld(*this);
        }
    }
    for (FireballPool& pool : _fireballPools) {
        pool.setGameWorld(this);
    }

    const GameSettings& settings = GameSettings::getInstance();
    if (settings.gameMode == GameMode::Minigame
        && settings.minigameMode == MinigameMode::VsAi) {
        std::shared_ptr<Player> mario;
        std::shared_ptr<Player> luigi;
        const std::vector<std::shared_ptr<Player>> players =
            _objectStore.getPlayers();
        for (const std::shared_ptr<Player>& player : players) {
            if (player->getCharacter() == "mario") {
                mario = player;
            } else if (player->getCharacter() == "luigi") {
                luigi = player;
            }
        }
        if (!mario || !luigi || players.size() != 2) {
            throw std::runtime_error(
                "VS AI levels must contain exactly one Mario and one Luigi"
            );
        }
        _objectStore.addAiController(
            std::make_unique<HeuristicAiController>(*luigi, *mario, *this)
        );
    }
}

void GameWorld::saveCheckpoint(sf::Vector2f position) {
    _checkpointPos = make_shared<sf::Vector2f>(position);
}

void GameWorld::respawnPlayer(
    const std::string& targetCharacter,
    std::optional<sf::Vector2f> customSpawnPos
) {
    PrefabSpawner spawner(
        _physicsWorld,
        _objectFactory,
        _objectStore,
        *this,
        _worldMap.getTerrainSeamFilter(),
        _worldMap.getCellSize(),
        _worldMap.getPlayerCount()
    );

    const std::vector<std::string>& layer = _currentLevelData.layer;
    for (int mapRow = 0; mapRow < _worldMap.getLoadedRows(); ++mapRow) {
        const int columns = std::min(
            static_cast<int>(layer[mapRow].size()),
            _worldMap.getLoadedColumns()
        );
        for (int column = 0; column < columns; ++column) {
            const auto mappingIt = _currentLevelData.tileMapping.find(
                layer[mapRow][column]
            );
            if (mappingIt == _currentLevelData.tileMapping.end()) {
                continue;
            }

            const SpawnSpec spec = _currentLevelData.prefabs.resolve(
                mappingIt->second
            );
            if (!spec.objectKind || *spec.objectKind != ObjectKind::Player) {
                continue;
            }

            const bool isLuigi = spec.animationId.find("luigi") != std::string::npos;
            const std::string charName = isLuigi ? "luigi" : "mario";
            if (!targetCharacter.empty() && targetCharacter != charName) {
                continue;
            }

            if (_scoreManager) {
                if (isLuigi && _scoreManager->getLuigiLives() <= 0) {
                    continue;
                }
                if (!isLuigi && _scoreManager->getLives() <= 0) {
                    continue;
                }
            }

            std::optional<sf::Vector2f> spawnCoord = customSpawnPos;
            if (!spawnCoord && _checkpointPos) {
                spawnCoord = *_checkpointPos;
            }

            const sf::Vector2f cellPosition = _worldMap.mapCellCenter(
                column,
                mapRow
            );
            const std::shared_ptr<GameObject> object = spawnCoord
                ? spawner.spawnAtPosition(spec, *spawnCoord)
                : spawner.spawnAtGrid(
                    spec,
                    column,
                    WorldMap::screenRowFor(
                        mapRow,
                        _worldMap.getLoadedRows(),
                        _worldMap.getGridHeight()
                    ),
                    cellPosition
                );
            if (object) {
                object->setSaveId(
                    "cell:" + std::to_string(column) + ":"
                    + std::to_string(
                        WorldMap::screenRowFor(
                            mapRow,
                            _worldMap.getLoadedRows(),
                            _worldMap.getGridHeight()
                        )
                    )
                );
                object->addBehaviour<Invincible>(3.0f);
                if (auto player = std::dynamic_pointer_cast<Player>(object)) {
                    player->setGameWorld(*this);
                }
            }
        }
    }
}

bool GameWorld::spawnFireball(
    sf::Vector2f spawnPosition,
    bool facingRight,
    int playerIndex
) {
    if (playerIndex < 0
        || playerIndex >= static_cast<int>(_fireballPools.size())) {
        return false;
    }
    return _fireballPools[playerIndex].spawnFireball(
        spawnPosition,
        facingRight,
        playerIndex
    );
}

bool GameWorld::spawnKoopaShell(sf::Vector2f spawnPosition, bool facingRight) {
    sf::Texture& itemsTexture = ResourceManager::getInstance().getTexture("koopa_spritesheet");
    auto shell = std::make_shared<KoopaShell>(itemsTexture);
    shell->setFacingRight(facingRight);
    shell->setSaveId(nextRuntimeSaveId());
    shell->setGameWorld(this);
    shell->spawn(_physicsWorld, spawnPosition, {60.0f, 48.0f});
    _objectStore.addObject(std::move(shell));
    return true;
}

bool GameWorld::spawnKoopa(sf::Vector2f spawnPosition, bool facingRight) {
    sf::Texture& itemsTexture = ResourceManager::getInstance().getTexture("koopa_spritesheet");
    // Bottom-align the koopa (100px tall) with the shell (48px tall) so it spawns resting on the ground.
    spawnPosition.y -= (100.0f - 48.0f) * 0.5f;
    auto koopa = std::make_shared<Koopa>(itemsTexture, "koopa", true);
    koopa->setGameWorld(this);
    koopa->setFacingRight(facingRight);
    koopa->setSaveId(nextRuntimeSaveId());
    koopa->setSupportGrid(&_worldMap.getTerrainSeamFilter(), _worldMap.getCellSize());
    koopa->spawn(_physicsWorld, spawnPosition, {64.0f, 100.0f});
    _objectStore.addObject(std::move(koopa));
    return true;
}

bool GameWorld::tryWarpPlayer(Player& player) {
    const sf::Vector2f playerPosition = player.getPosition();
    const sf::Vector2f playerSize = player.getHitboxPixels();
    const float halfWidth = playerSize.x * 0.5f;
    const float halfHeight = playerSize.y * 0.5f;
    const Pipe* source = nullptr;

    for (const auto& object : _objectStore.objects()) {
        const auto pipe = std::dynamic_pointer_cast<Pipe>(object);
        if (!pipe || !pipe->isWarp()) continue;
        const sf::Vector2f pipePosition = pipe->getPosition();
        const sf::Vector2f pipeSize = pipe->getHitboxPixels();
        const float left = pipePosition.x - pipeSize.x * 0.5f;
        const float right = pipePosition.x + pipeSize.x * 0.5f;
        const float top = pipePosition.y - pipeSize.y * 0.5f;
        const float bottom = pipePosition.y + pipeSize.y * 0.5f;
        const bool verticalOverlap = playerPosition.x + halfWidth > left + 4.0f
            && playerPosition.x - halfWidth < right - 4.0f;
        const bool horizontalOverlap = playerPosition.y + halfHeight > top + 4.0f
            && playerPosition.y - halfHeight < bottom - 4.0f;
        const sf::Vector2f velocity = player.getVelocity();
        bool entering = false;
        switch (pipe->getEndSide()) {
            case Pipe::EndSide::Top:
                entering = verticalOverlap && std::abs(playerPosition.y + halfHeight - top) < 20.0f && player.isMoveDownHeld();
                break;
            case Pipe::EndSide::Bottom:
                entering = verticalOverlap && std::abs(playerPosition.y - halfHeight - bottom) < 20.0f && player.isMoveUpHeld();
                break;
            case Pipe::EndSide::Left:
                entering = horizontalOverlap && std::abs(playerPosition.x + halfWidth - left) < 20.0f && velocity.x > 20.0f;
                break;
            case Pipe::EndSide::Right:
                entering = horizontalOverlap && std::abs(playerPosition.x - halfWidth - right) < 20.0f && velocity.x < -20.0f;
                break;
        }
        if (entering) {
            source = pipe.get();
            break;
        }
    }
    if (!source) return false;

    if (!source->getWarpLevel().empty()) {
        _pendingLevelWarp = LevelWarpRequest{
            source->getWarpLevel(),
            source->getWarpTarget(),
            0.70f
        };
        const PipeWarpPoints sourcePoints = makePipeWarpPoints(*source, playerSize);
        player.beginPipeWarp(
            sourcePoints.outside,
            sourcePoints.inside,
            sourcePoints.inside,
            sourcePoints.inside
        );
        freeze(Player::pipeWarpDurationSeconds);
        return true;
    }

    const Pipe* destination = nullptr;
    for (const auto& object : _objectStore.objects()) {
        const auto pipe = std::dynamic_pointer_cast<Pipe>(object);
        if (pipe && pipe->isWarp() && pipe.get() != source
            && source->getWarpTarget() >= 0
            && pipe->getWarpID() == source->getWarpTarget()) {
            destination = pipe.get();
            break;
        }
    }
    if (!destination) return false;

    const PipeWarpPoints sourcePoints = makePipeWarpPoints(
        *source,
        playerSize
    );
    const PipeWarpPoints destinationPoints = makePipeWarpPoints(
        *destination,
        playerSize
    );

    if (!player.beginPipeWarp(
            sourcePoints.outside,
            sourcePoints.inside,
            destinationPoints.inside,
            destinationPoints.outside
        )) {
        return false;
    }

    freeze(Player::pipeWarpDurationSeconds);
    return true;
}

std::optional<GameWorld::LevelWarpRequest> GameWorld::takeLevelWarpRequest() {
    if (_pendingLevelWarp && _pendingLevelWarp->delaySeconds <= 0.0f) {
        auto req = std::move(_pendingLevelWarp);
        _pendingLevelWarp.reset();
        return req;
    }
    return std::nullopt;
}

bool GameWorld::emergePlayerFromPipe(int targetWarpID) {
    auto player = std::dynamic_pointer_cast<Player>(getPrimaryPlayer());
    if (!player) return false;

    const Pipe* destination = nullptr;
    for (const auto& object : _objectStore.objects()) {
        const auto pipe = std::dynamic_pointer_cast<Pipe>(object);
        if (pipe && pipe->isWarp() && pipe->getWarpID() == targetWarpID) {
            destination = pipe.get();
            break;
        }
    }
    if (!destination) return false;

    const PipeWarpPoints destinationPoints = makePipeWarpPoints(
        *destination,
        player->getHitboxPixels()
    );

    if (!player->beginPipeWarp(
            destinationPoints.inside,
            destinationPoints.inside,
            destinationPoints.inside,
            destinationPoints.outside
        )) {
        return false;
    }

    freeze(Player::pipeWarpDurationSeconds);
    return true;
}

std::shared_ptr<GameObject> GameWorld::spawnItem(
    const std::string& itemTypeKey,
    sf::Vector2f position,
    sf::Vector2f size
) {
    if (size.x <= 0.0f || size.y <= 0.0f) {
        size = defaultItemSize(itemTypeKey);
    }

    sf::Texture* texture = nullptr;
    auto& resources = ResourceManager::getInstance();
    if (itemTypeKey == "Coin") {
        texture = &resources.getTexture("coin_spritesheet");
    } else if (itemTypeKey == "Flagpole") {
        texture = &resources.getTexture("goal_flag_spritesheet");
    } else if (itemTypeKey == "CheckpointFlag") {
        texture = &resources.getTexture("checkpoint_flag_spritesheet");
    } else if (itemTypeKey == "MegaMushroom") {
        texture = &resources.getTexture("mega_mushroom_spritesheet");
    } else {
        texture = &resources.getTexture("mario_and_items");
    }

    try {
        auto item = _objectFactory.createItem(itemTypeKey, texture);
        if (item) {
            item->setSaveId(nextRuntimeSaveId());
            item->spawn(_physicsWorld, position, size);
            _objectStore.addObject(item);
        }
        return item;
    } catch (...) {
        return nullptr;
    }
}

void GameWorld::freeze(float durationSeconds) {
    _freezeTimer = std::max(_freezeTimer, durationSeconds);
}

void GameWorld::releaseFreeze() {
    _freezeTimer = 0.0f;
    syncPlayerControllers();
}

void GameWorld::reachFlagpole(sf::Vector2f position) {
    if (_levelCleared) {
        return;
    }

    _levelCleared = true;

    if (_scoreManager) {
        _scoreManager->handleEvent(ScoreEventType::FlagpoleReached, position);
        // Play course clear music (non-looping)
        Audio::MusicManager::getInstance().setVolume(GameSettings::getInstance().musicVolume);
        Audio::MusicManager::getInstance().play("course_clear", false);
    }
}

sf::Vector2f GameWorld::defaultItemSize(
    const std::string& itemTypeKey
) noexcept {
    if (itemTypeKey == "MegaMushroom") {
        return {384.0f, 308.0f};
    }
    if (itemTypeKey == "MegaCoin") {
        return {96.0f, 96.0f};
    }
    return {54.0f, 54.0f};
}

void GameWorld::syncPlayerControllers() {
    _objectStore.syncControllersWithKeyboard();
}

void GameWorld::playVictoryAnimation() {
    for (const std::shared_ptr<GameObject>& object : _objectStore.objects()) {
        if (auto player = std::dynamic_pointer_cast<Player>(object)) {
            if (auto* animatable = player->getBehaviour<Animatable>()) {
                animatable->playAnimation("victory");
            }
        }
    }
}

std::shared_ptr<GameObject> GameWorld::getPrimaryPlayer() const {
    return _objectStore.getPrimaryPlayer();
}

std::string GameWorld::nextRuntimeSaveId() {
    return "runtime:" + std::to_string(_nextRuntimeObjectId++);
}

void GameWorld::removeObject(const std::shared_ptr<GameObject>& object) {
    _objectStore.removeObject(object);
}

bool GameWorld::hasLivingPlayers() const {
    return _objectStore.hasLivingPlayers();
}

std::vector<std::shared_ptr<Player>> GameWorld::getPlayers() const {
    return _objectStore.getPlayers();
}

std::vector<std::shared_ptr<Player>> GameWorld::getLivingPlayers() const {
    return _objectStore.getLivingPlayers();
}

sf::FloatRect GameWorld::getBounds() const {
    return _worldMap.getBounds();
}

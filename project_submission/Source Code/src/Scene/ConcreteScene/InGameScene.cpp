#include <memory>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "Scene/ConcreteScene/InGameScene.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Behaviours/Damageable.h"
#include "Game/Objects/Block/Block.h"
#include "Game/Objects/Block/CoinBlock.h"
#include "Game/Objects/Block/LuckyBlock.h"
#include "Game/Objects/Enemy/ConcreteEnemy/Goomba.h"
#include "Game/Objects/Enemy/ConcreteEnemy/Koopa.h"
#include "Game/Objects/Enemy/ConcreteEnemy/PiranhaPlant.h"
#include "Game/Objects/Enemy/Enemy.h"
#include "Game/Objects/Item/ConcreteItems/CheckpointFlag.h"
#include "Game/Objects/Item/ConcreteItems/Coin.h"
#include "Game/Objects/Item/ConcreteItems/FireFlower.h"
#include "Game/Objects/Item/ConcreteItems/Flagpole.h"
#include "Game/Objects/Item/ConcreteItems/MegaCoin.h"
#include "Game/Objects/Item/ConcreteItems/MegaMushroom.h"
#include "Game/Objects/Item/ConcreteItems/OneUpMushroom.h"
#include "Game/Objects/Item/ConcreteItems/SuperMushroom.h"
#include "Game/Objects/Item/ConcreteItems/SuperStar.h"
#include "Game/Objects/Item/Item.h"
#include "Game/Objects/Pipe/Pipe.h"
#include "Game/Objects/Player/Player.h"
#include "Game/Objects/Projectile/Fireball.h"
#include "Game/Objects/Projectile/KoopaShell.h"
#include "Scene/ConcreteScene/ScoreComputationScene.h"
#include "Scene/ConcreteScene/SaveGameScene.h"
#include "Commands/FunctionalCommand.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "ResourceManager.h"
#include "Audio/MusicManager.h"
#include "Audio/SoundManager.h"
#include "Scene/SceneManager.h"
#include "Game/GameSettings.h"
#include "Game/LeaderboardManager.h"
#include <iostream>

namespace {
using json = nlohmann::json;

constexpr sf::IntRect pauseButtonNormalRect({256, 0}, {16, 16});
constexpr sf::IntRect pauseButtonHoverRect({256, 16}, {16, 16});
constexpr sf::Vector2f pauseButtonPosition{1824.f, 32.f};
constexpr sf::Vector2f pauseButtonScale{4.f, 4.f};

json vectorToJson(const sf::Vector2f& value) {
    return {value.x, value.y};
}

sf::Vector2f vectorFromJson(
    const json& value,
    sf::Vector2f fallback = {}
) {
    if (!value.is_array() || value.size() < 2) {
        return fallback;
    }
    return {
        value[0].get<float>(),
        value[1].get<float>()
    };
}

std::string gameModeToString(GameMode mode) {
    return mode == GameMode::Solo ? "solo" : "coop";
}

GameMode gameModeFromString(const std::string& value) {
    return value == "solo" ? GameMode::Solo : GameMode::Coop;
}

std::string saveObjectType(const GameObject& object) {
    if (dynamic_cast<const Player*>(&object)) return "player";
    if (dynamic_cast<const CoinBlock*>(&object)) return "coinBlock";
    if (dynamic_cast<const LuckyBlock*>(&object)) return "luckyBlock";
    if (dynamic_cast<const Block*>(&object)) return "block";
    if (dynamic_cast<const Goomba*>(&object)) return "goomba";
    if (dynamic_cast<const Koopa*>(&object)) return "koopa";
    if (dynamic_cast<const PiranhaPlant*>(&object)) return "piranhaPlant";
    if (dynamic_cast<const Enemy*>(&object)) return "enemy";
    if (dynamic_cast<const Coin*>(&object)) return "Coin";
    if (dynamic_cast<const SuperMushroom*>(&object)) return "SuperMushroom";
    if (dynamic_cast<const FireFlower*>(&object)) return "FireFlower";
    if (dynamic_cast<const OneUpMushroom*>(&object)) return "OneUpMushroom";
    if (dynamic_cast<const SuperStar*>(&object)) return "SuperStar";
    if (dynamic_cast<const MegaMushroom*>(&object)) return "MegaMushroom";
    if (dynamic_cast<const MegaCoin*>(&object)) return "MegaCoin";
    if (dynamic_cast<const CheckpointFlag*>(&object)) return "CheckpointFlag";
    if (dynamic_cast<const Flagpole*>(&object)) return "Flagpole";
    if (dynamic_cast<const Item*>(&object)) return "item";
    if (dynamic_cast<const Pipe*>(&object)) return "pipe";
    if (dynamic_cast<const KoopaShell*>(&object)) return "koopaShell";
    if (dynamic_cast<const Fireball*>(&object)) return "fireball";
    return "object";
}

bool isPersistedObjectType(const std::string& type) {
    return type == "player"
        || type == "coinBlock"
        || type == "luckyBlock"
        || type == "block"
        || type == "goomba"
        || type == "koopa"
        || type == "piranhaPlant"
        || type == "enemy"
        || type == "Coin"
        || type == "SuperMushroom"
        || type == "FireFlower"
        || type == "OneUpMushroom"
        || type == "SuperStar"
        || type == "MegaMushroom"
        || type == "MegaCoin"
        || type == "CheckpointFlag"
        || type == "Flagpole"
        || type == "item"
        || type == "pipe";
}

bool isItemType(const std::string& type) {
    return type == "Coin"
        || type == "SuperMushroom"
        || type == "FireFlower"
        || type == "OneUpMushroom"
        || type == "SuperStar"
        || type == "MegaMushroom"
        || type == "MegaCoin"
        || type == "CheckpointFlag"
        || type == "Flagpole"
        || type == "item";
}

std::string levelThemeFor(
    const std::string& levelName,
    const std::string& selectedMusic
) {
    if (!selectedMusic.empty()) {
        return selectedMusic;
    }

    if (levelName.find("map-2") != std::string::npos
        || levelName.find("map-3") != std::string::npos) {
        return "underground_theme";
    }
    return "ground_theme";
}
}

InGameScene::InGameScene(
    const std::string& name,
    std::optional<nlohmann::json> initialSaveState,
    bool returnToMapEditor
)
    : Scene(name),
      _initialSaveState(std::move(initialSaveState)),
      _returnToMapEditor(returnToMapEditor),
      _currentLoadedLevel(name) {
    _settingsPanel.setOnBack([this]() {
        _pauseOverlay = PauseOverlay::PauseMenu;
    });
}

void InGameScene::init() {
    _pauseOverlay = PauseOverlay::None;
    _suppressExitSnapshot = false;
    _winReactionActive = false;
    _gameOverActive = false;
    _winActive = false;
    _minigameWinner.reset();
    _minigameParticipantCount = 0;
    _gameOverTexture = &ResourceManager::getInstance().getTexture("game_over");
    _gameOverOverlay.emplace(*_gameOverTexture);
    sf::Texture& pauseButtonTexture =
        ResourceManager::getInstance().getTexture("square_premade_buttons");
    pauseButtonTexture.setSmooth(false);
    _pauseButton.emplace(pauseButtonTexture, pauseButtonNormalRect);
    _pauseButton->setPosition(pauseButtonPosition);
    _pauseButton->setScale(pauseButtonScale);
    _pauseButtonHovered = false;
    _gameOverPrompt.emplace(
        ResourceManager::getInstance().getFont("SuperMario"),
        "Press any key to continue",
        67
    );
    _gameOverPrompt->setFillColor(sf::Color::White);
    _winTitle.emplace(
        ResourceManager::getInstance().getFont("SuperMario"),
        "COURSE CLEAR!",
        82
    );
    _winTitle->setFillColor(sf::Color(255, 227, 102));
    _winPrompt.emplace(
        ResourceManager::getInstance().getFont("SuperMario"),
        "Press any key to continue",
        67
    );
    _winPrompt->setFillColor(sf::Color::White);
    buildPauseMenu();
    buildReturnConfirmation();

    if (_initialSaveState) {
        const std::string savedMode = _initialSaveState->value(
            "gameMode",
            "coop"
        );
        GameSettings::getInstance().gameMode = gameModeFromString(savedMode);
        GameSettings::getInstance().player1Character =
            _initialSaveState->value(
                "player1Character",
                GameSettings::getInstance().player1Character
            );
    }

    _gameWorld.loadLevel(_name);
    if (GameSettings::getInstance().gameMode == GameMode::Minigame) {
        _minigameParticipantCount = _gameWorld.getPlayers().size();
        _scoreManager.setLives(1);
        _scoreManager.setLuigiLives(1);
    }
    _gameWorld.setScoreManager(&_scoreManager); // Set score manager for the game world
    _scoreManager.setHighScore(LeaderboardManager::getInstance().getHighScore(_name));
    if (_initialSaveState) {
        restoreSaveState(*_initialSaveState);
    }
    if (!_returnToMapEditor
        && GameSettings::getInstance().gameMode != GameMode::Minigame) {
        SaveLoadGame::getInstance().setCurrentSession(
            captureSaveState(),
            !_initialSaveState.has_value()
        );
    }
    _saveStateInitialized = true;

    // Configure 2D Platformer Camera System parameters
    CameraConfig config;
    config.deadzoneSize = {250.0f, 180.0f};          // Invisible rectangular deadzone box
    config.lookaheadDistance = 160.0f;               // Forward anticipation distance
    config.lookaheadSpeed = 3.5f;                    // Interpolation speed for lookahead transitions
    config.dampingX = 6.0f;                          // Horizontal smooth damping factor
    config.dampingY = 4.5f;                          // Vertical smooth damping factor
    config.yStabilizationEnabled = true;             // Ignore minor vertical hops/jumps
    config.yThreshold = 140.0f;                      // Vertical displacement threshold for Y tracking
    config.levelBounds = _gameWorld.getBounds();     // Clamp view within level limits
    config.useBounds = true;

    _camera.setConfig(config);

    // Bind camera tracking target(s) - 2-player center + zoom
    const auto players = _gameWorld.getPlayers();
    if (players.size() >= 2 && GameSettings::getInstance().gameMode != GameMode::Solo) {
        std::vector<std::shared_ptr<GameObject>> targets;
        targets.reserve(players.size());
        for (const auto& p : players) targets.push_back(p);
        _camera.setTargets(targets);
    } else if (auto player = _gameWorld.getPrimaryPlayer()) {
        _camera.setTarget(player);
    } else {
        _camera.setCenter({1920.f / 2.f, _gameWorld.getGridHeight() * _gameWorld.getCellSize() - 1080.f / 2.f});
    }
}

    void InGameScene::onEnter() {
        // Ensure title-screen music is stopped and play level theme
        _isActive = true;
        GameSettings::getInstance().isInGameSceneActive = true;
        GameSettings::getInstance().isLevelSelectActive = false;
        _starmanMusicActive = false;
        _scoreManager.setTimePaused(_pauseOverlay != PauseOverlay::None);
        stopTitleScreenMusic();
        Audio::MusicManager::getInstance().setVolume(GameSettings::getInstance().musicVolume);
        Audio::MusicManager::getInstance().play(
            levelThemeFor(_name, _gameWorld.getLevelMusic()),
            true
        );
    }

    void InGameScene::onExit() {
        if (!_suppressExitSnapshot
            && _saveStateInitialized
            && !_returnToMapEditor
            && GameSettings::getInstance().gameMode != GameMode::Minigame) {
            SaveLoadGame::getInstance().setCurrentSession(
                captureSaveState(),
                true
            );
        }
        // Stop any level music when leaving the scene
        _starmanMusicActive = false;
        GameSettings::getInstance().isInGameSceneActive = false;
        Audio::MusicManager::getInstance().stop();
        Scene::onExit();
    }

void InGameScene::handleInput(const sf::Event& event) {
    if (_winReactionActive) {
        return;
    }

    if (_winActive || _gameOverActive) {
        if (event.is<sf::Event::KeyPressed>()
            || event.is<sf::Event::MouseButtonPressed>()
            || event.is<sf::Event::JoystickButtonPressed>()) {

            ScoreSummaryData summary;
            summary.levelPath = _name;

            if (_name.find("map-1") != std::string::npos) {
                summary.levelName = "WORLD 1 - GRASSLAND";
            } else if (_name.find("map-2") != std::string::npos) {
                summary.levelName = "WORLD 2 - UNDERGROUND";
            } else if (_name.find("map-3") != std::string::npos) {
                summary.levelName = "WORLD 3 - CASTLE";
            } else if (_name.find("custom-map") != std::string::npos) {
                summary.levelName = "CUSTOM MAP";
            } else {
                summary.levelName = "LEVEL RESULTS";
            }

            if (auto player = std::dynamic_pointer_cast<Player>(_gameWorld.getPrimaryPlayer())) {
                summary.character = player->getCharacter();
            }
            summary.isWin = _winActive;
            summary.baseScore = _scoreManager.getScore();
            if (GameSettings::getInstance().gameMode == GameMode::Coop) {
                summary.coinsCollected = _scoreManager.getCoins() + _scoreManager.getLuigiCoins();
                summary.livesRemaining = _scoreManager.getLives() + _scoreManager.getLuigiLives();
            } else if (GameSettings::getInstance().player1Character == "luigi") {
                summary.coinsCollected = _scoreManager.getLuigiCoins();
                summary.livesRemaining = std::max(0, _scoreManager.getLuigiLives());
            } else {
                summary.coinsCollected = _scoreManager.getCoins();
                summary.livesRemaining = std::max(0, _scoreManager.getLives());
            }
            summary.timeRemaining = _scoreManager.getIntTimeRemaining();
            summary.highScore = LeaderboardManager::getInstance().getHighScore(_name);
            summary.returnToMapEditor = _returnToMapEditor;

            if (auto* manager = getSceneManager()) {
                manager->pushScene(std::make_unique<ScoreComputationScene>(summary));
            }
        }
        return;
    }

    if (_pauseOverlay == PauseOverlay::Settings) {
        _settingsPanel.handleInput(event);
        return;
    }

    if (_pauseOverlay == PauseOverlay::ReturnConfirmation) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
            keyEvent && keyEvent->code == sf::Keyboard::Key::Escape) {
            _pauseOverlay = PauseOverlay::PauseMenu;
            return;
        }
        _returnConfirmationMenu.processEvent(event);
        return;
    }

    if (_pauseOverlay == PauseOverlay::PauseMenu) {
        if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>();
            keyEvent && (keyEvent->code == sf::Keyboard::Key::Escape || keyEvent->code == sf::Keyboard::Key::P)) {
            resumeGame();
            return;
        }
        _pauseMenu.processEvent(event);
        return;
    }

    if (handlePauseButtonInput(event)) {
        return;
    }

    if (auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape || keyEvent->code == sf::Keyboard::Key::P) {
            openPauseMenu();
            return;
        }
    }

    _gameWorld.handleInput(event);
}

bool InGameScene::handlePauseButtonInput(const sf::Event& event) {
    if (!isPauseButtonVisible() || !_pauseButton) {
        return false;
    }

    if (const auto* mouseMove = event.getIf<sf::Event::MouseMoved>()) {
        const sf::Vector2f mousePosition{
            static_cast<float>(mouseMove->position.x),
            static_cast<float>(mouseMove->position.y)
        };
        setPauseButtonHovered(
            _pauseButton->getGlobalBounds().contains(mousePosition)
        );
        return false;
    }

    if (const auto* mousePress = event.getIf<sf::Event::MouseButtonPressed>();
        mousePress && mousePress->button == sf::Mouse::Button::Left) {
        const sf::Vector2f mousePosition{
            static_cast<float>(mousePress->position.x),
            static_cast<float>(mousePress->position.y)
        };
        if (_pauseButton->getGlobalBounds().contains(mousePosition)) {
            openPauseMenu();
            return true;
        }
    }

    return false;
}

bool InGameScene::isPauseButtonVisible() const noexcept {
    return _pauseOverlay == PauseOverlay::None
        && !_winReactionActive
        && !_winActive
        && !_gameOverActive
        && _gameWorld.hasLivingPlayers();
}

void InGameScene::setPauseButtonHovered(bool hovered) {
    if (!_pauseButton || _pauseButtonHovered == hovered) {
        return;
    }
    _pauseButtonHovered = hovered;
    _pauseButton->setTextureRect(
        hovered ? pauseButtonHoverRect : pauseButtonNormalRect
    );
}

void InGameScene::drawPauseButton(sf::RenderTarget& target) {
    if (isPauseButtonVisible() && _pauseButton) {
        target.draw(*_pauseButton);
    }
}

void InGameScene::buildPauseMenu() {
    _pauseMenu.clear();
    _pauseMenu.setLayoutProperties(
        {820.f, 300.f}, {280.f, 60.f}, 80.f, false,
        sf::Color(100, 149, 237), 36 
    );
    _pauseMenu.addMainMenuButtonAuto(
        "Resume",
        std::make_unique<FunctionalCommand>(
            "Resume", [this]() { resumeGame(); }
        )
    );
    _pauseMenu.addMainMenuButtonAuto(
        "Restart Level",
        std::make_unique<FunctionalCommand>(
            "Restart Level", [this]() { restartLevel(); }
        )
    );
    _pauseMenu.addMainMenuButtonAuto(
        "Settings",
        std::make_unique<FunctionalCommand>(
            "Settings", [this]() { openSettings(); }
        )
    );

    const bool canSave = !_returnToMapEditor
        && GameSettings::getInstance().gameMode != GameMode::Minigame;
    if (canSave) {
        _pauseMenu.addMainMenuButtonAuto(
            "Save Game",
            std::make_unique<FunctionalCommand>(
                "Save Game", [this]() { saveGame(); }
            )
        );
    }

    _pauseMenu.addMainMenuButtonAuto(
        _returnToMapEditor ? "Return to Map Editor" : "Quit to Main Menu",
        std::make_unique<FunctionalCommand>(
            "Return", [this]() { requestReturn(); }
        )
    );
}

void InGameScene::buildReturnConfirmation() {
    _returnConfirmationMenu.clear();
    _returnConfirmationMenu.setLayoutProperties(
        {820.f, 300.f}, {280.f, 60.f}, 80.f, false,
        sf::Color(100, 149, 237), 36 
    );
    _returnConfirmationMenu.addMainMenuButtonAuto(
        "Save and Quit",
        std::make_unique<FunctionalCommand>(
            "Save and Return",
            [this]() {
                SaveLoadGame::getInstance().setCurrentSession(
                    captureSaveState(), true
                );
                if (auto* manager = getSceneManager()) {
                    manager->pushScene(std::make_unique<SaveGameScene>(
                        false,
                        [this]() {
                            _suppressExitSnapshot = true;
                            if (auto* sceneManager = getSceneManager()) {
                                sceneManager->requestReturnToMainMenu();
                            }
                        }
                    ));
                }
            }
        )
    );
    _returnConfirmationMenu.addMainMenuButtonAuto(
        "Quit Without Saving",
        std::make_unique<FunctionalCommand>(
            "Discard and Return", [this]() { returnWithoutSaving(); }
        )
    );
    _returnConfirmationMenu.addMainMenuButtonAuto(
        "Cancel",
        std::make_unique<FunctionalCommand>(
            "Cancel", [this]() { _pauseOverlay = PauseOverlay::PauseMenu; }
        )
    );
}

void InGameScene::openPauseMenu() {
    Audio::SoundManager::getInstance().playEffect("select_button");
    buildPauseMenu();
    _pauseOverlay = PauseOverlay::PauseMenu;
    _scoreManager.setTimePaused(true);
}

void InGameScene::openSettings() {
    Audio::SoundManager::getInstance().playEffect("select_button");
    _settingsPanel.refresh();
    _pauseOverlay = PauseOverlay::Settings;
}

void InGameScene::resumeGame() {
    Audio::SoundManager::getInstance().playEffect("select_button");
    _pauseOverlay = PauseOverlay::None;
    _scoreManager.setTimePaused(false);
    _gameWorld.syncPlayerControllers();
}

void InGameScene::restartLevel() {
    Audio::SoundManager::getInstance().playEffect("select_button");
    _pauseOverlay = PauseOverlay::None;
    _gameOverActive = false;
    _winActive = false;
    _winReactionActive = false;
    _starmanMusicActive = false;

    _scoreManager.resetTime(400.0f);
    _scoreManager.restoreState(0, 0, 3, LeaderboardManager::getInstance().getHighScore(_name), 400.0f, 0, 3, 0, 0);
    if (GameSettings::getInstance().gameMode == GameMode::Minigame) {
        _scoreManager.setLives(1);
        _scoreManager.setLuigiLives(1);
    }
    _gameWorld.restoreCheckpoint(std::nullopt);
    _gameWorld.loadLevel(_name);
    _currentLoadedLevel = _name;
    const auto players = _gameWorld.getPlayers();
    if (players.size() >= 2 && GameSettings::getInstance().gameMode != GameMode::Solo) {
        std::vector<std::shared_ptr<GameObject>> targets;
        targets.reserve(players.size());
        for (const auto& p : players) targets.push_back(p);
        _camera.setTargets(targets);
    } else if (auto player = _gameWorld.getPrimaryPlayer()) {
        _camera.setTarget(player);
    } else {
        _camera.setCenter({1920.f / 2.f, _gameWorld.getGridHeight() * _gameWorld.getCellSize() - 1080.f / 2.f});
    }
    _scoreManager.setTimePaused(false);
    Audio::MusicManager::getInstance().play(
        levelThemeFor(_currentLoadedLevel, _gameWorld.getLevelMusic()),
        true
    );
}

void InGameScene::saveGame() {
    SaveLoadGame::getInstance().setCurrentSession(captureSaveState(), true);
    if (auto* manager = getSceneManager()) {
        manager->pushScene(std::make_unique<SaveGameScene>());
    }
}

void InGameScene::requestReturn() {
    if (auto* manager = getSceneManager()) {
        if (_returnToMapEditor) {
            _suppressExitSnapshot = true;
            manager->requestReturnToMapEditor();
        } else if (GameSettings::getInstance().gameMode == GameMode::Minigame) {
            _suppressExitSnapshot = true;
            manager->requestReturnToMainMenu();
        } else {
            _pauseOverlay = PauseOverlay::ReturnConfirmation;
        }
    }
}

void InGameScene::returnWithoutSaving() {
    _suppressExitSnapshot = true;
    SaveLoadGame::getInstance().clearCurrentSession();
    if (auto* manager = getSceneManager()) {
        manager->requestReturnToMainMenu();
    }
}

void InGameScene::drawPauseOverlay(sf::RenderTarget& target) {
    if (_pauseOverlay == PauseOverlay::Settings) {
        _settingsPanel.render(target);
        return;
    }

    const sf::View defaultView = target.getDefaultView();
    target.setView(defaultView);

    sf::RectangleShape backdrop({1920.f, 1080.f});
    backdrop.setPosition({0.f, 0.f});
    backdrop.setFillColor(sf::Color(0, 0, 0, 185));
    target.draw(backdrop);

    const sf::Font& font = ResourceManager::getInstance().getFont("SuperMario");
    const std::string titleText = _pauseOverlay == PauseOverlay::ReturnConfirmation
        ? "QUIT TO MAIN MENU?" : "GAME PAUSED";
    sf::Text title(font, titleText, 58);
    title.setFillColor(sf::Color::White);
    title.setOutlineColor(sf::Color::Black);
    title.setOutlineThickness(5.f);
    title.setPosition({960.f - title.getLocalBounds().size.x * 0.5f, 160.f});
    target.draw(title);

    if (_pauseOverlay == PauseOverlay::ReturnConfirmation) {
        _returnConfirmationMenu.render(target);
    } else {
        _pauseMenu.render(target);
    }
}

void InGameScene::updateSimulation(const float &fixedDt) {
    if (_pauseOverlay != PauseOverlay::None
        || _winReactionActive || _gameOverActive || _winActive) {
        return;
    }

    if (_scoreManager.isTimeUp()) {
        for (const auto& player : _gameWorld.getLivingPlayers()) {
            if (player) {
                player->destroy();
            }
        }
    }

    _gameWorld.updateSimulation(fixedDt);

    if (auto warpReq = _gameWorld.takeLevelWarpRequest()) {
        executeSubRoomWarp(warpReq->targetLevel, warpReq->targetWarpID);
    }

    if (GameSettings::getInstance().gameMode == GameMode::Minigame) {
        _checkMinigameResult();
    } else {
        _checkWin();
        _checkGameOver();
    }
}

void InGameScene::updateVisuals(float deltaTime) {
    if (_pauseOverlay != PauseOverlay::None) {
        if (_pauseOverlay == PauseOverlay::Settings) {
            _settingsPanel.updateVisuals(deltaTime);
        } else if (_pauseOverlay == PauseOverlay::ReturnConfirmation) {
            _returnConfirmationMenu.updateVisuals(deltaTime);
        } else {
            _pauseMenu.updateVisuals(deltaTime);
        }
        return;
    }

    _gameWorld.updateVisuals(deltaTime);

    const auto player = std::dynamic_pointer_cast<Player>(_gameWorld.getPrimaryPlayer());
    const bool starmanMusicShouldPlay =
        player && (player->isMegaState() || player->isStarManState());
    if (starmanMusicShouldPlay != _starmanMusicActive) {
        Audio::MusicManager::getInstance().play(
            starmanMusicShouldPlay
                ? "starman_theme"
                : levelThemeFor(_currentLoadedLevel, _gameWorld.getLevelMusic()),
            true
        );
        _starmanMusicActive = starmanMusicShouldPlay;
    }

    if (_winReactionActive) {
        const std::shared_ptr<Player> reactionPlayer = _minigameWinner
            ? _minigameWinner
            : player;
        auto* animatable = reactionPlayer
            ? reactionPlayer->getBehaviour<Animatable>()
            : nullptr;
        if (!animatable || animatable->isAnimationDone()) {
            _winReactionActive = false;
            _winActive = true;
        }
    }

    if (!_winReactionActive && !_gameOverActive && !_winActive) {
        _camera.update(deltaTime);
        const bool needsRebind = [&]() {
            const auto ps = _gameWorld.getPlayers();
            const bool isMulti = ps.size() >= 2 && GameSettings::getInstance().gameMode != GameMode::Solo;
            return !isMulti && !_camera.getTarget() && _gameWorld.getPrimaryPlayer();
        }();
        if (needsRebind) {
            _camera.setTarget(_gameWorld.getPrimaryPlayer());
        }
    }

    _scoreManager.update(deltaTime);
}

void InGameScene::render(sf::RenderTarget& target) {
    sf::View defaultView = target.getDefaultView();
    target.setView(_camera.getView());

    _gameWorld.render(target);

    // Render floating score popups in world coordinates
    const sf::Font& font = ResourceManager::getInstance().getFont("SuperMario");
    _scoreManager.renderFloatingTexts(target, font);

    // Render camera debug overlays (deadzone, lookahead line, level bounds) when debug grid is enabled
    if (GameSettings::getInstance().debugDrawGrid) {
        _camera.renderDebug(target);
    }

    target.setView(defaultView);

    // Render screen HUD overlay
    _scoreManager.renderHUD(target, font, &_gameWorld, sf::Vector2f(40.f, 24.f));
    drawPauseButton(target);

    if (_gameOverActive) {
        _drawGameOverOverlay(target);
    } else if (_winActive) {
        _drawWinOverlay(target);
    }

    if (_pauseOverlay != PauseOverlay::None) {
        drawPauseOverlay(target);
    }
}

nlohmann::json InGameScene::captureSaveState() const {
    nlohmann::json state;
    state["saveVersion"] = 1;
    state["levelPath"] = _name;
    state["theme"] = _gameWorld.getLevelTheme();
    state["music"] = _gameWorld.getLevelMusic();
    state["gameMode"] = gameModeToString(GameSettings::getInstance().gameMode);
    state["player1Character"] = GameSettings::getInstance().player1Character;
    state["levelCleared"] = _gameWorld.hasWon();

    state["score"] = {
        {"points", _scoreManager.getScore()},
        {"coins", _scoreManager.getCoins()},
        {"lives", _scoreManager.getLives()},
        {"luigiCoins", _scoreManager.getLuigiCoins()},
        {"luigiLives", _scoreManager.getLuigiLives()},
        {"marioScore", _scoreManager.getMarioScore()},
        {"luigiScore", _scoreManager.getLuigiScore()},
        {"highScore", _scoreManager.getHighScore()},
        {"time", _scoreManager.getTimeRemaining()}
    };

    if (const std::optional<sf::Vector2f> checkpoint =
            _gameWorld.getCheckpointPosition()) {
        state["checkpoint"] = vectorToJson(*checkpoint);
    } else {
        state["checkpoint"] = nullptr;
    }

    state["destroyedTiles"] = nlohmann::json::array();
    for (const sf::Vector2i& cell : _gameWorld.getDestroyedTileCells()) {
        state["destroyedTiles"].push_back({cell.x, cell.y});
    }

    state["objects"] = nlohmann::json::array();
    for (const std::shared_ptr<GameObject>& object : _gameWorld.objects()) {
        if (!object || object->isPendingDestroy()) {
            continue;
        }

        const std::string type = saveObjectType(*object);
        if (!isPersistedObjectType(type)) {
            continue;
        }

        nlohmann::json snapshot;
        snapshot["id"] = object->getSaveId();
        snapshot["type"] = type;
        snapshot["position"] = vectorToJson(object->getPosition());
        snapshot["velocity"] = vectorToJson(object->getVelocity());
        snapshot["size"] = vectorToJson(object->getHitboxPixels());

        if (const auto player = std::dynamic_pointer_cast<Player>(object)) {
            snapshot["character"] = player->getCharacter();
            snapshot["facingLeft"] = player->isFacingLeft();
            snapshot["flyMode"] = player->isFlyMode();
            snapshot["state"] = player->getBaseStateNameForSave();
            snapshot["megaTimeRemaining"] =
                player->getMegaStateTimeRemaining();
            snapshot["starManTimeRemaining"] =
                player->getStarManStateTimeRemaining();
            if (const auto* damageable =
                    player->getBehaviour<Damageable>()) {
                snapshot["health"] = damageable->getCurrentHealth();
            }
        } else if (const auto enemy = std::dynamic_pointer_cast<Enemy>(object)) {
            snapshot["facingRight"] = enemy->getMoveDirection() > 0;
            if (const auto* damageable =
                    enemy->getBehaviour<Damageable>()) {
                snapshot["health"] = damageable->getCurrentHealth();
            }
        }

        if (const auto coinBlock =
                std::dynamic_pointer_cast<CoinBlock>(object)) {
            snapshot["capacity"] = coinBlock->getCapacity();
        }
        if (const auto luckyBlock =
                std::dynamic_pointer_cast<LuckyBlock>(object)) {
            snapshot["capacity"] = luckyBlock->getCapacity();
            snapshot["visualVisible"] = luckyBlock->isVisualVisible();
        }
        if (const auto checkpoint =
                std::dynamic_pointer_cast<CheckpointFlag>(object)) {
            snapshot["triggered"] = checkpoint->isTriggered();
        }
        if (const auto flagpole =
                std::dynamic_pointer_cast<Flagpole>(object)) {
            snapshot["triggered"] = flagpole->isTriggered();
        }
        if (const auto item = std::dynamic_pointer_cast<Item>(object)) {
            snapshot["emerging"] = item->isEmerging();
        }
        if (const auto pipe = std::dynamic_pointer_cast<Pipe>(object)) {
            snapshot["warpID"] = pipe->getWarpID();
            snapshot["warpTarget"] = pipe->getWarpTarget();
            snapshot["brokenSegments"] = pipe->getBrokenSegmentIndices();
        }

        state["objects"].push_back(std::move(snapshot));
    }

    return state;
}

void InGameScene::restoreSaveState(const nlohmann::json& state) {
    const nlohmann::json score = state.value(
        "score",
        nlohmann::json::object()
    );
    _scoreManager.restoreState(
        score.value("points", 0),
        score.value("coins", 0),
        score.value("lives", 3),
        score.value("highScore", 0),
        score.value("time", 400.0f),
        score.value("luigiCoins", 0),
        score.value("luigiLives", 3),
        score.value("marioScore", score.value("points", 0)),
        score.value("luigiScore", 0)
    );

    if (state.contains("checkpoint") && !state["checkpoint"].is_null()) {
        _gameWorld.restoreCheckpoint(vectorFromJson(state["checkpoint"]));
    } else {
        _gameWorld.restoreCheckpoint(std::nullopt);
    }

    std::vector<sf::Vector2i> destroyedTiles;
    const nlohmann::json savedTiles = state.value(
        "destroyedTiles",
        nlohmann::json::array()
    );
    if (savedTiles.is_array()) {
        for (const nlohmann::json& cell : savedTiles) {
            if (cell.is_array() && cell.size() >= 2) {
                destroyedTiles.emplace_back(
                    cell[0].get<int>(),
                    cell[1].get<int>()
                );
            }
        }
    }
    _gameWorld.restoreDestroyedTileCells(destroyedTiles);

    std::unordered_map<std::string, nlohmann::json> savedObjects;
    const nlohmann::json objectArray = state.value(
        "objects",
        nlohmann::json::array()
    );
    if (objectArray.is_array()) {
        for (const nlohmann::json& snapshot : objectArray) {
            const std::string id = snapshot.value("id", "");
            if (!id.empty() && snapshot.is_object()) {
                savedObjects.insert_or_assign(id, snapshot);
            }
        }
    }

    std::unordered_set<std::string> restoredIds;
    std::vector<std::shared_ptr<GameObject>> objectsToRemove;
    for (const std::shared_ptr<GameObject>& object : _gameWorld.objects()) {
        if (!object || object->getSaveId().empty()) {
            continue;
        }

        const std::string id = object->getSaveId();
        const std::string currentType = saveObjectType(*object);
        const auto savedIt = savedObjects.find(id);
        if (savedIt == savedObjects.end()) {
            if (currentType != "player" && currentType != "pipe"
                && isPersistedObjectType(currentType)) {
                objectsToRemove.push_back(object);
            }
            continue;
        }

        const nlohmann::json& snapshot = savedIt->second;
        if (snapshot.value("type", "") != currentType) {
            continue;
        }
        restoredIds.insert(id);

        object->setPosition(
            vectorFromJson(snapshot.value("position", nlohmann::json{}))
        );
        object->setVelocity(
            vectorFromJson(snapshot.value("velocity", nlohmann::json{}))
        );

        if (const auto player = std::dynamic_pointer_cast<Player>(object)) {
            player->restoreSavedState(
                snapshot.value("state", "Normal"),
                snapshot.value("megaTimeRemaining", 0.0f),
                snapshot.value("starManTimeRemaining", 0.0f)
            );
            player->setFacingLeft(snapshot.value("facingLeft", false));
            player->setFlyMode(snapshot.value("flyMode", false));
            if (auto* damageable = player->getBehaviour<Damageable>()) {
                damageable->setCurrentHealth(
                    snapshot.value("health", damageable->getMaxHealth())
                );
            }
        }

        if (const auto enemy = std::dynamic_pointer_cast<Enemy>(object)) {
            enemy->setFacingRight(snapshot.value("facingRight", true));
            if (auto* damageable = enemy->getBehaviour<Damageable>()) {
                damageable->setCurrentHealth(
                    snapshot.value("health", damageable->getMaxHealth())
                );
            }
        }

        if (const auto coinBlock =
                std::dynamic_pointer_cast<CoinBlock>(object)) {
            coinBlock->restoreCapacity(snapshot.value("capacity", 1));
        }
        if (const auto luckyBlock =
                std::dynamic_pointer_cast<LuckyBlock>(object)) {
            luckyBlock->restoreCapacity(snapshot.value("capacity", 1));
            if (luckyBlock->getCapacity() > 0) {
                luckyBlock->setVisualVisible(
                    snapshot.value("visualVisible", true)
                );
            }
        }
        if (const auto pipe = std::dynamic_pointer_cast<Pipe>(object)) {
            const nlohmann::json brokenSegments = snapshot.value(
                "brokenSegments",
                nlohmann::json::array()
            );
            if (brokenSegments.is_array()) {
                std::vector<int> segmentIndices;
                for (const nlohmann::json& segment : brokenSegments) {
                    if (segment.is_number_integer()) {
                        segmentIndices.push_back(segment.get<int>());
                    }
                }
                pipe->restoreBrokenSegments(segmentIndices);
            }
        }
        if (const auto checkpoint =
                std::dynamic_pointer_cast<CheckpointFlag>(object)) {
            const bool triggered = snapshot.value("triggered", false);
            checkpoint->restoreTriggered(triggered);
            if (triggered) {
                if (auto* animatable = checkpoint->getBehaviour<Animatable>()) {
                    animatable->playAnimation("captured");
                }
            }
        }
        if (const auto flagpole =
                std::dynamic_pointer_cast<Flagpole>(object)) {
            flagpole->restoreTriggered(snapshot.value("triggered", false));
        }
    }

    for (const std::shared_ptr<GameObject>& object : objectsToRemove) {
        _gameWorld.removeObject(object);
    }

    // Items created by a used lucky block are not part of the default map's
    // initial object list. Recreate those runtime items from their snapshot.
    for (const auto& [id, snapshot] : savedObjects) {
        const std::string type = snapshot.value("type", "");
        if (restoredIds.contains(id)
            || id.rfind("runtime:", 0) != 0
            || !isItemType(type)) {
            continue;
        }

        const sf::Vector2f position = vectorFromJson(
            snapshot.value("position", nlohmann::json{})
        );
        const sf::Vector2f size = vectorFromJson(
            snapshot.value("size", nlohmann::json{}),
            {0.0f, 0.0f}
        );
        const std::shared_ptr<GameObject> item =
            _gameWorld.spawnItem(type, position, size);
        if (item) {
            item->setSaveId(id);
            item->setVelocity(
                vectorFromJson(snapshot.value("velocity", nlohmann::json{}))
            );
        }
    }

    _gameWorld.restoreLevelCleared(state.value("levelCleared", false));
}

void InGameScene::_checkWin() {
    if (_winReactionActive || _winActive || _gameOverActive) {
        return;
    }

    if (!_gameWorld.hasWon()) {
        if (_name.find("custom-map") != std::string::npos) {
            if (auto player = _gameWorld.getPrimaryPlayer()) {
                const float levelEndThreshold = static_cast<float>(_gameWorld.getGridWidth() - 2) * _gameWorld.getCellSize();
                if (player->getPosition().x >= levelEndThreshold) {
                    _gameWorld.reachFlagpole(player->getPosition());
                    return;
                }
            }
        }
        return;
    }

    _winReactionActive = true;
    _scoreManager.setTimePaused(true);

    auto player = std::dynamic_pointer_cast<Player>(_gameWorld.getPrimaryPlayer());
    if (player) {
        if (auto* animatable = player->getBehaviour<Animatable>()) {
            animatable->playAnimation("victory");
        }
    }
}

void InGameScene::_checkMinigameResult() {
    if (_winReactionActive || _winActive || _gameOverActive
        || _minigameParticipantCount < 2) {
        return;
    }

    const std::vector<std::shared_ptr<Player>> survivors =
        _gameWorld.getLivingPlayers();
    if (survivors.size() > 1) {
        return;
    }

    if (survivors.empty()) {
        _minigameWinner.reset();
        if (_winTitle) {
            _winTitle->setString("DRAW!");
        }
        _winActive = true;
        return;
    }

    _minigameWinner = survivors.front();
    if (_winTitle) {
        _winTitle->setString(
            _minigameWinner->getCharacter() == "luigi"
                ? "LUIGI WINS!"
                : "MARIO WINS!"
        );
    }
    _camera.setTarget(_minigameWinner);

    auto* animatable = _minigameWinner->getBehaviour<Animatable>();
    if (!animatable) {
        _winActive = true;
        return;
    }

    animatable->playAnimation("victory", true);
    if (animatable->getActiveAnimationName() == "victory") {
        _winReactionActive = true;
    } else {
        _winActive = true;
    }
}

void InGameScene::_checkGameOver() {
    if (_winReactionActive || _gameOverActive || _winActive) {
        return;
    }

    const bool isCoop = GameSettings::getInstance().gameMode == GameMode::Coop;
    if (isCoop) {
        std::shared_ptr<Player> mario;
        std::shared_ptr<Player> luigi;
        for (const auto& p : _gameWorld.getPlayers()) {
            if (p->getCharacter() == "mario") mario = p;
            else if (p->getCharacter() == "luigi") luigi = p;
        }

        // Mario was eliminated and removed from active objects
        if (!mario) {
            int lives = _scoreManager.getLives() - 1;
            if (_returnToMapEditor && lives <= 0) {
                lives = 3;
            }
            _scoreManager.setLives(lives);
            if (lives > 0) {
                std::optional<sf::Vector2f> spawnPos;
                if (luigi && !luigi->isEliminated()) {
                    spawnPos = luigi->getPosition();
                    spawnPos->y -= 20.0f;
                }
                _gameWorld.respawnPlayer("mario", spawnPos);
                _gameWorld.setScoreManager(&_scoreManager);
                _rebindCamera();
            }
        }

        // Luigi was eliminated and removed from active objects
        if (!luigi) {
            int lives = _scoreManager.getLuigiLives() - 1;
            if (_returnToMapEditor && lives <= 0) {
                lives = 3;
            }
            _scoreManager.setLuigiLives(lives);
            if (lives > 0) {
                std::optional<sf::Vector2f> spawnPos;
                if (mario && !mario->isEliminated()) {
                    spawnPos = mario->getPosition();
                    spawnPos->y -= 20.0f;
                }
                _gameWorld.respawnPlayer("luigi", spawnPos);
                _gameWorld.setScoreManager(&_scoreManager);
                _rebindCamera();
            }
        }

        if (_scoreManager.getLives() <= 0 && _scoreManager.getLuigiLives() <= 0) {
            _gameOverActive = true;
            Audio::MusicManager::getInstance().play("game_over_music", false);
            if (_gameOverOverlay.has_value() && _gameOverTexture) {
                _gameOverOverlay->setTexture(*_gameOverTexture);
            }
        }
    } else {
        const bool isLuigi = (GameSettings::getInstance().player1Character == "luigi");
        const auto players = _gameWorld.getPlayers();
        if (players.empty()) {
            int remainingLives = isLuigi ? (_scoreManager.getLuigiLives() - 1) : (_scoreManager.getLives() - 1);
            if (_returnToMapEditor && remainingLives <= 0) {
                remainingLives = 3;
            }
            if (isLuigi) {
                _scoreManager.setLuigiLives(remainingLives);
            } else {
                _scoreManager.setLives(remainingLives);
            }

            if (remainingLives > 0) {
                _respawnPlayer();
            } else {
                _gameOverActive = true;
                Audio::MusicManager::getInstance().play("game_over_music", false);
                if (_gameOverOverlay.has_value() && _gameOverTexture) {
                    _gameOverOverlay->setTexture(*_gameOverTexture);
                }
            }
        }
    }
}

void InGameScene::_rebindCamera() {
    const auto players = _gameWorld.getPlayers();
    if (players.size() >= 2 && GameSettings::getInstance().gameMode != GameMode::Solo) {
        std::vector<std::shared_ptr<GameObject>> targets;
        targets.reserve(players.size());
        for (const auto& p : players) targets.push_back(p);
        _camera.setTargets(targets);
    } else if (auto player = _gameWorld.getPrimaryPlayer()) {
        _camera.setTarget(player);
    }
}

void InGameScene::_respawnPlayer() {
    _gameWorld.respawnPlayer();
    _gameWorld.setScoreManager(&_scoreManager);
    _rebindCamera();
}

void InGameScene::_drawWinOverlay(sf::RenderTarget& target) {
    const sf::View view = target.getDefaultView();
    target.setView(view);

    sf::RectangleShape backdrop(view.getSize());
    backdrop.setFillColor(sf::Color(0, 0, 0, 190));
    backdrop.setPosition({0.0f, 0.0f});
    target.draw(backdrop);

    if (_winTitle.has_value()) {
        _winTitle->setOrigin(_winTitle->getLocalBounds().position + (_winTitle->getLocalBounds().size * 0.5f));
        _winTitle->setPosition({view.getSize().x * 0.5f, view.getSize().y * 0.42f});
        target.draw(*_winTitle);
    }

    if (_winPrompt.has_value()) {
        _winPrompt->setOrigin(_winPrompt->getLocalBounds().position + (_winPrompt->getLocalBounds().size * 0.5f));
        _winPrompt->setPosition({view.getSize().x * 0.5f, view.getSize().y * 0.58f});
        _winPrompt->setFillColor(sf::Color(255, 255, 255, 235));
        target.draw(*_winPrompt);
    }
}

void InGameScene::_drawGameOverOverlay(sf::RenderTarget& target) {
    if (!_gameOverOverlay.has_value() || !_gameOverTexture || !_gameOverPrompt.has_value()) {
        return;
    }

    const sf::View view = target.getDefaultView();
    target.setView(view);

    const sf::Vector2u textureSize = _gameOverTexture->getSize();
    const sf::Vector2f viewSize = view.getSize();
    if (textureSize.x == 0 || textureSize.y == 0) {
        return;
    }

    _gameOverOverlay->setOrigin({
        static_cast<float>(textureSize.x) * 0.5f,
        static_cast<float>(textureSize.y) * 0.5f
    });
    _gameOverOverlay->setPosition({viewSize.x * 0.5f, viewSize.y * 0.5f});
    _gameOverOverlay->setScale({
        viewSize.x / static_cast<float>(textureSize.x),
        viewSize.y / static_cast<float>(textureSize.y)
    });
    _gameOverOverlay->setColor(sf::Color(255, 255, 255, 210));

    target.draw(*_gameOverOverlay);

    _gameOverPrompt->setOrigin(_gameOverPrompt->getLocalBounds().position + (_gameOverPrompt->getLocalBounds().size * 0.5f));
    _gameOverPrompt->setPosition({viewSize.x * 0.5f, viewSize.y * 0.93f});
    _gameOverPrompt->setFillColor(sf::Color(255, 255, 255, 235));
    target.draw(*_gameOverPrompt);
}

void InGameScene::executeSubRoomWarp(const std::string& targetLevel, int targetWarpID) {
    std::string baseState = "normal";
    float megaTime = 0.0f;
    float starTime = 0.0f;
    if (auto player = std::dynamic_pointer_cast<Player>(_gameWorld.getPrimaryPlayer())) {
        baseState = player->getBaseStateNameForSave();
        megaTime = player->getMegaStateTimeRemaining();
        starTime = player->getStarManStateTimeRemaining();
    }

    _gameWorld.loadLevel(targetLevel);
    _currentLoadedLevel = targetLevel;

    if (auto player = std::dynamic_pointer_cast<Player>(_gameWorld.getPrimaryPlayer())) {
        player->restoreSavedState(baseState, megaTime, starTime);
    }

    _gameWorld.emergePlayerFromPipe(targetWarpID);

    CameraConfig config = _camera.getConfig();
    config.levelBounds = _gameWorld.getBounds();
    _camera.setConfig(config);
    _camera.setTarget(_gameWorld.getPrimaryPlayer());

    Audio::MusicManager::getInstance().play(
        levelThemeFor(targetLevel, _gameWorld.getLevelMusic()),
        true
    );
}

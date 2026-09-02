#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "Scene/Scene.h"
#include "Button/ButtonMenu.h"
#include "Button/SettingsPanel.h"
#include "Game/World/GameWorld.h"
#include "Game/Camera.h"
#include "Game/ScoreManager.h"



class InGameScene : public Scene {
public:
    explicit InGameScene(
        const std::string& name,
        std::optional<nlohmann::json> initialSaveState = std::nullopt,
        bool returnToMapEditor = false
    );
    ~InGameScene() override = default;

    void init() override;
    void onEnter() override;
    void onExit() override;
    void handleInput(const sf::Event& event) override;
    void updateSimulation(const float &fixedDt) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

    nlohmann::json captureSaveState() const;

private:
    enum class PauseOverlay {
        None,
        PauseMenu,
        Settings,
        ReturnConfirmation
    };

    void restoreSaveState(const nlohmann::json& state);
    void openPauseMenu();
    void openSettings();
    void resumeGame();
    void restartLevel();
    void saveGame();
    void requestReturn();
    void returnWithoutSaving();
    void buildPauseMenu();
    void buildReturnConfirmation();
    void drawPauseOverlay(sf::RenderTarget& target);
    bool handlePauseButtonInput(const sf::Event& event);
    bool isPauseButtonVisible() const noexcept;
    void setPauseButtonHovered(bool hovered);
    void drawPauseButton(sf::RenderTarget& target);
    void _checkGameOver();
    void _checkWin();
    void _checkMinigameResult();
    void _respawnPlayer();
    void _rebindCamera();
    void _drawGameOverOverlay(sf::RenderTarget& target);
    void _drawWinOverlay(sf::RenderTarget& target);
    void executeSubRoomWarp(const std::string& targetLevel, int targetWarpID);

    bool _winReactionActive = false;
    bool _gameOverActive = false;
    bool _winActive = false;
    bool _starmanMusicActive = false;
    std::string _currentLoadedLevel;
    std::size_t _minigameParticipantCount = 0;
    std::shared_ptr<Player> _minigameWinner;
    std::optional<sf::Sprite> _gameOverOverlay;
    std::optional<sf::Text> _gameOverPrompt;
    const sf::Texture* _gameOverTexture = nullptr;
    std::optional<sf::Text> _winTitle;
    std::optional<sf::Text> _winPrompt;
    std::optional<sf::Sprite> _pauseButton;
    bool _pauseButtonHovered = false;

    std::optional<nlohmann::json> _initialSaveState;
    bool _saveStateInitialized = false;
    bool _returnToMapEditor = false;
    bool _suppressExitSnapshot = false;

    PauseOverlay _pauseOverlay = PauseOverlay::None;
    UI::ButtonMenu _pauseMenu;
    UI::ButtonMenu _returnConfirmationMenu;
    UI::SettingsPanel _settingsPanel;

    GameWorld _gameWorld;
    Camera _camera;
    ScoreManager _scoreManager;
};

#include "Scene/SceneFactory.h"
#include "Scene/ConcreteScene/CharacterSelectScene.h"
#include "Scene/ConcreteScene/DefaultGameMenuScene.h"
#include "Scene/ConcreteScene/ExitConfirmScene.h"
#include "Scene/ConcreteScene/InGameScene.h"
#include "Scene/ConcreteScene/LevelSelectionScene.h"
#include "Scene/ConcreteScene/LoadGameScene.h"
#include "Scene/ConcreteScene/MainMenuScene.h"
#include "Scene/ConcreteScene/MapEditorScene.h"
#include "Scene/ConcreteScene/ModeSelectScene.h"
#include "Scene/ConcreteScene/PlayerModeSelectScene.h"
#include "Scene/ConcreteScene/SaveGameScene.h"
#include "Scene/ConcreteScene/SettingsScene.h"
#include "Scene/ConcreteScene/ScoreComputationScene.h"
#include "Scene/ConcreteScene/LeaderboardScene.h"
#include <iostream>

SceneFactory::SceneFactory() {
    registerScene("MAIN_MENU", []() { return std::make_unique<MainMenuScene>(); });
    //registerScene("GAME_DATA", []() { return std::make_unique<MenuScene>("Load Game"); });
    registerScene("SETTINGS", []() { return std::make_unique<SettingsScene>(); });
    registerScene("LEADERBOARD", []() { return std::make_unique<LeaderboardScene>(); });
    registerScene("MODE_SELECT", []() { return std::make_unique<ModeSelectScene>(); });
    registerScene("DEFAULT_GAME_MENU", []() { return std::make_unique<DefaultGameMenuScene>(); });
    registerScene("PLAYER_MODE_SELECT", []() { return std::make_unique<PlayerModeSelectScene>(); });
    registerScene("CHARACTER_SELECT", []() { return std::make_unique<CharacterSelectScene>(); });
    registerScene("LEVEL_SELECT", []() { return std::make_unique<LevelSelectionScene>(); });
    registerScene("SAVE_GAME", []() { return std::make_unique<SaveGameScene>(); });
    registerScene("LOAD_GAME", []() { return std::make_unique<LoadGameScene>(); });
    registerScene("EXIT_CONFIRM", []() { return std::make_unique<ExitConfirmScene>(); });
    registerScene("MAP_EDITOR", []() { return std::make_unique<MapEditorScene>(); });
    registerScene("IN_GAME", []() { return std::make_unique<InGameScene>("assets/datas/levels/map-1.json"); });
    registerScene("SCORE_COMPUTATION", []() { return std::make_unique<ScoreComputationScene>(); });
}

void SceneFactory::registerScene(const std::string& stateName, std::function<std::unique_ptr<Scene>()> factory) {
    _factories[stateName] = std::move(factory);
}

std::unique_ptr<Scene> SceneFactory::createScene(const std::string& stateName) {
    auto it = _factories.find(stateName);
    if (it != _factories.end()) {
        return it->second();
    }
    std::cerr << "Error: Unknown scene for state: " << stateName << std::endl;
    return nullptr;
}

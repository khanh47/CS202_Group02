#include "Scene/ConcreteScene/LevelSelectionScene.h"
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "Commands/FunctionalCommand.h"
#include "Game/GameSettings.h"
#include "Game/World/LevelDataLoader.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/InGameScene.h"
#include "Scene/SceneManager.h"

namespace {
struct LevelPlayers {
    std::size_t count = 0;
    bool hasMario = false;
    bool hasLuigi = false;
};

LevelPlayers playersInLevel(const LevelData& levelData) {
    LevelPlayers players;
    if (levelData.layer.empty() || levelData.layer.front().empty()) {
        return players;
    }

    const std::size_t levelWidth = levelData.layer.front().size();
    std::unordered_map<
        std::size_t,
        const LevelData::Placement*
    > placementsByCell;
    placementsByCell.reserve(levelData.placements.size());
    for (const LevelData::Placement& placement : levelData.placements) {
        placementsByCell.insert_or_assign(
            static_cast<std::size_t>(placement.row) * levelWidth
                + static_cast<std::size_t>(placement.column),
            &placement
        );
    }

    for (std::size_t row = 0; row < levelData.layer.size(); ++row) {
        for (
            std::size_t column = 0;
            column < levelData.layer[row].size();
            ++column
        ) {
            const char symbol = levelData.layer[row][column];
            const auto mappingIt = levelData.tileMapping.find(symbol);
            if (mappingIt == levelData.tileMapping.end()) {
                continue;
            }

            SpawnSpec spec = levelData.prefabs.resolve(mappingIt->second);
            const auto placementIt = placementsByCell.find(
                row * levelWidth + column
            );
            if (placementIt != placementsByCell.end()) {
                spec = placementIt->second->spec;
            }

            if (!spec.objectKind || *spec.objectKind != ObjectKind::Player) {
                continue;
            }

            ++players.count;
            if (spec.animationId.find("luigi") != std::string::npos) {
                players.hasLuigi = true;
            } else {
                players.hasMario = true;
            }
        }
    }

    return players;
}

bool customMapCanBePlayed(const std::filesystem::path& path) {
    try {
        const LevelData levelData = LevelDataLoader::load(path);
        const LevelPlayers players = playersInLevel(levelData);
        if (players.count == 0) {
            return false;
        }

        const GameSettings& settings = GameSettings::getInstance();
        if (settings.gameMode != GameMode::Solo) {
            return true;
        }

        return settings.player1Character == "luigi"
            ? players.hasLuigi
            : players.hasMario;
    } catch (...) {
        return false;
    }
}
} // namespace

LevelSelectionScene::LevelSelectionScene()
    : Scene("LevelSelectionScene"),
      _titleText(ResourceManager::getInstance().getFont("SuperMario"), "SELECT LEVEL", 64) {
    sf::FloatRect bounds = _titleText.getLocalBounds();
    _titleText.setOutlineThickness(5.0f);
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setFillColor(sf::Color::White);
    _titleText.setOrigin({bounds.position.x + bounds.size.x / 2.0f,
                          bounds.position.y + bounds.size.y / 2.0f});
    _titleText.setPosition({960.0f, 220.0f});
    _titleText.setFillColor(sf::Color::White);
}

void LevelSelectionScene::onEnter() {
    Scene::onEnter();
    GameSettings::getInstance().isLevelSelectActive = true;
    _setupButtons();
}

void LevelSelectionScene::onExit() {
    GameSettings::getInstance().isLevelSelectActive = false;
    Scene::onExit();
}

void LevelSelectionScene::handleInput(const sf::Event& event) {
    if (auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape) {
            if (auto mgr = getSceneManager()) {
                mgr->requestPopScene();
                return;
            }
        }
    }

    _buttonMenu.processEvent(event);
}

void LevelSelectionScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    target.draw(_titleText);
    _buttonMenu.render(target);
}

void LevelSelectionScene::_setupButtons() {
    _buttonMenu.clear();
    _buttonMenu.setLayoutProperties(
        {820.0f, 320.0f},
        {280.0f, 60.0f},
        80.0f,
        false,
        sf::Color(100, 149, 237),
        36
    );

    const std::string base = "assets/datas/levels/map-";

    _buttonMenu.addMainMenuButtonAuto("Level 1", std::make_unique<FunctionalCommand>(
        "Level 1", [this, base]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushScene(std::make_unique<InGameScene>(base + "1.json"));
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Level 2", std::make_unique<FunctionalCommand>(
        "Level 2", [this, base]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushScene(std::make_unique<InGameScene>(base + "2.json"));
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Level 3", std::make_unique<FunctionalCommand>(
        "Level 3", [this, base]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushScene(std::make_unique<InGameScene>(base + "3.json"));
            }
        }
    ));

    const std::string customMap = "assets/datas/levels/custom-map.json";
    if (std::filesystem::exists(customMap)
        && customMapCanBePlayed(customMap)) {
        _buttonMenu.addMainMenuButtonAuto("Custom Map", std::make_unique<FunctionalCommand>(
            "Custom Map", [this, customMap]() {
                if (auto mgr = getSceneManager()) {
                    mgr->pushScene(std::make_unique<InGameScene>(customMap));
                }
            }
        ));
    }
}

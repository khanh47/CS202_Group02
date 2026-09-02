#include "Scene/ConcreteScene/DefaultGameMenuScene.h"

#include "Commands/FunctionalCommand.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/InGameScene.h"
#include "Scene/SceneManager.h"

DefaultGameMenuScene::DefaultGameMenuScene()
    : Scene("DefaultGameMenuScene"),
      _titleText(
          ResourceManager::getInstance().getFont("SuperMario"),
          "DEFAULT LEVELS",
          64
      ) {
    _titleText.setOutlineThickness(5.0f);
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setFillColor(sf::Color::White);
    const sf::FloatRect bounds = _titleText.getLocalBounds();
    _titleText.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });
    _titleText.setPosition({960.0f, 220.0f});
}

void DefaultGameMenuScene::onEnter() {
    Scene::onEnter();
    setupButtons();
}

void DefaultGameMenuScene::handleInput(const sf::Event& event) {
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape) {
            if (auto* manager = getSceneManager()) {
                manager->requestPopScene();
                return;
            }
        }
    }
    _buttonMenu.processEvent(event);
}

void DefaultGameMenuScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    target.draw(_titleText);
    _buttonMenu.render(target);
}

void DefaultGameMenuScene::startNewGame() {
    SaveLoadGame::getInstance().clearCurrentSession();
    if (auto* manager = getSceneManager()) {
        manager->pushSceneByName("PLAYER_MODE_SELECT");
    }
}

void DefaultGameMenuScene::setupButtons() {
    _buttonMenu.clear();
    _buttonMenu.setLayoutProperties(
        {820.0f, 330.0f},
        {300.0f, 60.0f},
        80.0f,
        false,
        sf::Color(100, 149, 237),
        36
    );

    _buttonMenu.addMainMenuButtonAuto(
        "New Game",
        std::make_unique<FunctionalCommand>(
            "New Game",
            [this]() { startNewGame(); }
        )
    );

    _buttonMenu.addMainMenuButtonAuto(
        "Continue",
        std::make_unique<FunctionalCommand>(
            "Continue",
            [this]() {
                auto* manager = getSceneManager();
                if (!manager) {
                    return;
                }

                const SaveLoadGame& saves = SaveLoadGame::getInstance();
                const nlohmann::json* session = saves.getCurrentSession();
                if (session && session->contains("levelPath")) {
                    const std::string levelPath = session->value(
                        "levelPath",
                        ""
                    );
                    if (!levelPath.empty()) {
                        manager->pushScene(std::make_unique<InGameScene>(
                            levelPath,
                            std::optional<nlohmann::json>{*session}
                        ));
                        return;
                    }
                }

                if (saves.hasAnySave()) {
                    manager->pushSceneByName("LOAD_GAME");
                } else {
                    // A fresh installation has nothing to continue, so the
                    // Continue choice behaves as the first New Game choice.
                    startNewGame();
                }
            }
        )
    );

    _buttonMenu.addMainMenuButtonAuto(
        "Back",
        std::make_unique<FunctionalCommand>(
            "Back",
            [this]() {
                if (auto* manager = getSceneManager()) {
                    manager->requestPopScene();
                }
            }
        )
    );
}

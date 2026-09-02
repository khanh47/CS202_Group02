#include "Scene/ConcreteScene/MainMenuScene.h"
#include "Commands/FunctionalCommand.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/ExitConfirmScene.h"
#include "Scene/SceneManager.h"
#include <SFML/Graphics/Rect.hpp>

MainMenuScene::MainMenuScene()
    : Scene("MainMenuScene"),
      _promptText(ResourceManager::getInstance().getFont("SuperMario"), "Press any key to continue", 48),
      _mainText(ResourceManager::getInstance().getFont("SuperMario"), "SUPER MARIO BROS", 80) {
    sf::FloatRect bounds = _promptText.getLocalBounds();
    _promptText.setOutlineThickness(5.0f);

    _promptText.setOutlineColor(sf::Color::Black);
    _promptText.setFillColor(sf::Color::White);

    _promptText.setOrigin({bounds.position.x + bounds.size.x / 2.0f,
                           bounds.position.y + bounds.size.y / 2.0f});
    _promptText.setPosition({960.0f, 450.0f});
    _promptText.setFillColor(sf::Color::White);

    sf::FloatRect mainBounds = _mainText.getLocalBounds();
    _mainText.setOutlineThickness(5.0f);

    _mainText.setOutlineColor(sf::Color::Black);
    _mainText.setFillColor(sf::Color::White);

    _mainText.setOrigin({mainBounds.position.x + mainBounds.size.x / 2.0f,
                           mainBounds.position.y + mainBounds.size.y / 2.0f});
    _mainText.setPosition({960.0f, 200.0f});
    _mainText.setFillColor(sf::Color::White);
}

void MainMenuScene::init() {
}

void MainMenuScene::onEnter() {
    Scene::onEnter();

    if (!_showPrompt) {
        _setupButtons();
    }
}

void MainMenuScene::updateSimulation(const float& fixedDt) {
}

void MainMenuScene::updateVisuals(float deltaTime) {
}

void MainMenuScene::handleInput(const sf::Event& event) {
    if (_showPrompt) {
        if (event.is<sf::Event::KeyPressed>() ||
            event.is<sf::Event::MouseButtonPressed>() ||
            event.is<sf::Event::JoystickButtonPressed>()) {
            _showPrompt = false;
            _setupButtons();
        }
        return;
    }

    _buttonMenu.processEvent(event);
}

void MainMenuScene::render(sf::RenderTarget& target) {

    if (_showPrompt) {
        target.draw(_promptText);
        return;
    }

    target.draw(_mainText);
    _buttonMenu.render(target);
}

void MainMenuScene::_setupButtons() {
    _buttonMenu.clear();
    _buttonMenu.setLayoutProperties(
        {820.0f, 300.0f},
        {280.0f, 60.0f},
        80.0f,
        false,
        sf::Color(100, 149, 237),
       36 
    );

    _buttonMenu.addMainMenuButtonAuto("Play", std::make_unique<FunctionalCommand>(
        "Play", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("MODE_SELECT");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Save Game", std::make_unique<FunctionalCommand>(
        "Save Game", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("SAVE_GAME");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Load Game", std::make_unique<FunctionalCommand>(
        "Load Game", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("LOAD_GAME");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Leaderboard", std::make_unique<FunctionalCommand>(
        "Leaderboard", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("LEADERBOARD");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Create Map", std::make_unique<FunctionalCommand>(
        "Create Map", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("MAP_EDITOR");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Settings", std::make_unique<FunctionalCommand>(
        "Settings", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("SETTINGS");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Exit Game", std::make_unique<FunctionalCommand>(
        "Exit Game", [this]() {
            if (auto mgr = getSceneManager()) {
                if (SaveLoadGame::getInstance().hasUnsavedSession()) {
                    mgr->pushSceneByName("EXIT_CONFIRM");
                } else if (auto wnd = mgr->getRenderWindow()) {
                    wnd->close();
                }
            }
        }
    ));
}

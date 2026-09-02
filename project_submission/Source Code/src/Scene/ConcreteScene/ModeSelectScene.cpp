#include "Scene/ConcreteScene/ModeSelectScene.h"
#include "Commands/FunctionalCommand.h"
#include "Game/GameSettings.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/DefaultGameMenuScene.h"
#include "Scene/ConcreteScene/MinigameModeScene.h"
#include "Scene/SceneManager.h"

ModeSelectScene::ModeSelectScene()
    : Scene("ModeSelectScene"),
      _titleText(ResourceManager::getInstance().getFont("SuperMario"), "CHOOSE GAME MODE", 64) {
    sf::FloatRect bounds = _titleText.getLocalBounds();
    _titleText.setOutlineThickness(5.0f);
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setFillColor(sf::Color::White);
    _titleText.setOrigin({bounds.position.x + bounds.size.x / 2.0f,
                          bounds.position.y + bounds.size.y / 2.0f});
    _titleText.setPosition({960.0f, 220.0f});
    _titleText.setFillColor(sf::Color::White);
}

void ModeSelectScene::onEnter() {
    Scene::onEnter();
    _setupButtons();
}

void ModeSelectScene::handleInput(const sf::Event& event) {
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

void ModeSelectScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    target.draw(_titleText);
    _buttonMenu.render(target);
}

void ModeSelectScene::_setupButtons() {
    _buttonMenu.clear();
    _buttonMenu.setLayoutProperties(
        {820.0f, 320.0f},
        {280.0f, 60.0f},
        80.0f,
        false,
        sf::Color(100, 149, 237),
        36
    );

    _buttonMenu.addMainMenuButtonAuto("Default", std::make_unique<FunctionalCommand>(
        "Default", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("DEFAULT_GAME_MENU");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Minigames", std::make_unique<FunctionalCommand>(
        "Minigames", [this]() {
            GameSettings::getInstance().gameMode = GameMode::Minigame;
            if (auto mgr = getSceneManager()) {
                mgr->pushScene(std::make_unique<MinigameModeScene>("minigame-2"));
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Back", std::make_unique<FunctionalCommand>(
        "Back", [this]() {
            if (auto mgr = getSceneManager()) {
                mgr->requestPopScene();
            }
        }
    ));
}

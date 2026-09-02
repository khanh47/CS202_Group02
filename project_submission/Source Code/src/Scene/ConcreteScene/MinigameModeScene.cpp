#include "Scene/ConcreteScene/MinigameModeScene.h"
#include "Commands/FunctionalCommand.h"
#include "Game/GameSettings.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/InGameScene.h"
#include "Scene/SceneManager.h"

MinigameModeScene::MinigameModeScene(const std::string& mapPath)
    : Scene("MinigameModeScene"),
      _mapPath(mapPath),
      _titleText(ResourceManager::getInstance().getFont("SuperMario"), "SELECT MINIGAME MODE", 64) {

    _titleText.setOutlineThickness(5.0f);
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setFillColor(sf::Color::White);

    sf::FloatRect bounds = _titleText.getLocalBounds();
    _titleText.setOrigin({bounds.position.x + bounds.size.x / 2.0f,
                          bounds.position.y + bounds.size.y / 2.0f});
    _titleText.setPosition({960.0f, 200.0f});
    _titleText.setFillColor(sf::Color::White
    );

}

void MinigameModeScene::onEnter() {
    Scene::onEnter();
    _setupButtons();
}

void MinigameModeScene::handleInput(const sf::Event& event) {
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

void MinigameModeScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    target.draw(_titleText);
    _buttonMenu.render(target);
}

void MinigameModeScene::_setupButtons() {
    _buttonMenu.clear();
    _buttonMenu.setLayoutProperties(
        {820.0f, 300.0f},
        {280.0f, 60.0f},
        80.0f,
        false,
        sf::Color(100, 149, 237),
        36
    );

    _buttonMenu.addMainMenuButtonAuto("2 Player", std::make_unique<FunctionalCommand>(
        "2 Player", [this]() {
            GameSettings::getInstance().minigameMode = MinigameMode::TwoPlayer;
            if (auto mgr = getSceneManager()) {
                mgr->pushScene(std::make_unique<InGameScene>(
                    "assets/datas/minigames/2p/" + _mapPath + ".json"));
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("VS AI", std::make_unique<FunctionalCommand>(
        "VS AI", [this]() {
            GameSettings::getInstance().minigameMode = MinigameMode::VsAi;
            if (auto mgr = getSceneManager()) {
                mgr->pushScene(std::make_unique<InGameScene>(
                    "assets/datas/minigames/vsai/" + _mapPath + ".json"));
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

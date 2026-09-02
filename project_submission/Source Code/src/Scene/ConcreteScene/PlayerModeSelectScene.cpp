#include "Scene/ConcreteScene/PlayerModeSelectScene.h"

#include "Commands/FunctionalCommand.h"
#include "Game/GameSettings.h"
#include "ResourceManager.h"
#include "Scene/SceneManager.h"

PlayerModeSelectScene::PlayerModeSelectScene()
    : Scene("PlayerModeSelectScene"),
      _titleText(
          ResourceManager::getInstance().getFont("SuperMario"),
          "SELECT PLAY MODE",
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

void PlayerModeSelectScene::onEnter() {
    Scene::onEnter();
    setupButtons();
}

void PlayerModeSelectScene::handleInput(const sf::Event& event) {
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

void PlayerModeSelectScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    target.draw(_titleText);
    _buttonMenu.render(target);
}

void PlayerModeSelectScene::setupButtons() {
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
        "Solo",
        std::make_unique<FunctionalCommand>(
            "Solo",
            [this]() {
                GameSettings::getInstance().gameMode = GameMode::Solo;
                GameSettings::getInstance().isLevelSelectActive = false;
                if (auto* manager = getSceneManager()) {
                    manager->pushSceneByName("CHARACTER_SELECT");
                }
            }
        )
    );

    _buttonMenu.addMainMenuButtonAuto(
        "Coop",
        std::make_unique<FunctionalCommand>(
            "Coop",
            [this]() {
                GameSettings::getInstance().gameMode = GameMode::Coop;
                GameSettings::getInstance().isLevelSelectActive = true;
                if (auto* manager = getSceneManager()) {
                    manager->pushSceneByName("LEVEL_SELECT");
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

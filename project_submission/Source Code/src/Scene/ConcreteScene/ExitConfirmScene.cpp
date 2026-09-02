#include "Scene/ConcreteScene/ExitConfirmScene.h"

#include "Commands/FunctionalCommand.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/SaveGameScene.h"
#include "Scene/SceneManager.h"

ExitConfirmScene::ExitConfirmScene()
    : Scene("ExitConfirmScene"),
      _titleText(
          ResourceManager::getInstance().getFont("SuperMario"),
          "SAVE BEFORE EXIT?",
          58
      ) {
    _titleText.setOutlineThickness(5.0f);
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setFillColor(sf::Color::White);
    const sf::FloatRect bounds = _titleText.getLocalBounds();
    _titleText.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });
    _titleText.setPosition({960.0f, 250.0f});
}

void ExitConfirmScene::onEnter() {
    Scene::onEnter();
    setupButtons();
}

void ExitConfirmScene::handleInput(const sf::Event& event) {
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

void ExitConfirmScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    target.draw(_titleText);
    _buttonMenu.render(target);
}

void ExitConfirmScene::exitWithoutSaving() {
    SaveLoadGame::getInstance().clearCurrentSession();
    if (auto* manager = getSceneManager()) {
        if (auto* window = manager->getRenderWindow()) {
            window->close();
        }
    }
}

void ExitConfirmScene::setupButtons() {
    _buttonMenu.clear();
    _buttonMenu.setLayoutProperties(
        {780.0f, 380.0f},
        {360.0f, 60.0f},
        80.0f,
        false,
        sf::Color(100, 149, 237),
        36
    );

    _buttonMenu.addMainMenuButtonAuto(
        "Save and Exit",
        std::make_unique<FunctionalCommand>(
            "Save and Exit",
            [this]() {
                if (auto* manager = getSceneManager()) {
                    manager->pushScene(std::make_unique<SaveGameScene>(true));
                }
            }
        )
    );

    _buttonMenu.addMainMenuButtonAuto(
        "Exit Without Saving",
        std::make_unique<FunctionalCommand>(
            "Exit Without Saving",
            [this]() { exitWithoutSaving(); }
        )
    );

    _buttonMenu.addMainMenuButtonAuto(
        "Cancel",
        std::make_unique<FunctionalCommand>(
            "Cancel",
            [this]() {
                if (auto* manager = getSceneManager()) {
                    manager->requestPopScene();
                }
            }
        )
    );
}

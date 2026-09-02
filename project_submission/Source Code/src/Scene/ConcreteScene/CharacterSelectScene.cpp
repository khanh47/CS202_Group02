#include "Scene/ConcreteScene/CharacterSelectScene.h"
#include "Commands/FunctionalCommand.h"
#include "Game/GameSettings.h"
#include "ResourceManager.h"
#include "Scene/SceneManager.h"

CharacterSelectScene::CharacterSelectScene()
    : Scene("CharacterSelectScene"),
      _titleText(ResourceManager::getInstance().getFont("SuperMario"), "PICK A CHARACTER", 64) {
    sf::FloatRect bounds = _titleText.getLocalBounds();
    _titleText.setOutlineThickness(5.0f);
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setFillColor(sf::Color::White);
    _titleText.setOrigin({bounds.position.x + bounds.size.x / 2.0f,
                          bounds.position.y + bounds.size.y / 2.0f});
    _titleText.setPosition({960.0f, 220.0f});
    _titleText.setFillColor(sf::Color::White);
}

void CharacterSelectScene::onEnter() {
    Scene::onEnter();
    GameSettings::getInstance().isCharacterSelectActive = true;
    if (GameSettings::getInstance().characterSelectHovered != "luigi" &&
        GameSettings::getInstance().characterSelectHovered != "mario") {
        GameSettings::getInstance().characterSelectHovered = GameSettings::getInstance().player1Character;
        if (GameSettings::getInstance().characterSelectHovered != "luigi" &&
            GameSettings::getInstance().characterSelectHovered != "mario") {
            GameSettings::getInstance().characterSelectHovered = "mario";
        }
    }
    _setupButtons();
    _updatePreviewFromFocus();
}

void CharacterSelectScene::onExit() {
    GameSettings::getInstance().isCharacterSelectActive = false;
    Scene::onExit();
}

void CharacterSelectScene::handleInput(const sf::Event& event) {
    if (auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape) {
            if (auto mgr = getSceneManager()) {
                mgr->requestPopScene();
                return;
            }
        }
    }

    _buttonMenu.processEvent(event);
    _updatePreviewFromFocus();
}

void CharacterSelectScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    target.draw(_titleText);
    _buttonMenu.render(target);
}

void CharacterSelectScene::_setupButtons() {
    _buttonMenu.clear();
    _buttonMenu.setLayoutProperties(
        {820.0f, 360.0f},
        {280.0f, 60.0f},
        80.0f,
        false,
        sf::Color(100, 149, 237),
        36 
    );

    _buttonMenu.addMainMenuButtonAuto("Mario", std::make_unique<FunctionalCommand>(
        "Mario", [this]() {
            GameSettings::getInstance().player1Character = "mario";
            GameSettings::getInstance().characterSelectHovered = "mario";
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("LEVEL_SELECT");
            }
        }
    ));

    _buttonMenu.addMainMenuButtonAuto("Luigi", std::make_unique<FunctionalCommand>(
        "Luigi", [this]() {
            GameSettings::getInstance().player1Character = "luigi";
            GameSettings::getInstance().characterSelectHovered = "luigi";
            if (auto mgr = getSceneManager()) {
                mgr->pushSceneByName("LEVEL_SELECT");
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

void CharacterSelectScene::_updatePreviewFromFocus() {
    if (_buttonMenu.size() == 0) return;
    const int idx = _buttonMenu.getFocusedIndex();
    // Unified hover+keyboard: MouseMoved syncs _focusedIndex to hovered button when mouseEnabled,
    // so polling getFocusedIndex covers both mouse hover and keyboard Up/Down/W/S.
    if (idx == 0) {
        GameSettings::getInstance().characterSelectHovered = "mario";
    } else if (idx == 1) {
        GameSettings::getInstance().characterSelectHovered = "luigi";
    } // Back (2) keeps previous hovered
}

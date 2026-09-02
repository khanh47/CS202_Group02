#include "Scene/ConcreteScene/SettingsScene.h"

#include "Scene/SceneManager.h"

SettingsScene::SettingsScene()
    : Scene("SettingsScene") {}

void SettingsScene::init() {
    _settingsPanel.setOnBack([this]() {
        if (auto* manager = getSceneManager()) {
            manager->requestPopScene();
        }
    });
    _settingsPanel.refresh();
}

void SettingsScene::onEnter() {
    Scene::onEnter();
    _settingsPanel.refresh();
}

void SettingsScene::handleInput(const sf::Event& event) {
    _settingsPanel.handleInput(event);
}

void SettingsScene::updateVisuals(float deltaTime) {
    _settingsPanel.updateVisuals(deltaTime);
}

void SettingsScene::render(sf::RenderTarget& target) {
    Scene::render(target);
    _settingsPanel.render(target);
}

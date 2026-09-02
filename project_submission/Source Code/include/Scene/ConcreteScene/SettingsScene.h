#pragma once

#include "Button/SettingsPanel.h"
#include "Scene/Scene.h"

class SettingsScene : public Scene {
public:
    SettingsScene();
    ~SettingsScene() override = default;

    void init() override;
    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

private:
    UI::SettingsPanel _settingsPanel;
};

#pragma once
#include "Scene/Scene.h"
#include "Button/ButtonMenu.h"

class MainMenuScene : public Scene {
public:
    MainMenuScene();
    ~MainMenuScene() override = default;

    void init() override;
    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void updateSimulation(const float& fixedDt) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

private:
    void _setupButtons();

    UI::ButtonMenu _buttonMenu;
    sf::Text _promptText;
    sf::Text _mainText;
    bool _showPrompt = true;
};

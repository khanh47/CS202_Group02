#pragma once

#include "Button/ButtonMenu.h"
#include "Scene/Scene.h"

class DefaultGameMenuScene : public Scene {
public:
    DefaultGameMenuScene();
    ~DefaultGameMenuScene() override = default;

    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;

private:
    void setupButtons();
    void startNewGame();

    UI::ButtonMenu _buttonMenu;
    sf::Text _titleText;
};

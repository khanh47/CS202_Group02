#pragma once

#include "Button/ButtonMenu.h"
#include "Scene/Scene.h"

class PlayerModeSelectScene : public Scene {
public:
    PlayerModeSelectScene();
    ~PlayerModeSelectScene() override = default;

    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;

private:
    void setupButtons();

    UI::ButtonMenu _buttonMenu;
    sf::Text _titleText;
};

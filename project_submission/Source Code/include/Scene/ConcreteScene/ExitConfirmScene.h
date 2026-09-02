#pragma once

#include "Button/ButtonMenu.h"
#include "Scene/Scene.h"

class ExitConfirmScene : public Scene {
public:
    ExitConfirmScene();
    ~ExitConfirmScene() override = default;

    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;

private:
    void setupButtons();
    void exitWithoutSaving();

    UI::ButtonMenu _buttonMenu;
    sf::Text _titleText;
};

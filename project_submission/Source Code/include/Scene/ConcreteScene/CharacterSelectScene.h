#pragma once
#include "Scene/Scene.h"
#include "Button/ButtonMenu.h"

class CharacterSelectScene : public Scene {
public:
    CharacterSelectScene();
    ~CharacterSelectScene() override = default;

    void onEnter() override;
    void onExit() override;
    void handleInput(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;

private:
    void _setupButtons();
    void _updatePreviewFromFocus();

    UI::ButtonMenu _buttonMenu;
    sf::Text _titleText;
};

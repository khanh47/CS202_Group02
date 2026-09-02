#pragma once
#include "Scene/Scene.h"
#include "Button/ButtonMenu.h"
#include <string>

class MinigameModeScene : public Scene {
public:
    explicit MinigameModeScene(const std::string& mapPath);
    ~MinigameModeScene() override = default;

    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;

private:
    void _setupButtons();

    std::string _mapPath;
    UI::ButtonMenu _buttonMenu;
    sf::Text _titleText;
};

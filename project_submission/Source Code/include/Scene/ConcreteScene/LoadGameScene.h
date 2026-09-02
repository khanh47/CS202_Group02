#pragma once

#include <memory>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

#include "Button/ButtonMenu.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "Scene/Scene.h"

class LoadGameScene : public Scene {
public:
    LoadGameScene();
    ~LoadGameScene() override = default;

    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

private:
    void setupButtons();
    void loadFromSave(const std::string& saveId);
    void deleteSaveFile(const std::string& saveId);
    void setStatus(const std::string& status, sf::Color color = sf::Color::White);
    void changePage(int delta);

    UI::ButtonMenu _buttonMenu;
    std::vector<SaveLoadGame::SaveInfo> _allSaves;
    int _currentPage = 0;
    static constexpr int SavesPerPage = 3;

    sf::Text _titleText;
    sf::Text _statusText;
};

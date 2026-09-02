#pragma once

#include <vector>
#include <string>
#include <memory>
#include <SFML/Graphics.hpp>
#include "Scene/Scene.h"
#include "Button/ButtonMenu.h"
#include "Game/LeaderboardManager.h"

class LeaderboardScene : public Scene {
public:
    LeaderboardScene(const std::string& defaultLevelKey = "map-1");
    ~LeaderboardScene() override = default;

    void handleInput(const sf::Event& event) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

private:
    enum class FocusSection {
        Tabs,
        Back
    };

    void setupUI();
    void selectTab(const std::string& levelKey);
    void updateTabStyles();
    void renderTable(sf::RenderTarget& target, const sf::Font& font);

    std::string _selectedLevelKey = "map-1";
    FocusSection _focusSection = FocusSection::Tabs;
    UI::ButtonMenu _tabMenu;
    UI::ButtonMenu _backMenu;
    std::optional<sf::Sprite> _backgroundSprite;
};

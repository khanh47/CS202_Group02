#include "Scene/ConcreteScene/LeaderboardScene.h"
#include "ResourceManager.h"
#include "Scene/SceneManager.h"
#include "Commands/FunctionalCommand.h"
#include "Audio/SoundManager.h"
#include <iomanip>
#include <sstream>

namespace {
std::string formatScoreString(int score) {
    std::ostringstream ss;
    ss << std::setw(6) << std::setfill('0') << score;
    return ss.str();
}
}

LeaderboardScene::LeaderboardScene(const std::string& defaultLevelKey)
    : _selectedLevelKey(defaultLevelKey) {
    try {
        sf::Texture& bgTex = ResourceManager::getInstance().getTexture("far_sky");
        _backgroundSprite.emplace(bgTex);
        const sf::Vector2u texSize = bgTex.getSize();
        if (texSize.x > 0 && texSize.y > 0) {
            _backgroundSprite->setScale({1920.f / texSize.x, 1080.f / texSize.y});
        }
    } catch (...) {
        _backgroundSprite.reset();
    }

    setupUI();
}

void LeaderboardScene::setupUI() {
    // 1. Horizontal Tabs Menu
    _tabMenu.clear();
    _tabMenu.setLayoutProperties(
        {320.f, 130.f}, {240.f, 50.f}, 260.f, true,
        sf::Color(45, 75, 115), 22
    );

    _tabMenu.addButtonAuto(
        "World 1",
        std::make_unique<FunctionalCommand>("W1", [this]() { selectTab("map-1"); })
    );
    _tabMenu.addButtonAuto(
        "World 2",
        std::make_unique<FunctionalCommand>("W2", [this]() { selectTab("map-2"); })
    );
    _tabMenu.addButtonAuto(
        "World 3",
        std::make_unique<FunctionalCommand>("W3", [this]() { selectTab("map-3"); })
    );
    _tabMenu.addButtonAuto(
        "Minigames",
        std::make_unique<FunctionalCommand>("MG", [this]() { selectTab("minigame"); })
    );
    _tabMenu.addButtonAuto(
        "Custom Map",
        std::make_unique<FunctionalCommand>("CM", [this]() { selectTab("custom-map"); })
    );

    // 2. Back Button at Bottom Center
    _backMenu.clear();
    _backMenu.setKeyboardEnabled(false);
    _backMenu.setLayoutProperties(
        {810.f, 960.f}, {300.f, 60.f}, 70.f, false,
        sf::Color(100, 149, 237), 30
    );
    _backMenu.addMainMenuButtonAuto(
        "Back",
        std::make_unique<FunctionalCommand>("Back", [this]() {
            Audio::SoundManager::getInstance().playEffect("select_button");
            if (auto* mgr = getSceneManager()) {
                mgr->requestPopScene();
            }
        })
    );

    updateTabStyles();
}

void LeaderboardScene::selectTab(const std::string& levelKey) {
    Audio::SoundManager::getInstance().playEffect("select_button");
    _selectedLevelKey = levelKey;
    updateTabStyles();
}

void LeaderboardScene::updateTabStyles() {
    const std::vector<std::string> keys = {"map-1", "map-2", "map-3", "minigame", "custom-map"};
    for (size_t i = 0; i < keys.size(); ++i) {
        if (auto btn = _tabMenu.getButton(i)) {
            if (keys[i] == _selectedLevelKey) {
                btn->setColor(sf::Color(230, 160, 20)); // Golden highlight for active tab
            } else {
                btn->setColor(sf::Color(45, 75, 115)); // Inactive tab
            }
        }
    }
}

void LeaderboardScene::handleInput(const sf::Event& event) {
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape || keyEvent->code == sf::Keyboard::Key::Backspace) {
            Audio::SoundManager::getInstance().playEffect("select_button");
            if (auto* mgr = getSceneManager()) {
                mgr->requestPopScene();
            }
            return;
        }

        // Navigate between Tabs and Back button with Up/Down
        if (keyEvent->code == sf::Keyboard::Key::Down || keyEvent->code == sf::Keyboard::Key::S) {
            if (_focusSection == FocusSection::Tabs) {
                _focusSection = FocusSection::Back;
                _tabMenu.setKeyboardEnabled(false);
                _backMenu.setKeyboardEnabled(true);
                _backMenu.setFocusedIndex(0);
                Audio::SoundManager::getInstance().playEffect("select_button");
                return;
            }
        } else if (keyEvent->code == sf::Keyboard::Key::Up || keyEvent->code == sf::Keyboard::Key::W) {
            if (_focusSection == FocusSection::Back) {
                _focusSection = FocusSection::Tabs;
                _backMenu.setKeyboardEnabled(false);
                _tabMenu.setKeyboardEnabled(true);
                Audio::SoundManager::getInstance().playEffect("select_button");
                return;
            }
        }
    }

    if (event.is<sf::Event::MouseButtonPressed>() || event.is<sf::Event::MouseMoved>()) {
        _tabMenu.processEvent(event);
        _backMenu.processEvent(event);
    } else {
        if (_focusSection == FocusSection::Tabs) {
            _tabMenu.processEvent(event);
        } else {
            _backMenu.processEvent(event);
        }
    }
}

void LeaderboardScene::updateVisuals(float deltaTime) {
    _tabMenu.updateVisuals(deltaTime);
    _backMenu.updateVisuals(deltaTime);
}

void LeaderboardScene::render(sf::RenderTarget& target) {
    target.setView(target.getDefaultView());

    if (_backgroundSprite.has_value()) {
        target.draw(*_backgroundSprite);
    } else {
        sf::RectangleShape solidBg({1920.f, 1080.f});
        solidBg.setFillColor(sf::Color(20, 35, 60));
        target.draw(solidBg);
    }

    const sf::Font& font = ResourceManager::getInstance().getFont("SuperMario");

    // Title
    sf::Text title(font, "HALL OF FAME", 54);
    title.setFillColor(sf::Color(255, 220, 0));
    title.setOutlineColor(sf::Color::Black);
    title.setOutlineThickness(4.f);
    title.setPosition({960.f - title.getLocalBounds().size.x * 0.5f, 40.f});
    target.draw(title);

    // Tab buttons
    _tabMenu.render(target);

    // Table Content
    renderTable(target, font);

    // Back button
    _backMenu.render(target);
}

void LeaderboardScene::renderTable(sf::RenderTarget& target, const sf::Font& font) {
    // Backdrop Panel
    sf::RectangleShape panel({1400.f, 710.f});
    panel.setPosition({260.f, 210.f});
    panel.setFillColor(sf::Color(10, 20, 40, 225));
    panel.setOutlineColor(sf::Color(255, 215, 0, 200));
    panel.setOutlineThickness(3.f);
    target.draw(panel);

    // Column Headers
    const float headerY = 230.f;
    const std::vector<std::pair<std::string, float>> columns = {
        {"RANK", 310.f},
        {"PLAYER", 470.f},
        {"SCORE", 750.f},
        {"COINS", 1010.f},
        {"TIME", 1210.f},
        {"DATE", 1430.f}
    };

    for (const auto& [name, colX] : columns) {
        sf::Text colText(font, name, 24);
        colText.setFillColor(sf::Color(100, 200, 255));
        colText.setOutlineColor(sf::Color::Black);
        colText.setOutlineThickness(2.f);
        colText.setPosition({colX, headerY});
        target.draw(colText);
    }

    // Divider line
    sf::RectangleShape line({1340.f, 3.f});
    line.setPosition({290.f, 275.f});
    line.setFillColor(sf::Color(100, 180, 240, 150));
    target.draw(line);

    // Retrieve entries
    const auto& entries = LeaderboardManager::getInstance().getEntries(_selectedLevelKey);

    if (entries.empty()) {
        sf::Text emptyText(font, "NO RECORDS YET - PLAY TO CLAIM #1!", 28);
        emptyText.setFillColor(sf::Color(200, 220, 240));
        emptyText.setOutlineColor(sf::Color::Black);
        emptyText.setOutlineThickness(3.f);
        emptyText.setPosition({960.f - emptyText.getLocalBounds().size.x * 0.5f, 500.f});
        target.draw(emptyText);
        return;
    }

    // Render Rows
    float rowY = 300.f;
    const float rowSpacing = 50.f;

    for (size_t i = 0; i < entries.size() && i < 8; ++i) {
        const auto& entry = entries[i];
        const int rank = static_cast<int>(i + 1);

        // Rank Color
        sf::Color rankColor = sf::Color::White;
        std::string rankPrefix = std::to_string(rank);
        if (rank == 1) {
            rankColor = sf::Color(255, 215, 0); // Gold
            rankPrefix = "1ST";
        } else if (rank == 2) {
            rankColor = sf::Color(220, 220, 230); // Silver
            rankPrefix = "2ND";
        } else if (rank == 3) {
            rankColor = sf::Color(205, 127, 50); // Bronze
            rankPrefix = "3RD";
        } else {
            rankPrefix = std::to_string(rank) + "TH";
        }

        // 1. Rank
        sf::Text rankText(font, rankPrefix, 24);
        rankText.setFillColor(rankColor);
        rankText.setOutlineColor(sf::Color::Black);
        rankText.setOutlineThickness(2.f);
        rankText.setPosition({310.f, rowY});
        target.draw(rankText);

        // 2. Character & Player Name
        sf::Color nameColor = entry.character == "luigi" ? sf::Color(100, 255, 100) : sf::Color(255, 100, 100);
        sf::Text nameText(font, entry.playerName, 24);
        nameText.setFillColor(nameColor);
        nameText.setOutlineColor(sf::Color::Black);
        nameText.setOutlineThickness(2.f);
        nameText.setPosition({470.f, rowY});
        target.draw(nameText);

        // 3. Score
        sf::Text scoreText(font, formatScoreString(entry.score), 24);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setOutlineColor(sf::Color::Black);
        scoreText.setOutlineThickness(2.f);
        scoreText.setPosition({750.f, rowY});
        target.draw(scoreText);

        // 4. Coins
        sf::Text coinsText(font, "X " + std::to_string(entry.coins), 24);
        coinsText.setFillColor(sf::Color(255, 230, 80));
        coinsText.setOutlineColor(sf::Color::Black);
        coinsText.setOutlineThickness(2.f);
        coinsText.setPosition({1010.f, rowY});
        target.draw(coinsText);

        // 5. Time
        sf::Text timeText(font, std::to_string(entry.timeRemaining) + "S", 24);
        timeText.setFillColor(sf::Color(150, 230, 255));
        timeText.setOutlineColor(sf::Color::Black);
        timeText.setOutlineThickness(2.f);
        timeText.setPosition({1210.f, rowY});
        target.draw(timeText);

        // 6. Date
        sf::Text dateText(font, entry.date.empty() ? "--" : entry.date, 22);
        dateText.setFillColor(sf::Color(180, 200, 220));
        dateText.setOutlineColor(sf::Color::Black);
        dateText.setOutlineThickness(2.f);
        dateText.setPosition({1430.f, rowY});
        target.draw(dateText);

        rowY += rowSpacing;
    }
}

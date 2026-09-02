#include "Scene/ConcreteScene/LoadGameScene.h"

#include <algorithm>
#include <utility>

#include "Audio/SoundManager.h"
#include "Button/Button.h"
#include "Commands/FunctionalCommand.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "ResourceManager.h"
#include "Scene/ConcreteScene/InGameScene.h"
#include "Scene/SceneManager.h"
#include "UI/UIHelpers.h"

namespace {
constexpr float kPanelX = 390.0f;
constexpr float kPanelY = 170.0f;
constexpr float kPanelW = 1140.0f;
constexpr float kPanelH = 700.0f;
constexpr float kCardX = 430.0f;
constexpr float kCardW = 1060.0f;
constexpr float kCardH = 115.0f;
constexpr float kRowStartY = 260.0f;
constexpr float kRowSpacing = 130.0f;
}

LoadGameScene::LoadGameScene()
    : Scene("LoadGameScene"),
      _titleText(
          ResourceManager::getInstance().getFont("SuperMario"),
          "LOAD GAME",
          52
      ),
      _statusText(
          ResourceManager::getInstance().getFont("moon_get"),
          "",
          22
      ) {
    _titleText.setOutlineThickness(5.0f);
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setFillColor(sf::Color::White);
    _titleText.setStyle(sf::Text::Bold);
    const sf::FloatRect titleBounds = _titleText.getLocalBounds();
    _titleText.setOrigin({
        titleBounds.position.x + titleBounds.size.x * 0.5f,
        titleBounds.position.y + titleBounds.size.y * 0.5f
    });
    _titleText.setPosition({960.0f, 120.0f});
    _statusText.setFillColor(sf::Color::White);
}

void LoadGameScene::onEnter() {
    Scene::onEnter();
    _currentPage = 0;
    setupButtons();
    if (!SaveLoadGame::getInstance().hasAnySave()) {
        setStatus(
            "No saved default-level games found.",
            sf::Color(255, 220, 120)
        );
    }
}

void LoadGameScene::handleInput(const sf::Event& event) {
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape) {
            Audio::SoundManager::getInstance().playEffect("select_button");
            if (auto* manager = getSceneManager()) {
                manager->requestPopScene();
                return;
            }
        }
    }
    _buttonMenu.processEvent(event);
}

void LoadGameScene::updateVisuals(float deltaTime) {
    _buttonMenu.updateVisuals(deltaTime);
}

void LoadGameScene::render(sf::RenderTarget& target) {
    Scene::render(target);

    const sf::Vector2f viewSize = target.getView().getSize();
    const sf::Vector2f topLeft = target.getView().getCenter() - viewSize * 0.5f;

    // 1. Semi-transparent backdrop dimming
    sf::RectangleShape backdrop(viewSize);
    backdrop.setPosition(topLeft);
    backdrop.setFillColor(sf::Color(0, 0, 0, 160));
    target.draw(backdrop);

    // 2. Main Glassmorphism Panel
    {
        sf::ConvexShape panel = UI::Helper::makeRoundedRect({kPanelX, kPanelY}, {kPanelW, kPanelH}, 24.0f, 10);
        panel.setFillColor(sf::Color(14, 20, 32, 215));
        panel.setOutlineThickness(2.0f);
        panel.setOutlineColor(sf::Color(165, 190, 220, 100));
        target.draw(panel);
    }

    // 3. Title & Status Text
    target.draw(_titleText);
    target.draw(_statusText);

    const sf::Font& fontMario = ResourceManager::getInstance().getFont("SuperMario");
    const sf::Font& fontMoon = ResourceManager::getInstance().getFont("moon_get");

    const int totalSaves = static_cast<int>(_allSaves.size());
    const int maxPages = std::max(1, (totalSaves + SavesPerPage - 1) / SavesPerPage);
    const int startIdx = _currentPage * SavesPerPage;
    const int endIdx = std::min(startIdx + SavesPerPage, totalSaves);

    if (totalSaves == 0) {
        sf::Text emptyMsg(fontMoon, "No saved game files found. Start a new game and save from the pause menu!", 20);
        emptyMsg.setFillColor(sf::Color(150, 175, 205));
        const sf::FloatRect emb = emptyMsg.getLocalBounds();
        emptyMsg.setOrigin({emb.position.x + emb.size.x * 0.5f, emb.position.y + emb.size.y * 0.5f});
        emptyMsg.setPosition({960.0f, 440.0f});
        target.draw(emptyMsg);
    } else {
        for (int i = startIdx; i < endIdx; ++i) {
            const int slotOnPage = i - startIdx;
            const float rowY = kRowStartY + static_cast<float>(slotOnPage) * kRowSpacing;

            // Card backdrop
            sf::ConvexShape card = UI::Helper::makeRoundedRect({kCardX, rowY}, {kCardW, kCardH}, 14.0f, 8);
            card.setFillColor(sf::Color(24, 38, 58, 190));
            card.setOutlineThickness(1.5f);
            card.setOutlineColor(sf::Color(70, 105, 150, 120));
            target.draw(card);

            // Save Name
            sf::Text title(fontMario, _allSaves[static_cast<std::size_t>(i)].name, 24);
            title.setFillColor(sf::Color::White);
            title.setOutlineThickness(2.0f);
            title.setOutlineColor(sf::Color::Black);
            title.setPosition({kCardX + 30.0f, rowY + 18.0f});
            target.draw(title);

            // Metadata Subtitle
            sf::Text metaText(fontMoon, "Saved: " + _allSaves[static_cast<std::size_t>(i)].savedDate, 16);
            metaText.setFillColor(sf::Color(160, 190, 220));
            metaText.setPosition({kCardX + 30.0f, rowY + 68.0f});
            target.draw(metaText);
        }
    }

    // 4. Pagination Text
    if (maxPages > 1) {
        sf::Text pageText(fontMoon, "Page " + std::to_string(_currentPage + 1) + " / " + std::to_string(maxPages), 18);
        pageText.setFillColor(sf::Color(220, 235, 255));
        const sf::FloatRect pb = pageText.getLocalBounds();
        pageText.setOrigin({pb.position.x + pb.size.x * 0.5f, pb.position.y + pb.size.y * 0.5f});
        pageText.setPosition({960.0f, 665.0f});
        target.draw(pageText);
    }

    // 5. Interactive UI Buttons (LOAD, DELETE, PREV, NEXT, BACK)
    _buttonMenu.render(target);

    // 6. Bottom Navigation Hint
    sf::Text hint(fontMoon, "UP/DOWN NAVIGATE | ENTER LOAD | ESC BACK", 18);
    hint.setFillColor(sf::Color(170, 195, 220, 190));
    const sf::FloatRect hintBounds = hint.getLocalBounds();
    hint.setOrigin({
        hintBounds.position.x + hintBounds.size.x * 0.5f,
        hintBounds.position.y + hintBounds.size.y * 0.5f
    });
    hint.setPosition({960.0f, 1030.0f});
    target.draw(hint);
}

void LoadGameScene::setStatus(const std::string& status, sf::Color color) {
    _statusText.setString(status);
    _statusText.setFillColor(color);
    const sf::FloatRect bounds = _statusText.getLocalBounds();
    _statusText.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });
    _statusText.setPosition({960.0f, 210.0f});
}

void LoadGameScene::loadFromSave(const std::string& saveId) {
    SaveLoadGame::SaveInfo info;
    nlohmann::json state;
    if (!SaveLoadGame::getInstance().loadSave(saveId, info, state)) {
        setStatus("This save file is invalid or missing.", sf::Color(255, 120, 120));
        return;
    }

    const std::string levelPath = state.value("levelPath", "");
    const std::string gameMode = state.value("gameMode", "");
    if (levelPath.empty() || gameMode == "minigame") {
        setStatus("This save file is not a valid level game.", sf::Color(255, 120, 120));
        return;
    }

    Audio::SoundManager::getInstance().playEffect("pipe");

    if (auto* manager = getSceneManager()) {
        manager->pushScene(std::make_unique<InGameScene>(
            levelPath,
            std::optional<nlohmann::json>{std::move(state)}
        ));
    }
}

void LoadGameScene::deleteSaveFile(const std::string& saveId) {
    if (SaveLoadGame::getInstance().deleteSave(saveId)) {
        Audio::SoundManager::getInstance().playEffect("kick");
        setStatus("Deleted save file.", sf::Color(255, 180, 120));
        setupButtons();
    } else {
        setStatus("Could not delete save file.", sf::Color(255, 120, 120));
    }
}

void LoadGameScene::changePage(int delta) {
    const int totalSaves = static_cast<int>(_allSaves.size());
    const int maxPages = std::max(1, (totalSaves + SavesPerPage - 1) / SavesPerPage);
    _currentPage = std::clamp(_currentPage + delta, 0, maxPages - 1);
    Audio::SoundManager::getInstance().playEffect("select_button");
    setupButtons();
}

void LoadGameScene::setupButtons() {
    _buttonMenu.clear();

    _buttonMenu.setMouseEnabled(true);
    _buttonMenu.setKeyboardEnabled(true);
    _buttonMenu.setWasdEnabled(false);

    _allSaves = SaveLoadGame::getInstance().getAllSaves();
    const int totalSaves = static_cast<int>(_allSaves.size());
    const int maxPages = std::max(1, (totalSaves + SavesPerPage - 1) / SavesPerPage);
    _currentPage = std::clamp(_currentPage, 0, maxPages - 1);

    const int startIdx = _currentPage * SavesPerPage;
    const int endIdx = std::min(startIdx + SavesPerPage, totalSaves);

    for (int i = startIdx; i < endIdx; ++i) {
        const int slotOnPage = i - startIdx;
        const float rowY = kRowStartY + static_cast<float>(slotOnPage) * kRowSpacing;
        const std::string saveId = _allSaves[static_cast<std::size_t>(i)].id;

        // 1. Load Button
        auto loadBtn = std::make_shared<UI::Button>(
            sf::Vector2f{kCardX + 680.0f, rowY + 32.0f},
            sf::Vector2f{180.0f, 50.0f},
            sf::Color(70, 130, 180),
            "LOAD",
            22,
            10.0f
        );
        loadBtn->setCommand(std::make_unique<FunctionalCommand>(
            "Load_" + saveId,
            [this, saveId]() { loadFromSave(saveId); }
        ));
        _buttonMenu.addButton(loadBtn);

        // 2. Delete Button
        auto deleteBtn = std::make_shared<UI::Button>(
            sf::Vector2f{kCardX + 880.0f, rowY + 32.0f},
            sf::Vector2f{150.0f, 50.0f},
            sf::Color(180, 60, 60),
            "DELETE",
            20,
            10.0f
        );
        deleteBtn->setCommand(std::make_unique<FunctionalCommand>(
            "Delete_" + saveId,
            [this, saveId]() { deleteSaveFile(saveId); }
        ));
        _buttonMenu.addButton(deleteBtn);
    }

    // 3. Pagination Buttons
    if (maxPages > 1) {
        if (_currentPage > 0) {
            auto prevBtn = std::make_shared<UI::Button>(
                sf::Vector2f{730.0f, 650.0f},
                sf::Vector2f{110.0f, 40.0f},
                sf::Color(65, 100, 145),
                "< PREV",
                18,
                8.0f
            );
            prevBtn->setCommand(std::make_unique<FunctionalCommand>(
                "PrevPage",
                [this]() { changePage(-1); }
            ));
            _buttonMenu.addButton(prevBtn);
        }

        if (_currentPage < maxPages - 1) {
            auto nextBtn = std::make_shared<UI::Button>(
                sf::Vector2f{1080.0f, 650.0f},
                sf::Vector2f{110.0f, 40.0f},
                sf::Color(65, 100, 145),
                "NEXT >",
                18,
                8.0f
            );
            nextBtn->setCommand(std::make_unique<FunctionalCommand>(
                "NextPage",
                [this]() { changePage(1); }
            ));
            _buttonMenu.addButton(nextBtn);
        }
    }

    // 4. Back Button Centered at Bottom of Panel
    auto backBtn = std::make_shared<UI::Button>(
        sf::Vector2f{860.0f, 800.0f},
        sf::Vector2f{200.0f, 50.0f},
        sf::Color(180, 75, 75),
        "BACK",
        22,
        12.0f
    );
    backBtn->setCommand(std::make_unique<FunctionalCommand>(
        "Back",
        [this]() {
            Audio::SoundManager::getInstance().playEffect("select_button");
            if (auto* manager = getSceneManager()) {
                manager->requestPopScene();
            }
        }
    ));
    _buttonMenu.addButton(backBtn);
}

#include "Scene/ConcreteScene/SaveGameScene.h"

#include <algorithm>
#include <utility>

#include "Audio/SoundManager.h"
#include "Button/TextInput.h"
#include "Commands/FunctionalCommand.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "ResourceManager.h"
#include "Scene/SceneManager.h"
#include "UI/UIHelpers.h"

namespace {
constexpr float kPanelX = 390.0f;
constexpr float kPanelY = 170.0f;
constexpr float kPanelW = 1140.0f;
constexpr float kPanelH = 700.0f;
constexpr float kCardX = 430.0f;
constexpr float kCardW = 1060.0f;
}

SaveGameScene::SaveGameScene(
    bool exitAfterSave,
    std::function<void()> onSuccessfulSave
)
    : Scene("SaveGameScene"),
      _exitAfterSave(exitAfterSave),
      _onSuccessfulSave(std::move(onSuccessfulSave)),
      _titleText(
          ResourceManager::getInstance().getFont("SuperMario"),
          "SAVE GAME",
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

void SaveGameScene::onEnter() {
    Scene::onEnter();
    _currentPage = 0;
    setupControls();
    if (!SaveLoadGame::getInstance().hasCurrentSession()) {
        setStatus("No active default-level game to save.", sf::Color(255, 220, 120));
    }
}

void SaveGameScene::handleInput(const sf::Event& event) {
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

void SaveGameScene::updateVisuals(float deltaTime) {
    _buttonMenu.updateVisuals(deltaTime);
}

void SaveGameScene::render(sf::RenderTarget& target) {
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

    // 4. Section 1: "CREATE NEW SAVE" Card
    {
        sf::ConvexShape newCard = UI::Helper::makeRoundedRect({kCardX, 245.0f}, {kCardW, 85.0f}, 14.0f, 8);
        newCard.setFillColor(sf::Color(30, 48, 72, 190));
        newCard.setOutlineThickness(1.5f);
        newCard.setOutlineColor(sf::Color(100, 150, 210, 120));
        target.draw(newCard);

        sf::Text newLabel(fontMario, "NEW FILE:", 18);
        newLabel.setFillColor(sf::Color(255, 215, 0));
        newLabel.setOutlineThickness(2.0f);
        newLabel.setOutlineColor(sf::Color::Black);
        newLabel.setPosition({kCardX + 20.0f, 274.0f});
        target.draw(newLabel);
    }

    // 5. Section 2: "EXISTING SAVES" Header & Cards
    {
        sf::Text existingHeader(fontMario, "OR OVERWRITE EXISTING SAVE (" + std::to_string(_allSaves.size()) + "):", 19);
        existingHeader.setFillColor(sf::Color(220, 235, 255));
        existingHeader.setOutlineThickness(2.0f);
        existingHeader.setOutlineColor(sf::Color::Black);
        existingHeader.setPosition({kCardX + 5.0f, 350.0f});
        target.draw(existingHeader);
    }

    const int totalSaves = static_cast<int>(_allSaves.size());
    const int maxPages = std::max(1, (totalSaves + SavesPerPage - 1) / SavesPerPage);
    const int startIdx = _currentPage * SavesPerPage;
    const int endIdx = std::min(startIdx + SavesPerPage, totalSaves);

    if (totalSaves == 0) {
        sf::Text emptyMsg(fontMoon, "No previous save files found to overwrite. Save as a new file above!", 18);
        emptyMsg.setFillColor(sf::Color(150, 175, 205));
        emptyMsg.setPosition({kCardX + 160.0f, 440.0f});
        target.draw(emptyMsg);
    } else {
        for (int i = startIdx; i < endIdx; ++i) {
            const int slotOnPage = i - startIdx;
            const float rowY = 385.0f + static_cast<float>(slotOnPage) * 115.0f;

            // Card backdrop
            sf::ConvexShape card = UI::Helper::makeRoundedRect({kCardX, rowY}, {kCardW, 100.0f}, 14.0f, 8);
            card.setFillColor(sf::Color(24, 38, 58, 190));
            card.setOutlineThickness(1.5f);
            card.setOutlineColor(sf::Color(70, 105, 150, 120));
            target.draw(card);

            // Subtitle metadata
            sf::Text metaText(fontMoon, "Saved: " + _allSaves[static_cast<std::size_t>(i)].savedDate, 15);
            metaText.setFillColor(sf::Color(160, 190, 220));
            metaText.setPosition({kCardX + 25.0f, rowY + 62.0f});
            target.draw(metaText);
        }
    }

    // 6. Pagination Text (if multiple pages)
    if (maxPages > 1) {
        sf::Text pageText(fontMoon, "Page " + std::to_string(_currentPage + 1) + " / " + std::to_string(maxPages), 18);
        pageText.setFillColor(sf::Color(220, 235, 255));
        const sf::FloatRect pb = pageText.getLocalBounds();
        pageText.setOrigin({pb.position.x + pb.size.x * 0.5f, pb.position.y + pb.size.y * 0.5f});
        pageText.setPosition({960.0f, 650.0f});
        target.draw(pageText);
    }

    // 7. Interactive Buttons
    _buttonMenu.render(target);

    // 8. Bottom Navigation Hint
    sf::Text hint(fontMoon, "UP/DOWN NAVIGATE | TYPE TO RENAME | ENTER TO SAVE | ESC BACK", 18);
    hint.setFillColor(sf::Color(170, 195, 220, 190));
    const sf::FloatRect hintBounds = hint.getLocalBounds();
    hint.setOrigin({
        hintBounds.position.x + hintBounds.size.x * 0.5f,
        hintBounds.position.y + hintBounds.size.y * 0.5f
    });
    hint.setPosition({960.0f, 1030.0f});
    target.draw(hint);
}

void SaveGameScene::setStatus(const std::string& status, sf::Color color) {
    _statusText.setString(status);
    _statusText.setFillColor(color);
    const sf::FloatRect bounds = _statusText.getLocalBounds();
    _statusText.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });
    _statusText.setPosition({960.0f, 210.0f});
}

void SaveGameScene::createNewSave() {
    const nlohmann::json* currentSession =
        SaveLoadGame::getInstance().getCurrentSession();
    if (!currentSession) {
        setStatus("No active default-level game to save.", sf::Color(255, 220, 120));
        return;
    }

    std::string saveName = _newSaveInput ? _newSaveInput->getValue() : "";
    if (saveName.empty()) {
        saveName = SaveLoadGame::getInstance().getDefaultSaveName(*currentSession);
    }

    if (!SaveLoadGame::getInstance().createSave(saveName, *currentSession)) {
        setStatus("Could not create save file.", sf::Color(255, 120, 120));
        return;
    }

    Audio::SoundManager::getInstance().playEffect("coin");
    SaveLoadGame::getInstance().markSessionSaved();
    setStatus("Created new save \"" + saveName + "\"!", sf::Color(170, 255, 170));

    if (_newSaveInput) {
        _newSaveInput->setValue("");
    }

    setupControls();

    if (_exitAfterSave) {
        if (auto* manager = getSceneManager()) {
            if (auto* window = manager->getRenderWindow()) {
                window->close();
            }
        }
    } else if (_onSuccessfulSave) {
        _onSuccessfulSave();
    }
}

void SaveGameScene::overwriteExistingSave(const std::string& saveId, int inputIndex) {
    const nlohmann::json* currentSession =
        SaveLoadGame::getInstance().getCurrentSession();
    if (!currentSession) {
        setStatus("No active default-level game to save.", sf::Color(255, 220, 120));
        return;
    }

    std::string newName = "";
    if (inputIndex >= 0 && inputIndex < static_cast<int>(_existingNameInputs.size())) {
        newName = _existingNameInputs[static_cast<std::size_t>(inputIndex)]->getValue();
    }
    if (newName.empty()) {
        newName = saveId;
    }

    if (!SaveLoadGame::getInstance().overwriteSave(saveId, newName, *currentSession)) {
        setStatus("Could not overwrite save file.", sf::Color(255, 120, 120));
        return;
    }

    Audio::SoundManager::getInstance().playEffect("coin");
    SaveLoadGame::getInstance().markSessionSaved();
    setStatus("Updated save \"" + newName + "\"!", sf::Color(170, 255, 170));

    setupControls();

    if (_exitAfterSave) {
        if (auto* manager = getSceneManager()) {
            if (auto* window = manager->getRenderWindow()) {
                window->close();
            }
        }
    } else if (_onSuccessfulSave) {
        _onSuccessfulSave();
    }
}

void SaveGameScene::deleteExistingSave(const std::string& saveId) {
    if (SaveLoadGame::getInstance().deleteSave(saveId)) {
        Audio::SoundManager::getInstance().playEffect("kick");
        setStatus("Deleted save file.", sf::Color(255, 180, 120));
        setupControls();
    } else {
        setStatus("Could not delete save file.", sf::Color(255, 120, 120));
    }
}

void SaveGameScene::changePage(int delta) {
    const int totalSaves = static_cast<int>(_allSaves.size());
    const int maxPages = std::max(1, (totalSaves + SavesPerPage - 1) / SavesPerPage);
    _currentPage = std::clamp(_currentPage + delta, 0, maxPages - 1);
    Audio::SoundManager::getInstance().playEffect("select_button");
    setupControls();
}

void SaveGameScene::setupControls() {
    _buttonMenu.clear();
    _existingNameInputs.clear();

    _buttonMenu.setMouseEnabled(true);
    _buttonMenu.setKeyboardEnabled(true);
    _buttonMenu.setWasdEnabled(false);

    _allSaves = SaveLoadGame::getInstance().getAllSaves();
    const int totalSaves = static_cast<int>(_allSaves.size());
    const int maxPages = std::max(1, (totalSaves + SavesPerPage - 1) / SavesPerPage);
    _currentPage = std::clamp(_currentPage, 0, maxPages - 1);

    std::string defaultName = "";
    if (const auto* session = SaveLoadGame::getInstance().getCurrentSession()) {
        defaultName = SaveLoadGame::getInstance().getDefaultSaveName(*session);
    }

    // 1. New Save Input & Button
    _newSaveInput = std::make_shared<UI::TextInput>(
        sf::Vector2f{kCardX + 170.0f, 263.0f},
        sf::Vector2f{600.0f, 46.0f},
        sf::Color(45, 75, 115),
        "",
        20,
        defaultName,
        10.0f
    );
    _newSaveInput->setMaxLength(24);
    _buttonMenu.addButton(_newSaveInput);

    auto newSaveBtn = std::make_shared<UI::Button>(
        sf::Vector2f{kCardX + 790.0f, 263.0f},
        sf::Vector2f{250.0f, 46.0f},
        sf::Color(46, 139, 87),
        "+ SAVE AS NEW",
        20,
        10.0f
    );
    newSaveBtn->setCommand(std::make_unique<FunctionalCommand>(
        "CreateSave",
        [this]() { createNewSave(); }
    ));
    _buttonMenu.addButton(newSaveBtn);

    // 2. Existing Saves for Current Page
    const int startIdx = _currentPage * SavesPerPage;
    const int endIdx = std::min(startIdx + SavesPerPage, totalSaves);

    for (int i = startIdx; i < endIdx; ++i) {
        const int slotOnPage = i - startIdx;
        const float rowY = 385.0f + static_cast<float>(slotOnPage) * 115.0f;
        const std::string saveId = _allSaves[static_cast<std::size_t>(i)].id;
        const std::string saveName = _allSaves[static_cast<std::size_t>(i)].name;

        // Editable save name
        auto nameInput = std::make_shared<UI::TextInput>(
            sf::Vector2f{kCardX + 25.0f, rowY + 12.0f},
            sf::Vector2f{560.0f, 44.0f},
            sf::Color(45, 75, 115),
            "",
            20,
            saveName,
            10.0f
        );
        nameInput->setMaxLength(24);
        _existingNameInputs.push_back(nameInput);
        _buttonMenu.addButton(nameInput);

        const int inputIdx = static_cast<int>(_existingNameInputs.size() - 1);

        // Overwrite Button
        auto overwriteBtn = std::make_shared<UI::Button>(
            sf::Vector2f{kCardX + 610.0f, rowY + 12.0f},
            sf::Vector2f{230.0f, 44.0f},
            sf::Color(70, 130, 180),
            "OVERWRITE",
            19,
            10.0f
        );
        overwriteBtn->setCommand(std::make_unique<FunctionalCommand>(
            "Overwrite_" + saveId,
            [this, saveId, inputIdx]() { overwriteExistingSave(saveId, inputIdx); }
        ));
        _buttonMenu.addButton(overwriteBtn);

        // Delete Button
        auto deleteBtn = std::make_shared<UI::Button>(
            sf::Vector2f{kCardX + 860.0f, rowY + 12.0f},
            sf::Vector2f{180.0f, 44.0f},
            sf::Color(180, 60, 60),
            "DELETE",
            19,
            10.0f
        );
        deleteBtn->setCommand(std::make_unique<FunctionalCommand>(
            "Delete_" + saveId,
            [this, saveId]() { deleteExistingSave(saveId); }
        ));
        _buttonMenu.addButton(deleteBtn);
    }

    // 3. Pagination Buttons (if > 1 page)
    if (maxPages > 1) {
        if (_currentPage > 0) {
            auto prevBtn = std::make_shared<UI::Button>(
                sf::Vector2f{730.0f, 630.0f},
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
                sf::Vector2f{1080.0f, 630.0f},
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

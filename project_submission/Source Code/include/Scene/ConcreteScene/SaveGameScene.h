#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

#include "Button/ButtonMenu.h"
#include "Game/Snapshot/SaveLoadGame.h"
#include "Scene/Scene.h"

namespace UI {
class TextInput;
}

class SaveGameScene : public Scene {
public:
    explicit SaveGameScene(
        bool exitAfterSave = false,
        std::function<void()> onSuccessfulSave = {}
    );
    ~SaveGameScene() override = default;

    void onEnter() override;
    void handleInput(const sf::Event& event) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

private:
    void setupControls();
    void createNewSave();
    void overwriteExistingSave(const std::string& saveId, int inputIndex);
    void deleteExistingSave(const std::string& saveId);
    void setStatus(const std::string& status, sf::Color color = sf::Color::White);
    void changePage(int delta);

    bool _exitAfterSave = false;
    std::function<void()> _onSuccessfulSave;
    UI::ButtonMenu _buttonMenu;
    std::shared_ptr<UI::TextInput> _newSaveInput;
    std::vector<SaveLoadGame::SaveInfo> _allSaves;
    std::vector<std::shared_ptr<UI::TextInput>> _existingNameInputs;

    int _currentPage = 0;
    static constexpr int SavesPerPage = 2;

    sf::Text _titleText;
    sf::Text _statusText;
};

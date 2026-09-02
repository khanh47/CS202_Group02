#pragma once
#include <cstddef>
#include <memory>
#include <vector>
#include <string>
#include "Button.h"
#include "Commands/ICommand.h"

namespace UI {
class ButtonMenu {
private:
    struct LayoutProperties {
        sf::Vector2f startPosition = {20.0f, 80.0f};
        sf::Vector2f buttonSize = {220.0f, 60.0f};
        float spacing = 70.0f;
        bool horizontal = false;
        sf::Color defaultColor = sf::Color(100, 149, 237);
        unsigned int defaultCharSize = 28;
    };

    std::vector<std::shared_ptr<Button>> _buttonMenu;
    LayoutProperties _layout;
    int _focusedIndex = 0;
    bool _mouseEnabled = true;
    bool _keyboardEnabled = true;
    bool _wasdEnabled = true;

    void syncFocus();
    void clearFocus();

public:
    ButtonMenu() = default;

    void setLayoutProperties(const sf::Vector2f& startPosition,
                             const sf::Vector2f& buttonSize,
                             float spacing,
                             bool horizontal,
                             const sf::Color& defaultColor,
                             unsigned int defaultCharSize);

    void addButton(const std::shared_ptr<Button>& button);
    void addButtonAuto(const std::string& text, std::unique_ptr<ICommand> command);
    void addButtonAuto(const std::string& text, unsigned int charSize, 
                       std::unique_ptr<ICommand> command, 
                       const sf::Color& color = sf::Color(100, 149, 237));
    void addMainMenuButtonAuto(const std::string& text, std::unique_ptr<ICommand> command);
    void addMainMenuButtonAuto(const std::string& text, unsigned int charSize, 
                       std::unique_ptr<ICommand> command, 
                       const sf::Color& color = sf::Color(100, 149, 237));
    void addToggleButtonAuto(const std::string& text, bool initialState, std::unique_ptr<ICommand> command);
    void processEvent(const sf::Event& event);
    void updateVisuals(float deltaTime);
    void render(sf::RenderTarget& target);

    void setMouseEnabled(bool enabled);
    void setKeyboardEnabled(bool enabled);
    void setWasdEnabled(bool enabled) noexcept {
        _wasdEnabled = enabled;
    }

    // Deprecated wrappers for backwards compat
    void setMouseOnly(bool mouseOnly) {
        setMouseEnabled(mouseOnly);
        setKeyboardEnabled(!mouseOnly);
    }
    void setArrowKeysOnly(bool arrowKeysOnly) noexcept {
        setWasdEnabled(!arrowKeysOnly);
    }
    void setFocusedIndex(int index);
    int getFocusedIndex() const { return _focusedIndex; }
    std::shared_ptr<Button> getButton(std::size_t index) const {
        if (index < _buttonMenu.size()) return _buttonMenu[index];
        return nullptr;
    }
    void clear();
    std::size_t size() const;
};
} // namespace UI

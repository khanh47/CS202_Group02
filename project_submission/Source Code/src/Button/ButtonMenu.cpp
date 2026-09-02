#include "Button/ButtonMenu.h"
#include "Button/Dropdown.h"
#include "Button/TextInput.h"
#include "Button/ToggleButton.h"
#include "Button/MainMenuButton.h"
#include "Audio/SoundManager.h"

namespace UI {

void ButtonMenu::setLayoutProperties(const sf::Vector2f& startPosition,
                                     const sf::Vector2f& buttonSize,
                                     float spacing,
                                     bool horizontal,
                                     const sf::Color& defaultColor,
                                     unsigned int defaultCharSize) {
    _layout.startPosition = startPosition;
    _layout.buttonSize = buttonSize;
    _layout.spacing = spacing;
    _layout.horizontal = horizontal;
    _layout.defaultColor = defaultColor;
    _layout.defaultCharSize = defaultCharSize;

    for (std::size_t i = 0; i < _buttonMenu.size(); ++i) {
        const float offset = static_cast<float>(i) * _layout.spacing;
        const sf::Vector2f position = _layout.horizontal
            ? sf::Vector2f(_layout.startPosition.x + offset, _layout.startPosition.y)
            : sf::Vector2f(_layout.startPosition.x, _layout.startPosition.y + offset);

        _buttonMenu[i]->setSize(_layout.buttonSize);
        _buttonMenu[i]->setPosition(position);
    }
}

void ButtonMenu::addButton(const std::shared_ptr<Button>& button) {
    if (!button) {
        return;
    }

    _buttonMenu.push_back(button);
    if (_keyboardEnabled) {
        syncFocus();
    } else {
        button->setFocused(false);
    }
}

void ButtonMenu::addButtonAuto(const std::string& text, std::unique_ptr<ICommand> command) {
    addButtonAuto(text, _layout.defaultCharSize, std::move(command), _layout.defaultColor);
}

void ButtonMenu::addButtonAuto(const std::string& text, unsigned int charSize, 
                                std::unique_ptr<ICommand> command, const sf::Color& color) {
    const float offset = static_cast<float>(_buttonMenu.size()) * _layout.spacing;
    const sf::Vector2f position = _layout.horizontal
        ? sf::Vector2f(_layout.startPosition.x + offset, _layout.startPosition.y)
        : sf::Vector2f(_layout.startPosition.x, _layout.startPosition.y + offset);

    auto button = std::make_shared<Button>(
        position, _layout.buttonSize, color, text, charSize, 20.0f
    );
    button->setCommand(std::move(command));
    addButton(button);
}

void ButtonMenu::addMainMenuButtonAuto(const std::string& text, std::unique_ptr<ICommand> command) {
    addMainMenuButtonAuto(text, _layout.defaultCharSize, std::move(command), _layout.defaultColor);
}

void ButtonMenu::addMainMenuButtonAuto(const std::string& text, unsigned int charSize, 
                                std::unique_ptr<ICommand> command, 
                                const sf::Color& color) {
    const float offset = static_cast<float>(_buttonMenu.size()) * _layout.spacing;
    const sf::Vector2f position = _layout.horizontal
        ? sf::Vector2f(_layout.startPosition.x + offset, _layout.startPosition.y)
        : sf::Vector2f(_layout.startPosition.x, _layout.startPosition.y + offset);

    auto button = std::make_shared<MainMenuButton>(
        position, _layout.buttonSize, color, text, charSize
    );
    button->setCommand(std::move(command));
    addButton(button);
}

void ButtonMenu::addToggleButtonAuto(const std::string& text, bool initialState, std::unique_ptr<ICommand> command) {
    const float offset = static_cast<float>(_buttonMenu.size()) * _layout.spacing;
    const sf::Vector2f position = _layout.horizontal
        ? sf::Vector2f(_layout.startPosition.x + offset, _layout.startPosition.y)
        : sf::Vector2f(_layout.startPosition.x, _layout.startPosition.y + offset);

    auto toggleButton = std::make_shared<ToggleButton>(
        position, _layout.buttonSize, _layout.defaultColor, text, _layout.defaultCharSize, initialState, 20.0f
    );
    toggleButton->setCommand(std::move(command));
    addButton(toggleButton);
}

void ButtonMenu::processEvent(const sf::Event& event) {
    if (_buttonMenu.empty()) {
        return;
    }

    const bool isMouseEvent = event.is<sf::Event::MouseButtonPressed>()
                           || event.is<sf::Event::MouseButtonReleased>()
                           || event.is<sf::Event::MouseMoved>()
                           || event.is<sf::Event::MouseWheelScrolled>();

    // 1. Mouse handling (click + hover visuals) only when event is a mouse event
    if (_mouseEnabled && isMouseEvent) {
        const std::vector<std::shared_ptr<Button>> buttons = _buttonMenu;

        if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
            mouseEvent && mouseEvent->button == sf::Mouse::Button::Left) {
            const sf::Vector2f mousePosition = {
                static_cast<float>(mouseEvent->position.x),
                static_cast<float>(mouseEvent->position.y)
            };

            for (auto it = buttons.rbegin(); it != buttons.rend(); ++it) {
                const auto dropdown = std::dynamic_pointer_cast<Dropdown>(*it);
                if (dropdown && dropdown->isOpen()) {
                    dropdown->processEvent(event);
                    return;
                }
            }

            for (auto it = buttons.rbegin(); it != buttons.rend(); ++it) {
                if ((*it)->contains(mousePosition)) {
                    (*it)->processEvent(event);
                    // Sync focus to clicked button when keyboard also enabled
                    if (_keyboardEnabled) {
                        auto found = std::find(_buttonMenu.begin(), _buttonMenu.end(), *it);
                        if (found != _buttonMenu.end()) {
                            _focusedIndex = static_cast<int>(std::distance(_buttonMenu.begin(), found));
                            syncFocus();
                        }
                    }
                    return;
                }
            }
        }

        for (const std::shared_ptr<Button>& button : buttons) {
            button->processEvent(event);
        }
    }

    // 2. Keyboard handling if keyboard enabled
    if (_keyboardEnabled) {
        if (event.is<sf::Event::TextEntered>()) {
            if (_focusedIndex >= 0 && _focusedIndex < static_cast<int>(_buttonMenu.size())) {
                _buttonMenu[static_cast<std::size_t>(_focusedIndex)]->processEvent(event);
            }
            return;
        }

        if (auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
            if (_focusedIndex >= 0 && _focusedIndex < static_cast<int>(_buttonMenu.size())) {
                const std::shared_ptr<Button> focusedButton =
                    _buttonMenu[static_cast<std::size_t>(_focusedIndex)];
                focusedButton->processEvent(event);
            }

            if (_layout.horizontal) {
                if (keyEvent->code == sf::Keyboard::Key::Left
                    || (_wasdEnabled && keyEvent->code == sf::Keyboard::Key::A)) {
                    _focusedIndex = (_focusedIndex - 1 + static_cast<int>(_buttonMenu.size())) % static_cast<int>(_buttonMenu.size());
                    syncFocus();
                } else if (keyEvent->code == sf::Keyboard::Key::Right
                           || (_wasdEnabled && keyEvent->code == sf::Keyboard::Key::D)) {
                    _focusedIndex = (_focusedIndex + 1) % static_cast<int>(_buttonMenu.size());
                    syncFocus();
                }
            } else {
                if (keyEvent->code == sf::Keyboard::Key::Up
                    || (_wasdEnabled && keyEvent->code == sf::Keyboard::Key::W)) {
                    _focusedIndex = (_focusedIndex - 1 + static_cast<int>(_buttonMenu.size())) % static_cast<int>(_buttonMenu.size());
                    syncFocus();
                } else if (keyEvent->code == sf::Keyboard::Key::Down
                           || (_wasdEnabled && keyEvent->code == sf::Keyboard::Key::S)) {
                    _focusedIndex = (_focusedIndex + 1) % static_cast<int>(_buttonMenu.size());
                    syncFocus();
                }
            }

            if (keyEvent->code == sf::Keyboard::Key::Enter
                || keyEvent->code == sf::Keyboard::Key::Space) {
                if (_focusedIndex >= 0 && _focusedIndex < static_cast<int>(_buttonMenu.size())) {
                    const auto textInput = std::dynamic_pointer_cast<TextInput>(_buttonMenu[static_cast<std::size_t>(_focusedIndex)]);
                    if (!textInput) {
                        Audio::SoundManager::getInstance().playEffect("select_button");
                        _buttonMenu[static_cast<std::size_t>(_focusedIndex)]->execute();
                    }
                }
            }
            return;
        }
    }

    // 3. Hover -> keyboard focus sync if mouse enabled
    if (_mouseEnabled && event.is<sf::Event::MouseMoved>()) {
        // Don't steal focus when a TextInput is editing or dropdown open (preserve typing)
        for (auto &b : _buttonMenu) {
            if (auto dd = std::dynamic_pointer_cast<Dropdown>(b); dd && dd->isOpen()) return;
            if (auto ti = std::dynamic_pointer_cast<TextInput>(b); ti && ti->isEditing()) return;
        }
        bool hoveredButton = false;
        for (std::size_t i = 0; i < _buttonMenu.size(); ++i) {
            if (_buttonMenu[i]->isFocused()) {
                if (static_cast<int>(i) != _focusedIndex && _keyboardEnabled) {
                    _focusedIndex = static_cast<int>(i);
                    syncFocus();
                } else if (!_keyboardEnabled) {
                    _focusedIndex = static_cast<int>(i);
                    syncFocus();
                }
                hoveredButton = true;
                break;
            }
        }
        if (!hoveredButton && !_keyboardEnabled) {
            clearFocus();
        }
    }
}

void ButtonMenu::updateVisuals(float deltaTime) {
    for (const std::shared_ptr<Button>& button : _buttonMenu) {
        if (button) {
            button->updateVisuals(deltaTime);
        }
    }
}

void ButtonMenu::render(sf::RenderTarget& target) {
    for (const std::shared_ptr<Button>& button : _buttonMenu) {
        button->render(target);
    }
}

void ButtonMenu::setMouseEnabled(bool enabled) {
    _mouseEnabled = enabled;
    if (!_keyboardEnabled) {
        if (_mouseEnabled) {
            // mouse-only: no focused outline
            clearFocus();
        }
    } else {
        // keyboard active: ensure focus visible
        if (_mouseEnabled) {
            for (const auto &b : _buttonMenu) b->clearFocus();
        }
        syncFocus();
    }
    if (!_mouseEnabled && !_keyboardEnabled) {
        clearFocus();
    }
}

void ButtonMenu::setKeyboardEnabled(bool enabled) {
    _keyboardEnabled = enabled;
    if (_keyboardEnabled) {
        for (const auto &b : _buttonMenu) b->clearFocus();
        syncFocus();
    } else {
        if (_mouseEnabled) {
            clearFocus();
        } else {
            clearFocus();
        }
    }
}

void ButtonMenu::clear() {
    _buttonMenu.clear();
    _focusedIndex = 0;
}

std::size_t ButtonMenu::size() const {
    return _buttonMenu.size();
}

void ButtonMenu::syncFocus() {
    for (std::size_t i = 0; i < _buttonMenu.size(); ++i) {
        _buttonMenu[i]->setFocused(static_cast<int>(i) == _focusedIndex);
    }
}

void ButtonMenu::clearFocus() {
    for (const std::shared_ptr<Button>& button : _buttonMenu) {
        button->setFocused(false);
    }
}

void ButtonMenu::setFocusedIndex(int index) {
    if (index >= 0 && index < static_cast<int>(_buttonMenu.size())) {
        _focusedIndex = index;
        syncFocus();
    }
}

} // namespace UI

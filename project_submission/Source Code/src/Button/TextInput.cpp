#include "Button/TextInput.h"

#include <cctype>
#include <optional>
#include <utility>

namespace UI {

TextInput::TextInput(
    const sf::Vector2f& position,
    const sf::Vector2f& size,
    const sf::Color& color,
    const std::string& label,
    unsigned int charSize,
    const std::string& initialValue,
    float cornerRadius
)
    : Button(position, size, color, label, charSize, cornerRadius),
      _labelPrefix(label),
      _value(initialValue) {
    refreshLabel();
}

void TextInput::setValue(const std::string& value, bool notify) {
    _value.clear();
    _replaceOnFirstInput = false;
    for (const char character : value) {
        if (_value.size() >= _maxLength) {
            break;
        }
        if (_numericOnly && !std::isdigit(static_cast<unsigned char>(character))) {
            continue;
        }
        _value.push_back(character);
    }
    refreshLabel();
    if (notify) {
        notifyValue();
    }
}

void TextInput::setValueCallback(ValueCallback callback) {
    _valueCallback = std::move(callback);
}

void TextInput::processEvent(const sf::Event& event) {
    Button::processEvent(event);

    if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseEvent->button != sf::Mouse::Button::Left) {
            return;
        }
        const sf::FloatRect bounds{basePosition, baseSize};
        const sf::Vector2f mousePosition = {
            static_cast<float>(mouseEvent->position.x),
            static_cast<float>(mouseEvent->position.y)
        };
        if (bounds.contains(mousePosition)) {
            if (!_editing) {
                _editing = true;
                _replaceOnFirstInput = true;
            }
        } else if (_editing) {
            commit();
        }
        return;
    }

    if (const auto* textEvent = event.getIf<sf::Event::TextEntered>()) {
        // Numeric fields also handle KeyPressed below. This avoids adding the
        // same digit twice on platforms that emit both events for one key.
        if (_numericOnly) {
            return;
        }
        if (!_editing || textEvent->unicode < 32 || textEvent->unicode > 126
            || _maxLength == 0) {
            return;
        }
        appendCharacter(static_cast<char>(textEvent->unicode));
        return;
    }

    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Enter) {
            if (_editing) {
                commit();
            } else {
                _editing = true;
                _replaceOnFirstInput = true;
            }
            return;
        }

        if (keyEvent->code == sf::Keyboard::Key::Up
            || keyEvent->code == sf::Keyboard::Key::Down
            || keyEvent->code == sf::Keyboard::Key::Left
            || keyEvent->code == sf::Keyboard::Key::Right) {
            if (_editing) {
                commit();
            }
            return;
        }

        if (!_editing) {
            return;
        }
        if (keyEvent->code == sf::Keyboard::Key::Backspace) {
            _replaceOnFirstInput = false;
            if (!_value.empty()) {
                _value.pop_back();
                refreshLabel();
                notifyValue();
            }
        } else if (keyEvent->code == sf::Keyboard::Key::Escape) {
            commit();
        } else if (_numericOnly) {
            const auto digitFromKey = [](sf::Keyboard::Key key)
                -> std::optional<char> {
                if (key >= sf::Keyboard::Key::Num0
                    && key <= sf::Keyboard::Key::Num9) {
                    return static_cast<char>(
                        '0' + static_cast<int>(key)
                            - static_cast<int>(sf::Keyboard::Key::Num0)
                    );
                }
                if (key >= sf::Keyboard::Key::Numpad0
                    && key <= sf::Keyboard::Key::Numpad9) {
                    return static_cast<char>(
                        '0' + static_cast<int>(key)
                            - static_cast<int>(sf::Keyboard::Key::Numpad0)
                    );
                }
                return std::nullopt;
            };
            if (const auto digit = digitFromKey(keyEvent->code)) {
                appendCharacter(*digit);
            }
        }
    }
}

void TextInput::render(sf::RenderTarget& target) {
    Button::render(target);
    if (!_editing) {
        return;
    }

    sf::RectangleShape underline({baseSize.x - 24.0f, 2.0f});
    underline.setPosition({basePosition.x + 12.0f, basePosition.y + baseSize.y - 6.0f});
    underline.setFillColor(sf::Color(255, 226, 120));
    target.draw(underline);
}

void TextInput::refreshLabel() {
    if (_labelPrefix.empty()) {
        setText(_value.empty() ? "_" : _value);
    } else {
        setText(_labelPrefix + ": " + (_value.empty() ? "_" : _value));
    }
}

void TextInput::notifyValue() {
    if (_valueCallback) {
        _valueCallback(_value);
    }
}

void TextInput::appendCharacter(char character) {
    if (!_editing || _maxLength == 0
        || (_numericOnly
            && !std::isdigit(static_cast<unsigned char>(character)))) {
        return;
    }
    if (_replaceOnFirstInput) {
        _value.clear();
        _replaceOnFirstInput = false;
    }
    if (_value.size() >= _maxLength) {
        return;
    }
    _value.push_back(character);
    refreshLabel();
    notifyValue();
}

void TextInput::commit() {
    _editing = false;
    _replaceOnFirstInput = false;
    refreshLabel();
    notifyValue();
}

} // namespace UI

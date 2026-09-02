#include "Button/Dropdown.h"

#include <algorithm>
#include <utility>

#include "ResourceManager.h"

namespace UI {

Dropdown::Dropdown(
    const sf::Vector2f& position,
    const sf::Vector2f& size,
    const sf::Color& color,
    const std::string& label,
    unsigned int charSize,
    std::vector<std::string> options,
    std::size_t initialIndex,
    float cornerRadius
)
    : Button(position, size, color, label, charSize, cornerRadius),
      _labelPrefix(label),
      _options(std::move(options)) {
    setSelectedIndex(initialIndex);
}

void Dropdown::setOptions(std::vector<std::string> options) {
    _options = std::move(options);
    if (_options.empty()) {
        _selectedIndex = 0;
    } else {
        _selectedIndex = std::min(_selectedIndex, _options.size() - 1);
    }
    refreshLabel();
}

void Dropdown::setSelectedIndex(std::size_t index, bool notify) {
    if (_options.empty()) {
        _selectedIndex = 0;
        refreshLabel();
        return;
    }
    selectIndex(std::min(index, _options.size() - 1), notify);
}

const std::string& Dropdown::getSelectedValue() const noexcept {
    static const std::string empty;
    return _options.empty() ? empty : _options[_selectedIndex];
}

void Dropdown::setSelectionCallback(SelectionCallback callback) {
    _selectionCallback = std::move(callback);
}

void Dropdown::execute() {
    if (_options.empty()) {
        return;
    }
    selectIndex((_selectedIndex + 1) % _options.size(), true);
}

void Dropdown::processEvent(const sf::Event& event) {
    Button::processEvent(event);

    const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
    if (mouseEvent == nullptr || mouseEvent->button != sf::Mouse::Button::Left) {
        return;
    }

    const sf::Vector2f mousePosition = {
        static_cast<float>(mouseEvent->position.x),
        static_cast<float>(mouseEvent->position.y)
    };
    const sf::FloatRect mainBounds{basePosition, baseSize};
    if (mainBounds.contains(mousePosition)) {
        _open = !_open;
        return;
    }

    if (!_open) {
        return;
    }

    for (std::size_t index = 0; index < _options.size(); ++index) {
        if (!optionBounds(index).contains(mousePosition)) {
            continue;
        }
        selectIndex(index, true);
        _open = false;
        return;
    }

    _open = false;
}

void Dropdown::render(sf::RenderTarget& target) {
    Button::render(target);

    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    arrow.setPoint(0, {basePosition.x + baseSize.x - 30.0f, basePosition.y + 17.0f});
    arrow.setPoint(1, {basePosition.x + baseSize.x - 14.0f, basePosition.y + 17.0f});
    arrow.setPoint(2, {basePosition.x + baseSize.x - 22.0f, basePosition.y + 27.0f});
    arrow.setFillColor(sf::Color::White);
    target.draw(arrow);
}

void Dropdown::renderPopup(sf::RenderTarget& target) {
    if (!_open) {
        return;
    }

    sf::RectangleShape optionBackground({baseSize.x, baseSize.y});
    optionBackground.setFillColor(sf::Color(32, 55, 84, 250));
    optionBackground.setOutlineThickness(2.0f);
    optionBackground.setOutlineColor(sf::Color(150, 205, 255));
    for (std::size_t index = 0; index < _options.size(); ++index) {
        const sf::FloatRect bounds = optionBounds(index);
        optionBackground.setPosition(bounds.position);
        optionBackground.setFillColor(
            index == _selectedIndex
                ? sf::Color(65, 115, 165, 250)
                : sf::Color(32, 55, 84, 250)
        );
        target.draw(optionBackground);

        sf::Text optionText(
            ResourceManager::getInstance().getFont("moon_get"),
            _options[index],
            17
        );
        optionText.setPosition({bounds.position.x + 16.0f, bounds.position.y + 9.0f});
        optionText.setFillColor(sf::Color::White);
        target.draw(optionText);
    }
}

void Dropdown::refreshLabel() {
    setText(
        _labelPrefix + ": "
        + (_options.empty() ? std::string("-") : _options[_selectedIndex])
    );
}

void Dropdown::selectIndex(std::size_t index, bool notify) {
    if (_options.empty()) {
        return;
    }
    _selectedIndex = std::min(index, _options.size() - 1);
    refreshLabel();
    if (notify && _selectionCallback) {
        _selectionCallback(_selectedIndex);
    }
}

sf::FloatRect Dropdown::optionBounds(std::size_t index) const {
    return {
        {basePosition.x,
         basePosition.y + baseSize.y * static_cast<float>(index + 1)},
        {baseSize.x, baseSize.y}
    };
}

} // namespace UI

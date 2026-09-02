#include "Button/CheckBox.h"

#include <utility>

namespace UI {

CheckBox::CheckBox(
    const sf::Vector2f& position,
    const sf::Vector2f& size,
    const sf::Color& color,
    const std::string& label,
    unsigned int charSize,
    bool checked,
    float cornerRadius
)
    : Button(position, size, color, label, charSize, cornerRadius),
      _checked(checked) {}

void CheckBox::setChecked(bool checked, bool notify) {
    setCheckedInternal(checked, notify);
}

void CheckBox::setCheckedCallback(CheckedCallback callback) {
    _checkedCallback = std::move(callback);
}

void CheckBox::execute() {
    setCheckedInternal(!_checked, true);
}

void CheckBox::processEvent(const sf::Event& event) {
    Button::processEvent(event);
    const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>();
    if (mouseEvent == nullptr || mouseEvent->button != sf::Mouse::Button::Left) {
        return;
    }
    const sf::FloatRect bounds{basePosition, baseSize};
    const sf::Vector2f mousePosition = {
        static_cast<float>(mouseEvent->position.x),
        static_cast<float>(mouseEvent->position.y)
    };
    if (bounds.contains(mousePosition)) {
        execute();
    }
}

void CheckBox::render(sf::RenderTarget& target) {
    Button::render(target);

    sf::RectangleShape box({26.0f, 26.0f});
    box.setPosition({basePosition.x + 16.0f, basePosition.y + (baseSize.y - 26.0f) * 0.5f});
    box.setFillColor(_checked ? sf::Color(55, 175, 100) : sf::Color(35, 50, 70));
    box.setOutlineThickness(2.0f);
    box.setOutlineColor(sf::Color::White);
    target.draw(box);

    if (_checked) {
        sf::VertexArray check(sf::PrimitiveType::Lines, 4);
        check[0].position = box.getPosition() + sf::Vector2f{5.0f, 13.0f};
        check[1].position = box.getPosition() + sf::Vector2f{11.0f, 19.0f};
        check[2].position = box.getPosition() + sf::Vector2f{11.0f, 19.0f};
        check[3].position = box.getPosition() + sf::Vector2f{22.0f, 6.0f};
        for (std::size_t index = 0; index < 4; ++index) {
            check[index].color = sf::Color::White;
        }
        target.draw(check);
    }
}

void CheckBox::setCheckedInternal(bool checked, bool notify) {
    _checked = checked;
    if (notify && _checkedCallback) {
        _checkedCallback(_checked);
    }
}

} // namespace UI

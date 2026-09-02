#include "Button/ToggleButton.h"
#include "UI/UIHelpers.h"
#include <algorithm>
#include <cmath>

namespace UI {

ToggleButton::ToggleButton(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color,
                           const std::string& labelText, unsigned int charSize,
                           bool initialState, float cornerRadius)
    : Button(position, size, initialState ? sf::Color(46, 204, 113) : sf::Color(108, 122, 137),
             labelText, charSize, cornerRadius),
      _isToggled(initialState) {
    (void)color;
    updateColors();
}

void ToggleButton::setToggled(bool toggled) {
    _isToggled = toggled;
    updateColors();
}

void ToggleButton::toggle() {
    _isToggled = !_isToggled;
    updateColors();
}

void ToggleButton::setToggleCallback(ToggleCallback callback) {
    _toggleCallback = std::move(callback);
}

void ToggleButton::execute() {
    toggle();
    if (_toggleCallback) {
        _toggleCallback(_isToggled);
    }
    Button::execute();
}

void ToggleButton::updateColors() {
    sf::Color base = _isToggled ? _onColor : _offColor;
    defaultColor = base;
    focusedColor = UI::Helper::brighten(base, 65);
    // Pure mode never draws shape — skip shape fill
}

void ToggleButton::render(sf::RenderTarget& target) {
    // Pure toggle: no outer rectangle, left label right pill
    sf::Vector2f drawPos = basePosition;
    if (_isFocused) drawPos.y -= liftAmount;

    // Text on left, lift with focus
    sf::FloatRect tb = label.getLocalBounds();
    label.setOrigin({tb.position.x, tb.position.y + tb.size.y * 0.5f});
    label.setPosition({drawPos.x + 16.f, drawPos.y + baseSize.y * 0.5f});
    if (_isFocused) label.setFillColor(sf::Color(255, 240, 120));
    else label.setFillColor(sf::Color::White);
    // Outline for readability
    label.setOutlineColor(sf::Color(10, 18, 30));
    label.setOutlineThickness(1.0f);
    if (!label.getString().isEmpty()) target.draw(label);
    label.setOutlineThickness(0.f);
    label.setFillColor(sf::Color::White);

    // 2. Render Rounded Pill Track on right — pure, fits 890 width
    const float trackWidth = 48.0f;
    const float trackHeight = 22.0f;
    const float paddingRight = 16.0f;

    const float trackX = drawPos.x + baseSize.x - trackWidth - paddingRight;
    const float trackY = drawPos.y + (baseSize.y - trackHeight) / 2.0f;

    sf::Color trackFill = _isToggled
        ? (_isFocused ? sf::Color(46, 204, 113) : sf::Color(30, 130, 70))
        : sf::Color(75, 85, 95);
    sf::Color outlineCol = _isFocused ? sf::Color(255,255,255,220) : sf::Color(255,255,255,180);
    sf::ConvexShape track = UI::Helper::makePill({trackX, trackY}, trackWidth, trackHeight, trackFill, outlineCol, _isFocused ? 2.0f : 1.5f, 8);
    target.draw(track);

    // 3. Render Slider Knob (Circular thumb indicator inside pill track)
    const float knobRadius = 8.0f;
    sf::CircleShape knob(knobRadius);
    const float knobY = trackY + (trackHeight - knobRadius * 2.0f) / 2.0f;

    float knobX = 0.0f;
    if (_isToggled) {
        knobX = trackX + trackWidth - knobRadius * 2.0f - 3.0f; // Right position when ON
    } else {
        knobX = trackX + 3.0f;                                  // Left position when OFF
    }

    knob.setPosition({knobX, knobY});
    knob.setFillColor(sf::Color::White);
    target.draw(knob);
}

} // namespace UI

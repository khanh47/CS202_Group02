#include "Button/BarSlider.h"
#include "UI/UIHelpers.h"
#include <algorithm>
#include <cmath>

namespace UI {

BarSlider::BarSlider(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color,
                                                     const std::string& labelText, unsigned int charSize,
                                                     bool initialState, float cornerRadius) 
        : Button(position, size, initialState ? sf::Color(46, 204, 113) : sf::Color(108, 122, 137),
                         labelText, charSize, cornerRadius),
            _isSelected(initialState) {
        (void)color;
        updateColors();
}

BarSlider::BarSlider(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color,
                                                     const std::string& labelText, unsigned int charSize,
                                                     float initialValue, float minValue, float maxValue,
                                                     bool selected, float cornerRadius)
        : Button(position, size, color, labelText, charSize, cornerRadius),
            _isSelected(selected), _value(initialValue), _minValue(minValue), _maxValue(maxValue) {
        // label already set via Button ctor
        updateColors();
}

void BarSlider::setSelected(bool selected) {
    _isSelected = selected;
    updateColors();
}

void BarSlider::select() {
    _isSelected = !_isSelected;
    updateColors();
}

void BarSlider::setSelectCallback(SelectCallback callback) {
    _selectCallback = std::move(callback);
}

void BarSlider::execute() {
    select();
    if (_selectCallback) {
        _selectCallback(_isSelected);
    }
    Button::execute();
}

void BarSlider::updateColors() {
    sf::Color base = _isSelected ? _onColor : _offColor;
    defaultColor = base;
    focusedColor = UI::Helper::brighten(base, 65);
    // Pure numeric mode never draws shape — keep colors for toggle fallback only
    if (_maxValue <= _minValue) shape.setFillColor(defaultColor);
}

void BarSlider::processEvent(const sf::Event& event) {
    auto updateFromMouse = [&](sf::Vector2f mousePos) {
        auto g = UI::Helper::TrackGeom::fromBar(basePosition, baseSize);
        float t = (mousePos.x - g.x) / g.w;
        t = std::clamp(t, 0.f, 1.f);
        setValue(_minValue + t * (_maxValue - _minValue));
    };

    if (const auto* press = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (press->button == sf::Mouse::Button::Left) {
            sf::Vector2f mp{static_cast<float>(press->position.x), static_cast<float>(press->position.y)};
            if (contains(mp)) {
                updateFromMouse(mp);
            }
        }
    }
    if (const auto* drag = event.getIf<sf::Event::MouseMoved>()) {
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            sf::Vector2f mp{static_cast<float>(drag->position.x), static_cast<float>(drag->position.y)};
            if (contains(mp) || isFocused()) {
                // Only drag if mouse is held and we are focused/hovered
                // Check if left is held — we already handle press, now drag
                updateFromMouse(mp);
            }
        }
    }

    // Forward to base for hover/focus/execute handling
    Button::processEvent(event);
}

void BarSlider::render(sf::RenderTarget& target) {
    const bool isNumeric = (_maxValue > _minValue);
    sf::Vector2f drawPos = basePosition;
    if (_isFocused) drawPos.y -= liftAmount;

    // Pure bar: no outer rectangle, text left (16px) on top like toggles, bar below. Keep hitbox 890x60.
    if (isNumeric) {
        // Text on top — left-aligned 16px, lift with focus to match ToggleButton
        sf::FloatRect lb = label.getLocalBounds();
        label.setOrigin({lb.position.x, lb.position.y + lb.size.y * 0.5f});
        label.setPosition({drawPos.x + 16.f, drawPos.y + 14.f});
        if (_isFocused) {
            label.setFillColor(sf::Color(255, 240, 120));
        } else {
            label.setFillColor(sf::Color::White);
        }
        label.setOutlineColor(sf::Color(10, 18, 30));
        label.setOutlineThickness(1.0f);
        if (!label.getString().isEmpty()) target.draw(label);
        label.setOutlineThickness(0.f);
        label.setFillColor(sf::Color::White);
    } else {
        // Non-numeric fallback: draw as before with outer rect (not used for volume)
        shape.setPosition(drawPos);
        shape.setFillColor(_isFocused ? focusedColor : defaultColor);
        shape.setOutlineThickness(_isFocused ? 3.0f : 0.0f);
        shape.setOutlineColor(sf::Color::White);
        updateLayout(drawPos, baseSize);
        target.draw(shape);
        if (icon.has_value()) target.draw(icon.value());
        if (!label.getString().isEmpty()) target.draw(label);
        shape.setPosition(basePosition);
        return;
    }

    // 2. Bar track visuals — pure, fills 890 width with 16px side pad -> 858
    auto geom = UI::Helper::TrackGeom::fromBar(drawPos, baseSize);
    float trackW = geom.w, trackH = geom.h, trackX = geom.x, trackY = geom.y, radius = geom.r;

    sf::ConvexShape bg = UI::Helper::makePill({trackX, trackY}, trackW, trackH, sf::Color(60, 68, 80), _isFocused ? sf::Color(255,255,255,140) : sf::Color(255,255,255,70), 1.0f);
    target.draw(bg);

    float t = std::clamp((_value - _minValue) / (_maxValue - _minValue), 0.f, 1.f);
    if (t > 0.001f) {
        float fillW = std::max(radius*2.f, t * trackW);
        sf::ConvexShape fill = UI::Helper::makePill({trackX, trackY}, fillW, trackH, _isFocused ? sf::Color(70, 220, 130) : sf::Color(46, 204, 113), sf::Color::Transparent, 0.f);
        target.draw(fill);
    }

    float knobRadius = 8.f;
    sf::CircleShape knob(knobRadius);
    knob.setFillColor(sf::Color::White);
    knob.setOutlineThickness(1.5f);
    knob.setOutlineColor(sf::Color(40, 40, 40, 180));
    float knobX = trackX + t * trackW;
    knobX = std::clamp(knobX, trackX + knobRadius, trackX + trackW - knobRadius);
    knob.setPosition({knobX - knobRadius, trackY + trackH*0.5f - knobRadius});
    target.draw(knob);
}

void BarSlider::setValue(float value) {
    float clamped = std::clamp(value, _minValue, _maxValue);
    if (std::abs(clamped - _value) > 1e-6f) {
        _value = clamped;
        if (_valueCallback) {
            _valueCallback(_value);
        }
    }
}

float BarSlider::getValue() const {
    return _value;
}

void BarSlider::setValueCallback(ValueCallback cb) {
    _valueCallback = std::move(cb);
}

void BarSlider::adjust(float delta) {
    setValue(_value + delta);
}

} // namespace UI

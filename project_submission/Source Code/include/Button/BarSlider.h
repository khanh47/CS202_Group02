#pragma once

#include "Button/Button.h"
#include <functional>
#include <string>

namespace UI {

// BarSlider extends Button with state color changes and a rounded pill slider switch indicator.
class BarSlider : public Button {
public:
    using SelectCallback = std::function<void(bool)>;
    using ValueCallback = std::function<void(float)>;

    // Constructor for a simple toggle-style BarSlider
    BarSlider(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color,
                 const std::string& labelText, unsigned int charSize = 24,
                 bool initialState = false, float cornerRadius = 10.0f);

    // Constructor for a numeric value slider (min/max, initial value)
    BarSlider(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color,
                 const std::string& labelText, unsigned int charSize,
                 float initialValue, float minValue, float maxValue,
                 bool selected = false, float cornerRadius = 10.0f);

    ~BarSlider() override = default;

    // Returns current toggle state
    bool isSelected() const { return _isSelected; }

    // Sets toggle state and updates state colors
    void setSelected(bool toggled);

    // Flips current toggle state
    void select();

    // Registers callback function invoked when state changes
    void setSelectCallback(SelectCallback callback);

    // Executes toggle state transition and calls bound commands/callbacks
    void execute() override;

    void processEvent(const sf::Event& event) override;

    // Renders button background, label text, and rounded pill slider switch graphics
    void render(sf::RenderTarget& target) override;

    // Numeric slider API
    void setValue(float value);
    float getValue() const;
    void setValueCallback(ValueCallback cb);
    void adjust(float delta);

private:
    bool _isSelected = false;
    SelectCallback _selectCallback;

    // Numeric slider state
    float _value = 0.f;
    float _minValue = 0.f;
    float _maxValue = 100.f;
    ValueCallback _valueCallback;

    // Color palettes for ON/OFF toggle states
    sf::Color _onColor{46, 204, 113};    // Emerald Green when ON
    sf::Color _offColor{108, 122, 137};  // Slate Gray when OFF

    // Recalculates fill colors based on toggle state
    void updateColors();
};

} // namespace UI

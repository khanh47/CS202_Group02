#pragma once

#include "Button/Button.h"
#include <functional>
#include <string>

namespace UI {

// ToggleButton extends Button with state color changes and a rounded pill slider switch indicator.
class ToggleButton : public Button {
public:
    using ToggleCallback = std::function<void(bool)>;

    ToggleButton(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color,
                 const std::string& labelText, unsigned int charSize = 24,
                 bool initialState = false, float cornerRadius = 10.0f);

    ~ToggleButton() override = default;

    // Returns current toggle state
    bool isToggled() const { return _isToggled; }

    // Sets toggle state and updates state colors
    void setToggled(bool toggled);

    // Flips current toggle state
    void toggle();

    // Registers callback function invoked when state changes
    void setToggleCallback(ToggleCallback callback);

    // Executes toggle state transition and calls bound commands/callbacks
    void execute() override;

    // Renders button background, label text, and rounded pill slider switch graphics
    void render(sf::RenderTarget& target) override;

private:
    bool _isToggled = false;
    ToggleCallback _toggleCallback;

    // Color palettes for ON/OFF toggle states
    sf::Color _onColor{46, 204, 113};    // Emerald Green when ON
    sf::Color _offColor{108, 122, 137};  // Slate Gray when OFF

    // Recalculates fill colors based on toggle state
    void updateColors();
};

} // namespace UI

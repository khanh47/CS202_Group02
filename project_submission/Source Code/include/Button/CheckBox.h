#pragma once

#include "Button/Button.h"

#include <functional>

namespace UI {

class CheckBox : public Button {
public:
    using CheckedCallback = std::function<void(bool)>;

    CheckBox(
        const sf::Vector2f& position,
        const sf::Vector2f& size,
        const sf::Color& color,
        const std::string& label,
        unsigned int charSize,
        bool checked = false,
        float cornerRadius = 10.0f
    );

    bool isChecked() const noexcept { return _checked; }
    void setChecked(bool checked, bool notify = false);
    void setCheckedCallback(CheckedCallback callback);

    void execute() override;
    void processEvent(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;

private:
    bool _checked = false;
    CheckedCallback _checkedCallback;

    void setCheckedInternal(bool checked, bool notify);
};

} // namespace UI

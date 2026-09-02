#pragma once

#include "Button/Button.h"

#include <functional>
#include <string>

namespace UI {

class TextInput : public Button {
public:
    using ValueCallback = std::function<void(const std::string&)>;

    TextInput(
        const sf::Vector2f& position,
        const sf::Vector2f& size,
        const sf::Color& color,
        const std::string& label,
        unsigned int charSize,
        const std::string& initialValue = "",
        float cornerRadius = 10.0f
    );

    void setValue(const std::string& value, bool notify = false);
    const std::string& getValue() const noexcept { return _value; }
    void setValueCallback(ValueCallback callback);
    void setNumericOnly(bool numericOnly) noexcept { _numericOnly = numericOnly; }
    void setMaxLength(std::size_t maxLength) noexcept { _maxLength = maxLength; }
    bool isEditing() const noexcept { return _editing; }

    void processEvent(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;

private:
    std::string _labelPrefix;
    std::string _value;
    ValueCallback _valueCallback;
    bool _editing = false;
    bool _replaceOnFirstInput = false;
    bool _numericOnly = false;
    std::size_t _maxLength = 12;

    void refreshLabel();
    void notifyValue();
    void appendCharacter(char character);
    void commit();
};

} // namespace UI

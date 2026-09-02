#pragma once

#include "Button/Button.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace UI {

class Dropdown : public Button {
public:
    using SelectionCallback = std::function<void(std::size_t)>;

    Dropdown(
        const sf::Vector2f& position,
        const sf::Vector2f& size,
        const sf::Color& color,
        const std::string& label,
        unsigned int charSize,
        std::vector<std::string> options,
        std::size_t initialIndex = 0,
        float cornerRadius = 10.0f
    );

    void setOptions(std::vector<std::string> options);
    void setSelectedIndex(std::size_t index, bool notify = false);
    std::size_t getSelectedIndex() const noexcept { return _selectedIndex; }
    const std::string& getSelectedValue() const noexcept;
    bool isOpen() const noexcept { return _open; }
    void setSelectionCallback(SelectionCallback callback);

    void execute() override;
    void processEvent(const sf::Event& event) override;
    void render(sf::RenderTarget& target) override;
    void renderPopup(sf::RenderTarget& target);

private:
    std::string _labelPrefix;
    std::vector<std::string> _options;
    std::size_t _selectedIndex = 0;
    bool _open = false;
    SelectionCallback _selectionCallback;

    void refreshLabel();
    void selectIndex(std::size_t index, bool notify);
    sf::FloatRect optionBounds(std::size_t index) const;
};

} // namespace UI

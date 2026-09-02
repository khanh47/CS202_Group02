#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>
#include <memory>
#include <optional>
#include "Commands/ICommand.h"
#include "Button/Button.h"

namespace UI {

class MainMenuButton: public Button {
public:
    MainMenuButton(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color, 
           const std::string& text, unsigned int charSize); 
    void render(sf::RenderTarget& target) override;
    void processEvent(const sf::Event& event) override;
private:
    sf::ConvexShape triangle;
};
}

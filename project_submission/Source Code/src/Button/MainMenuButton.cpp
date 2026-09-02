
#include "Button/MainMenuButton.h"
#include "Button/Button.h"
#include "ResourceManager.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/System/Angle.hpp>

namespace UI {

MainMenuButton::MainMenuButton(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color, 
               const std::string& text, unsigned int charSize)
    : Button(position, size, color, text, charSize) {
    defaultColor = sf::Color::White;
    focusedColor = sf::Color::Yellow;
    label.setFont(ResourceManager::getInstance().getFont("SuperMario"));
    label.setOutlineColor(sf::Color::Black);
    label.setOutlineThickness(2.0f);
    
    // Create sharp, retro right-pointing triangle cursor
    triangle.setPointCount(3);
    const float triHeight = std::max(16.0f, static_cast<float>(charSize) * 0.65f);
    const float triWidth = triHeight * 0.8f;
    triangle.setPoint(0, sf::Vector2f(0.0f, -triHeight * 0.5f));
    triangle.setPoint(1, sf::Vector2f(triWidth, 0.0f));
    triangle.setPoint(2, sf::Vector2f(0.0f, triHeight * 0.5f));
    
    triangle.setFillColor(focusedColor);
    triangle.setOutlineColor(sf::Color::Black);
    triangle.setOutlineThickness(2.0f);
}

void MainMenuButton::render(sf::RenderTarget& target) {
    sf::Vector2f drawPos = basePosition;
    if (_isFocused) {
        drawPos.y -= liftAmount;
    }
    if (_isFocused) {
        label.setFillColor(sf::Color::Yellow);
    } else {
        label.setFillColor(sf::Color::White);
    }

    Button::updateLayout(drawPos, baseSize);

    if (!label.getString().isEmpty()) {
        target.draw(label);
    }
    if (_isFocused) {
        const sf::FloatRect textBounds = label.getGlobalBounds();
        const float triWidth = triangle.getPoint(1).x;
        const float cursorX = textBounds.position.x - triWidth - 14.0f;
        const float cursorY = textBounds.position.y + textBounds.size.y * 0.5f;
        triangle.setPosition({cursorX, cursorY});
        target.draw(triangle);
    }
}

void MainMenuButton::processEvent(const sf::Event& event) {
    // 1. HANDLE HOVER (POP-UP EFFECT)
    if (const auto* mouseMove = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f mousePos(static_cast<float>(mouseMove->position.x), static_cast<float>(mouseMove->position.y));
        // IMPORTANT: Check collision against the BASE position, NOT the current moving shape.
        // This prevents the button from jittering when the mouse is at the bottom edge.
        sf::FloatRect staticBounds{basePosition, baseSize};
        _isFocused = staticBounds.contains(mousePos); 

        if (_isFocused) {
            label.setFillColor(focusedColor);
        } else {
            label.setFillColor(defaultColor);
        }
    }

    // 2. HANDLE CLICK EVENT
    if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseEvent->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x), static_cast<float>(mouseEvent->position.y));
            
            // Check click against the static bounds as well to be consistent
            sf::FloatRect staticBounds{basePosition, baseSize};
            if (staticBounds.contains(mousePos)) {
                if (buttonCommand) buttonCommand->execute();
            }
        }
    }
}

} // namespace UI
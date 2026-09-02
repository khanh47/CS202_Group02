
#include "Button/Button.h"
#include "ResourceManager.h"
#include "UI/UIHelpers.h"
#include <SFML/System/String.hpp>
#include <algorithm>
#include <cmath> 

namespace UI {

Button::Button(const sf::Vector2f& position, const sf::Vector2f& size, const sf::Color& color, 
               const std::string& text, unsigned int charSize, 
               float radius, const std::string& iconAlias)
    : label(ResourceManager::getInstance().getFont("moon_get"), text, charSize),
      defaultColor(color), cornerRadius(radius), basePosition(position), baseSize(size) {

    focusedColor = UI::Helper::brighten(color, 80);

    label.setFillColor(sf::Color::White);

    if (!iconAlias.empty()) {
        try {
            sf::Texture& tex = ResourceManager::getInstance().getTexture(iconAlias);
            icon.emplace(tex);
            
            float iconTargetHeight = size.y * 0.6f;
            float scale = iconTargetHeight / tex.getSize().y;
            icon->setScale({scale, scale});
        } catch (const std::exception& e) {
            icon.reset();
        }
    }

    shape.setFillColor(defaultColor);
    updateRoundedShape(position, size);
    updateLayout(position, size);
}

void Button::updateRoundedShape(const sf::Vector2f& position, const sf::Vector2f& size) {
    shape = UI::Helper::makeRoundedRect(position, size, cornerRadius, 10);
}

void Button::updateLayout(const sf::Vector2f& position, const sf::Vector2f& size) {
    float centerX = position.x + size.x / 2.0f;

    // 1. Setup Label Origin
    sf::FloatRect textBounds = label.getLocalBounds();
    label.setOrigin({textBounds.position.x + textBounds.size.x / 2.0f,
                     textBounds.position.y + textBounds.size.y / 2.0f});

    // 2. Position Label at a FIXED vertical coordinate
    // We place the text at 82% of the button's height. 
    // This ensures all button texts are perfectly aligned horizontally.
    float fixedLabelY = position.y + (size.y * 0.82f);
    label.setPosition({centerX, fixedLabelY});
    if (icon.has_value()) {
        sf::FloatRect iconBounds = icon->getLocalBounds();
        
        // 3. Scale icon to fit the reserved upper area
        // We allow the icon to occupy 75% of width and 55% of height
        float maxIconW = size.x * 0.75f; 
        float maxIconH = size.y * 0.55f;

        float scale = std::min(maxIconW / iconBounds.size.x, maxIconH / iconBounds.size.y);
        icon->setScale({scale, scale});

        // 4. Position Icon centered in the UPPER area of the button
        // We place the icon center at 40% of the button's height
        float fixedIconCenterY = position.y + (size.y * 0.40f);
        
        icon->setOrigin({iconBounds.position.x + iconBounds.size.x / 2.0f,
                         iconBounds.position.y + iconBounds.size.y / 2.0f});
        icon->setPosition({centerX, fixedIconCenterY});

    } else {
        // If there's no icon (e.g. Back button), center the text perfectly in the button
        label.setPosition({centerX, position.y + size.y / 2.0f});
    }
}

void Button::setPosition(const sf::Vector2f& position) {
    basePosition = position;
    shape.setPosition(position);
    sf::FloatRect bounds = shape.getLocalBounds();
    sf::Vector2f size(bounds.size.x, bounds.size.y);
    baseSize = size; 
    updateLayout(position, size);
}

void Button::setSize(const sf::Vector2f& size) {
    baseSize = size;
    updateRoundedShape(shape.getPosition(), size);
    updateLayout(shape.getPosition(), size);
}

void Button::setCommand(std::unique_ptr<ICommand> command) {
    buttonCommand = std::move(command);
}

void Button::setFocused(bool focused) {
    _isFocused = focused;
}

void Button::execute() {
    if (buttonCommand) {
        buttonCommand->execute();
    }
}

void Button::processEvent(const sf::Event& event) {
    // 1. HANDLE HOVER (POP-UP EFFECT)
    if (const auto* mouseMove = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f mousePos(static_cast<float>(mouseMove->position.x), static_cast<float>(mouseMove->position.y));
        // IMPORTANT: Check collision against the BASE position, NOT the current moving shape.
        // This prevents the button from jittering when the mouse is at the bottom edge.
        sf::FloatRect staticBounds{basePosition, baseSize};
        _isFocused = staticBounds.contains(mousePos); 

        if (_isFocused) {
            shape.setFillColor(focusedColor);
        } else {
            shape.setFillColor(defaultColor);
        }
    }

    // 2. HANDLE CLICK EVENT
    if (const auto* mouseEvent = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseEvent->button == sf::Mouse::Button::Left) {
            sf::Vector2f mousePos(static_cast<float>(mouseEvent->position.x), static_cast<float>(mouseEvent->position.y));
            
            // Check click against the static bounds as well to be consistent
            sf::FloatRect staticBounds{basePosition, baseSize};
            if (staticBounds.contains(mousePos)) {
                execute();
            }
        }
    }
}

void Button::render(sf::RenderTarget& target) {
    sf::Vector2f drawPos = basePosition;
    if (_isFocused) {
        drawPos.y -= liftAmount;
    }

    shape.setPosition(drawPos);

    if (_isFocused) {
        shape.setFillColor(focusedColor);
    } else {
        shape.setFillColor(defaultColor);
    }

    shape.setOutlineThickness(_isFocused ? 3.0f : 0.0f);
    shape.setOutlineColor(sf::Color::White);

    updateLayout(drawPos, baseSize);

    target.draw(shape);
    if (icon.has_value()) {
        target.draw(icon.value());
    }
    if (!label.getString().isEmpty()) {
        target.draw(label);
    }

    shape.setPosition(basePosition);
}

void Button::setText(const std::string& text) {
    label.setString(text);
    updateLayout(shape.getPosition(), baseSize);
}

std::string Button::getText() const {
    return label.getString().toAnsiString();
}

void Button::setColor(const sf::Color& color) {
    defaultColor = color;
    int fr = std::min(255, color.r + 80);
    int fg = std::min(255, color.g + 80);
    int fb = std::min(255, color.b + 80);
    focusedColor = sf::Color(fr, fg, fb, color.a);
    shape.setFillColor(_isFocused ? focusedColor : defaultColor);
}

} // namespace UI

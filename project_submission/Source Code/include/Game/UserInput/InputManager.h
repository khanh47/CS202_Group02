#include <SFML/Window/Event.hpp>

#include "Game/UserInput/KeyBindings.h"

#pragma once

class InputManager : public KeyBindings {
public:
    InputManager() = default;
    ~InputManager() = default;

    bool handleEvent(const sf::Event& event);

protected:
    virtual bool ActionStart(const sf::Event::KeyPressed& event) = 0;
    virtual bool ActionEnd(const sf::Event::KeyReleased& event) = 0;
};

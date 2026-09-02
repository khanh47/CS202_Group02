#include "Game/UserInput/InputManager.h"

bool InputManager::handleEvent(const sf::Event& event) {
    if (const sf::Event::KeyPressed* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        return ActionStart(*keyPressed);
    }

    if (const sf::Event::KeyReleased* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        return ActionEnd(*keyReleased);
    }

    return false;
}

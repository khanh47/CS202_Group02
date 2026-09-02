#include <SFML/Graphics.hpp>
#include <optional>
#include <unordered_map>

#include "Game/UserInput/Action.h"

#pragma once

class KeyBindings {
private:
    std::unordered_map<sf::Keyboard::Key, ActionType> keyActionMap;
public:
    KeyBindings() = default;
    ~KeyBindings() = default;

    void BindKey(const sf::Keyboard::Key& key, const ActionType& action);
    void UnbindKey(const sf::Keyboard::Key& key);
    void ClearBindings() { keyActionMap.clear(); }
    std::optional<ActionType> GetActionForKey(const sf::Keyboard::Key& key) const;
    const std::unordered_map<sf::Keyboard::Key, ActionType>& getKeyActionMap() const { return keyActionMap; }
}; 

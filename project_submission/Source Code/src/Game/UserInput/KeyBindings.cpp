#include "Game/UserInput/Action.h"
#include "Game/UserInput/KeyBindings.h"

void KeyBindings::BindKey(const sf::Keyboard::Key& key, const ActionType& action) {
    keyActionMap[key] = action;
}

void KeyBindings::UnbindKey(const sf::Keyboard::Key& key) {
    keyActionMap.erase(key);
}

std::optional<ActionType> KeyBindings::GetActionForKey(const sf::Keyboard::Key& key) const {
    auto it = keyActionMap.find(key);
    if (it != keyActionMap.end()) {
        return it->second;
    }

    return std::nullopt;
}

#include "Game/GameSettings.h"

#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

namespace {
std::filesystem::path settingsFilePath() {
    return "assets/SaveGameFiles/settings.json";
}
} // namespace

GameSettings& GameSettings::getInstance() {
    static GameSettings instance;
    return instance;
}

sf::Keyboard::Key GameSettings::getKeyForAction(ActionType action) const {
    switch (action) {
        case ActionType::MoveLeft:
            return keyMoveLeft;
        case ActionType::MoveRight:
            return keyMoveRight;
        case ActionType::MoveUp:
            return keyJump;
        case ActionType::MoveDown:
            return keyMoveDown;
        case ActionType::Attack:
            return keyAttack;
        case ActionType::Interact:
            return keyInteract;
        case ActionType::ToggleFlyMode:
            return keyToggleFlyMode;
        default:
            return sf::Keyboard::Key::Unknown;
    }
}

void GameSettings::setKeyForAction(ActionType action, sf::Keyboard::Key key) {
    switch (action) {
        case ActionType::MoveLeft:
            keyMoveLeft = key;
            break;
        case ActionType::MoveRight:
            keyMoveRight = key;
            break;
        case ActionType::MoveUp:
            keyJump = key;
            break;
        case ActionType::MoveDown:
            keyMoveDown = key;
            break;
        case ActionType::Attack:
            keyAttack = key;
            break;
        case ActionType::Interact:
            keyInteract = key;
            break;
        case ActionType::ToggleFlyMode:
            keyToggleFlyMode = key;
            break;
        default:
            break;
    }
}

sf::Keyboard::Key GameSettings::getKeyForAction2(ActionType action) const {
    switch (action) {
        case ActionType::MoveLeft:
            return key2MoveLeft;
        case ActionType::MoveRight:
            return key2MoveRight;
        case ActionType::MoveUp:
            return key2Jump;
        case ActionType::MoveDown:
            return key2MoveDown;
        case ActionType::Attack:
            return key2Attack;
        case ActionType::Interact:
            return key2Interact;
        case ActionType::ToggleFlyMode:
            return key2ToggleFlyMode;
        default:
            return sf::Keyboard::Key::Unknown;
    }
}

void GameSettings::setKeyForAction2(ActionType action, sf::Keyboard::Key key) {
    switch (action) {
        case ActionType::MoveLeft:
            key2MoveLeft = key;
            break;
        case ActionType::MoveRight:
            key2MoveRight = key;
            break;
        case ActionType::MoveUp:
            key2Jump = key;
            break;
        case ActionType::MoveDown:
            key2MoveDown = key;
            break;
        case ActionType::Attack:
            key2Attack = key;
            break;
        case ActionType::Interact:
            key2Interact = key;
            break;
        case ActionType::ToggleFlyMode:
            key2ToggleFlyMode = key;
            break;
        default:
            break;
    }
}

std::string GameSettings::keyToString(sf::Keyboard::Key key) {
    if (key >= sf::Keyboard::Key::A && key <= sf::Keyboard::Key::Z) {
        char c = static_cast<char>('A' + (static_cast<int>(key) - static_cast<int>(sf::Keyboard::Key::A)));
        return std::string(1, c);
    }
    if (key >= sf::Keyboard::Key::Num0 && key <= sf::Keyboard::Key::Num9) {
        char c = static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(sf::Keyboard::Key::Num0)));
        return std::string(1, c);
    }
    if (key >= sf::Keyboard::Key::Numpad0 && key <= sf::Keyboard::Key::Numpad9) {
        char c = static_cast<char>('0' + (static_cast<int>(key) - static_cast<int>(sf::Keyboard::Key::Numpad0)));
        return "Num " + std::string(1, c);
    }

    switch (key) {
        case sf::Keyboard::Key::Left:      return "Left";
        case sf::Keyboard::Key::Right:     return "Right";
        case sf::Keyboard::Key::Up:        return "Up";
        case sf::Keyboard::Key::Down:      return "Down";
        case sf::Keyboard::Key::Space:     return "Space";
        case sf::Keyboard::Key::Enter:     return "Enter";
        case sf::Keyboard::Key::Escape:    return "Escape";
        case sf::Keyboard::Key::LShift:    return "LShift";
        case sf::Keyboard::Key::RShift:    return "RShift";
        case sf::Keyboard::Key::LControl:  return "LCtrl";
        case sf::Keyboard::Key::RControl:  return "RCtrl";
        case sf::Keyboard::Key::LAlt:      return "LAlt";
        case sf::Keyboard::Key::RAlt:      return "RAlt";
        case sf::Keyboard::Key::Tab:       return "Tab";
        case sf::Keyboard::Key::Backspace: return "Backspace";
        default:
            return "Key " + std::to_string(static_cast<int>(key));
    }
}

void GameSettings::save() const {
    nlohmann::json j;
    j["keyMoveLeft"] = static_cast<int>(keyMoveLeft);
    j["keyMoveRight"] = static_cast<int>(keyMoveRight);
    j["keyJump"] = static_cast<int>(keyJump);
    j["keyMoveDown"] = static_cast<int>(keyMoveDown);
    j["keyAttack"] = static_cast<int>(keyAttack);
    j["keyInteract"] = static_cast<int>(keyInteract);
    j["keyToggleFlyMode"] = static_cast<int>(keyToggleFlyMode);

    j["key2MoveLeft"] = static_cast<int>(key2MoveLeft);
    j["key2MoveRight"] = static_cast<int>(key2MoveRight);
    j["key2Jump"] = static_cast<int>(key2Jump);
    j["key2MoveDown"] = static_cast<int>(key2MoveDown);
    j["key2Attack"] = static_cast<int>(key2Attack);
    j["key2Interact"] = static_cast<int>(key2Interact);
    j["key2ToggleFlyMode"] = static_cast<int>(key2ToggleFlyMode);

    j["musicEnabled"] = musicEnabled;
    j["musicVolume"] = musicVolume;
    j["soundEnabled"] = soundEnabled;
    j["soundVolume"] = soundVolume;

    j["debugDrawGrid"] = debugDrawGrid;
    j["debugDrawCoordinates"] = debugDrawCoordinates;
    j["debugDrawHitbox"] = debugDrawHitbox;
    j["freeCameraMove"] = freeCameraMove;

    const std::filesystem::path path = settingsFilePath();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    std::ofstream outFile(path);
    if (outFile.is_open()) {
        outFile << j.dump(4);
    }
}

void GameSettings::load() {
    std::ifstream inFile(settingsFilePath());
    if (!inFile.is_open()) {
        return;
    }

    nlohmann::json j;
    try {
        inFile >> j;
    } catch (const nlohmann::json::exception&) {
        return;
    }

    keyMoveLeft = static_cast<sf::Keyboard::Key>(j.value("keyMoveLeft", static_cast<int>(keyMoveLeft)));
    keyMoveRight = static_cast<sf::Keyboard::Key>(j.value("keyMoveRight", static_cast<int>(keyMoveRight)));
    keyJump = static_cast<sf::Keyboard::Key>(j.value("keyJump", static_cast<int>(keyJump)));
    keyMoveDown = static_cast<sf::Keyboard::Key>(j.value("keyMoveDown", static_cast<int>(keyMoveDown)));
    keyAttack = static_cast<sf::Keyboard::Key>(j.value("keyAttack", static_cast<int>(keyAttack)));
    keyInteract = static_cast<sf::Keyboard::Key>(j.value("keyInteract", static_cast<int>(keyInteract)));
    keyToggleFlyMode = static_cast<sf::Keyboard::Key>(j.value("keyToggleFlyMode", static_cast<int>(keyToggleFlyMode)));

    key2MoveLeft = static_cast<sf::Keyboard::Key>(j.value("key2MoveLeft", static_cast<int>(key2MoveLeft)));
    key2MoveRight = static_cast<sf::Keyboard::Key>(j.value("key2MoveRight", static_cast<int>(key2MoveRight)));
    key2Jump = static_cast<sf::Keyboard::Key>(j.value("key2Jump", static_cast<int>(key2Jump)));
    key2MoveDown = static_cast<sf::Keyboard::Key>(j.value("key2MoveDown", static_cast<int>(key2MoveDown)));
    key2Attack = static_cast<sf::Keyboard::Key>(j.value("key2Attack", static_cast<int>(key2Attack)));
    key2Interact = static_cast<sf::Keyboard::Key>(j.value("key2Interact", static_cast<int>(key2Interact)));
    key2ToggleFlyMode = static_cast<sf::Keyboard::Key>(j.value("key2ToggleFlyMode", static_cast<int>(key2ToggleFlyMode)));

    musicEnabled = j.value("musicEnabled", musicEnabled);
    musicVolume = j.value("musicVolume", musicVolume);
    soundEnabled = j.value("soundEnabled", soundEnabled);
    soundVolume = j.value("soundVolume", soundVolume);

    debugDrawGrid = j.value("debugDrawGrid", debugDrawGrid);
    debugDrawCoordinates = j.value("debugDrawCoordinates", debugDrawCoordinates);
    debugDrawHitbox = j.value("debugDrawHitbox", debugDrawHitbox);
    freeCameraMove = j.value("freeCameraMove", freeCameraMove);
}

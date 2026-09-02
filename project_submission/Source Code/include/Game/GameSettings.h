#include <SFML/Window/Keyboard.hpp>
#include <string>
#include "Game/UserInput/Action.h"

#include <string>

enum class GameMode {
    Coop,
    Solo,
    Minigame
};

enum class MinigameMode {
    TwoPlayer,
    VsAi
};

class GameSettings {
public:
    static GameSettings& getInstance();

    GameMode gameMode = GameMode::Coop;
    MinigameMode minigameMode = MinigameMode::TwoPlayer;
    std::string player1Character = "mario";
    std::string characterSelectHovered = "mario";
    bool isCharacterSelectActive = false;
    bool isInGameSceneActive = false;
    bool isLevelSelectActive = false;

    bool debugDrawGrid = false;
    bool debugDrawCoordinates = false;
    bool debugDrawHitbox = false;
    bool freeCameraMove = false;

    bool musicEnabled = true;
    float musicVolume = 80.f;
    bool soundEnabled = true;
    float soundVolume = 80.f;

    // Player controls (keybindings)
    sf::Keyboard::Key keyMoveLeft = sf::Keyboard::Key::A;
    sf::Keyboard::Key keyMoveRight = sf::Keyboard::Key::D;
    sf::Keyboard::Key keyJump = sf::Keyboard::Key::W;
    sf::Keyboard::Key keyMoveDown = sf::Keyboard::Key::S;
    sf::Keyboard::Key keyAttack = sf::Keyboard::Key::X;
    sf::Keyboard::Key keyInteract = sf::Keyboard::Key::LShift;
    sf::Keyboard::Key keyToggleFlyMode = sf::Keyboard::Key::F;

    // Player 2 controls (keybindings)
    sf::Keyboard::Key key2MoveLeft = sf::Keyboard::Key::Left;
    sf::Keyboard::Key key2MoveRight = sf::Keyboard::Key::Right;
    sf::Keyboard::Key key2Jump = sf::Keyboard::Key::Up;
    sf::Keyboard::Key key2MoveDown = sf::Keyboard::Key::Down;
    sf::Keyboard::Key key2Attack = sf::Keyboard::Key::M;
    sf::Keyboard::Key key2Interact = sf::Keyboard::Key::RShift;
    sf::Keyboard::Key key2ToggleFlyMode = sf::Keyboard::Key::F;

    sf::Keyboard::Key getKeyForAction(ActionType action) const;
    void setKeyForAction(ActionType action, sf::Keyboard::Key key);

    sf::Keyboard::Key getKeyForAction2(ActionType action) const;
    void setKeyForAction2(ActionType action, sf::Keyboard::Key key);

    static std::string keyToString(sf::Keyboard::Key key);

    void save() const;
    void load();

private:
    GameSettings() = default;
    ~GameSettings() = default;
    
    // Delete copy/move constructors and assignment operators
    GameSettings(const GameSettings&) = delete;
    GameSettings& operator=(const GameSettings&) = delete;
    GameSettings(GameSettings&&) = delete;
    GameSettings& operator=(GameSettings&&) = delete;
};

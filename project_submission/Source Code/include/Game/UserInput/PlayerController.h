#pragma once

#include "Game/Objects/Player/Player.h"
#include "Game/UserInput/InputManager.h"

#include <SFML/Window/Event.hpp>

class GameWorld;

class PlayerController: public InputManager {
public:
    enum class ControlScheme {
        Wasd,
        ArrowKeys
    };

    explicit PlayerController(Player& player, GameWorld& gameWorld, ControlScheme controlScheme = ControlScheme::Wasd);

    bool ActionStart(const sf::Event::KeyPressed& event) override;
    bool ActionEnd(const sf::Event::KeyReleased& event) override;

    void syncStateWithKeyboard();
    bool isPlayerPendingDestroy() const { return _player.isPendingDestroy(); }
    void refreshBindings();

private:
    void bindControls(ControlScheme controlScheme);
    void applyPressAction(ActionType action);
    void applyReleaseAction(ActionType action);

    Player& _player;
    GameWorld& _gameWorld;
    ControlScheme _controlScheme;
};

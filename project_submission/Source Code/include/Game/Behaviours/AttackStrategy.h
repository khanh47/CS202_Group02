#pragma once

class Player;
class GameWorld;

/**
 * @brief Strategy interface for player attacks, implementing the Strategy Pattern.
 * Allows Mario states (e.g. Fire Mario vs Normal/Super Mario) to dynamically switch attack behavior.
 */
class IAttackStrategy {
public:
    virtual ~IAttackStrategy() = default;

    /**
     * @brief Executes the attack action for the player.
     * @param player Reference to the attacking player entity.
     * @param world Reference to the game world where attack objects are spawned.
     */
    virtual void executeAttack(Player& player, GameWorld& world) = 0;
};

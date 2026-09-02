#pragma once

#include "../Item.h"

class Player;

class CheckpointFlag : public Item {
public:
    CheckpointFlag();
    explicit CheckpointFlag(sf::Texture& texture);
    ~CheckpointFlag() override = default;

    void onContact(
        GameObject& other,
        const b2ContactData& contactData,
        b2ShapeId ownShape
    ) override;
    void spawn(
        const PhysicsWorld& physicsWorld,
        sf::Vector2f spawnPixels,
        sf::Vector2f hitboxPixels
    ) override;

    bool isTriggered() const noexcept { return _triggered; }
    void restoreTriggered(bool triggered) noexcept { _triggered = triggered; }

protected:
    void onUpdateVisuals(float deltaTime) override;
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;

    // Extension point for checkpoint/save logic.
    virtual void onCheckpointReached(Player& player);

private:
    void configureVisuals(sf::Texture& texture);

    bool _triggered = false;
};

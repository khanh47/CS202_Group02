#include "Game/Objects/Item/ConcreteItems/CheckpointFlag.h"

#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/GameObject.h"
#include "Game/World/GameWorld.h"
#include "Game/Objects/Player/Player.h"
#include "Physics/CollisionFilter.h"
#include "ResourceManager.h"
#include <SFML/System/Vector2.hpp>

namespace {
constexpr sf::Vector2f checkpointFlagVisualSize{96.0f, 96.0f};
constexpr sf::Vector2f checkpointFlagHitboxSize{48.0f, 96.0f};
}

class GameWorld;

CheckpointFlag::CheckpointFlag() : Item() {
    configureVisuals(
        ResourceManager::getInstance().getTexture(
            "checkpoint_flag_spritesheet"
        )
    );
}

CheckpointFlag::CheckpointFlag(sf::Texture& texture) : Item() {
    configureVisuals(texture);
}

void CheckpointFlag::configureVisuals(sf::Texture& texture) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        // Reuse Flagpole's visuals until CheckpointFlag gets its own asset.
        animatable->configureVisuals(texture, "checkpoint_flag");
    }
}

void CheckpointFlag::onUpdateVisuals(float deltaTime) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(
            deltaTime,
            checkpointFlagVisualSize
        );
    }
}

void CheckpointFlag::spawn(
    const PhysicsWorld& physicsWorld,
    sf::Vector2f spawnPixels,
    sf::Vector2f hitboxPixels
) {
    GameObject::spawn(physicsWorld, spawnPixels, hitboxPixels);
    updateHitboxSize(checkpointFlagHitboxSize);
}

void CheckpointFlag::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_staticBody;
}

void CheckpointFlag::onCreateShapeDef(b2ShapeDef& def) {
    def.isSensor = true;
    def.enableContactEvents = false;
    def.enableSensorEvents = true;
    def.density = 0.0f;
    def.filter.categoryBits = CollisionFilter::PICKUP;
    def.filter.maskBits = CollisionFilter::PLAYER;
}

void CheckpointFlag::onContact(
    GameObject& other,
    const b2ContactData&,
    b2ShapeId
) {
    if (_triggered) {
        return;
    }

    auto* player = dynamic_cast<Player*>(&other);
    if (!player) {
        return;
    }

    _triggered = true;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->playAnimation("captured");
    }
    onCheckpointReached(*player);
}

void CheckpointFlag::onCheckpointReached(Player& player) {
    (void)player;
    auto* gameWorld = player.getGameWorld();
    gameWorld->saveCheckpoint(player.getPosition());
}

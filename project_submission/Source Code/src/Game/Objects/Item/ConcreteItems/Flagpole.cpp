#include "Game/Objects/Item/ConcreteItems/Flagpole.h"

#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/GameObject.h"
#include "Game/Objects/Player/Player.h"
#include "Game/World/GameWorld.h"
#include "Physics/CollisionFilter.h"
#include "ResourceManager.h"

namespace {
constexpr sf::Vector2f kFlagpoleVisualSize{128.f, 896.f};
}

Flagpole::Flagpole() : Item() {
    configureVisuals(ResourceManager::getInstance().getTexture("goal_flag_spritesheet"));
}

Flagpole::Flagpole(sf::Texture& texture) : Item() {
    configureVisuals(texture);
}

void Flagpole::configureVisuals(sf::Texture& texture) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "flagpole");
    }
}

void Flagpole::onUpdateVisuals(float deltaTime) {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(deltaTime, kFlagpoleVisualSize);
    }
}

void Flagpole::spawn(const PhysicsWorld &physicsWorld, sf::Vector2f spawnPixels, sf::Vector2f hitboxPixels) {
    GameObject::spawn(physicsWorld, spawnPixels, hitboxPixels);
    updateHitboxSize({16, 896});
}

void Flagpole::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_staticBody;
}

void Flagpole::onCreateShapeDef(b2ShapeDef& def) {
    def.isSensor = true;
    def.enableContactEvents = false;
    def.enableSensorEvents = true;
    def.density = 0.0f;

    def.filter.categoryBits = CollisionFilter::PICKUP;
    def.filter.maskBits = CollisionFilter::PLAYER;
}

void Flagpole::onContact(GameObject& other, const b2ContactData&, b2ShapeId) {
    if (_triggered) {
        return;
    }

    auto* player = dynamic_cast<Player*>(&other);
    if (!player || !player->getGameWorld()) {
        return;
    }

    _triggered = true;
    player->getGameWorld()->reachFlagpole(getPosition());
}

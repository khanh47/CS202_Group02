#include "Game/Objects/Item/ConcreteItems/FireFlower.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Player/Player.h"
#include "Physics/CollisionFilter.h"

FireFlower::FireFlower() : Item() {
}

FireFlower::FireFlower(sf::Texture& texture) : Item() {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "fire_flower");
    }
}

void FireFlower::onPickup(Player& player) {
    if (player.getState()) {
        player.getState()->handleFireFlower(player);
    }
    destroy();
}

void FireFlower::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_staticBody;
}

void FireFlower::onCreateShapeDef(b2ShapeDef& def) {
    def.isSensor = true;
    def.density = 0.0f;

    def.filter.categoryBits = CollisionFilter::PICKUP;
    def.filter.maskBits = CollisionFilter::PLAYER;
}

void FireFlower::onUpdateVisuals(float deltaTime) {
    Item::onUpdateVisuals(deltaTime);
}

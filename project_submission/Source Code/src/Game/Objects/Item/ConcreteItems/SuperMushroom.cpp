#include "Game/Objects/Item/ConcreteItems/SuperMushroom.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Player/Player.h"
#include "Physics/CollisionFilter.h"

SuperMushroom::SuperMushroom() : Item() {
}

SuperMushroom::SuperMushroom(sf::Texture& texture) : Item() {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "super_mushroom");
    }
}

void SuperMushroom::onPickup(Player& player) {
    if (player.getState()) {
        player.getState()->handleSuperMushroom(player);
    }
    destroy();
}

void SuperMushroom::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_staticBody;
}

void SuperMushroom::onCreateShapeDef(b2ShapeDef& def) {
    def.isSensor = true;
    def.density = 0.0f;

    def.filter.categoryBits = CollisionFilter::PICKUP;
    def.filter.maskBits = CollisionFilter::PLAYER;
}

void SuperMushroom::onUpdateVisuals(float deltaTime) {
    Item::onUpdateVisuals(deltaTime);
}

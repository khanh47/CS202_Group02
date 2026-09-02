#include "Game/Objects/Item/ConcreteItems/MegaCoin.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Player/Player.h"
#include "Physics/CollisionFilter.h"

MegaCoin::MegaCoin() : Item() {
}

MegaCoin::MegaCoin(sf::Texture& texture) : Item() {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "mega_coin");
    }
}

void MegaCoin::onPickup(Player& player) {
    // player.changeToFireState();
    destroy();
}

void MegaCoin::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_staticBody;
}

void MegaCoin::onCreateShapeDef(b2ShapeDef& def) {
    def.isSensor = true;
    def.density = 0.0f;

    def.filter.categoryBits = CollisionFilter::PICKUP;
    def.filter.maskBits = CollisionFilter::PLAYER;
}

void MegaCoin::onUpdateVisuals(float deltaTime) {
    Item::onUpdateVisuals(deltaTime);
}

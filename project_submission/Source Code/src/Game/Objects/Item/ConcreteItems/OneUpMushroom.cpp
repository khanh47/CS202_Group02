#include "Game/Objects/Item/ConcreteItems/OneUpMushroom.h"

#include "Audio/SoundManager.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Player/Player.h"
#include "Physics/CollisionFilter.h"

OneUpMushroom::OneUpMushroom() : Item() {
}

OneUpMushroom::OneUpMushroom(sf::Texture& texture) : Item() {
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "one_up_mushroom");
    }
}

void OneUpMushroom::onPickup(Player& player) {
    (void)player;
    Audio::SoundManager::getInstance().playEffect("one_up");
    destroy();
}

void OneUpMushroom::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_staticBody;
}

void OneUpMushroom::onCreateShapeDef(b2ShapeDef& def) {
    def.isSensor = true;
    def.density = 0.0f;

    def.filter.categoryBits = CollisionFilter::PICKUP;
    def.filter.maskBits = CollisionFilter::PLAYER;
}

void OneUpMushroom::onUpdateVisuals(float deltaTime) {
    Item::onUpdateVisuals(deltaTime);
}

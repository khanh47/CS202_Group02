#include "Game/World/WorldInteractionSystem.h"

#include <algorithm>

#include "Game/Objects/GameObject.h"
#include "Game/Objects/Player/Player.h"
#include "Physics/CollisionFilter.h"
#include "box2d/box2d.h"

namespace {
constexpr float minimumGroundNormalY = 0.9f;
constexpr float minimumSupportOverlapMeters = 1.0f / 64.0f;

Player* getPlayer(GameObject* objectA, GameObject* objectB) {
    if (auto* player = dynamic_cast<Player*>(objectA)) {
        return player;
    }
    return dynamic_cast<Player*>(objectB);
}

void processGroundContactBegin(
    const b2ContactBeginTouchEvent& event,
    GameObject* objectA,
    GameObject* objectB
) {
    Player* player = getPlayer(objectA, objectB);
    if (!player || !b2Contact_IsValid(event.contactId)) {
        return;
    }

    const b2ShapeId playerShape =
        player == objectA ? event.shapeIdA : event.shapeIdB;
    const b2ShapeId otherShape =
        player == objectA ? event.shapeIdB : event.shapeIdA;
    if ((b2Shape_GetFilter(otherShape).categoryBits
         & CollisionFilter::ENV) == 0) {
        return;
    }

    const b2ContactData contact = b2Contact_GetData(event.contactId);
    b2Vec2 playerToOther = contact.manifold.normal;
    if (!B2_ID_EQUALS(contact.shapeIdA, playerShape)) {
        playerToOther = {-playerToOther.x, -playerToOther.y};
    }

    const b2AABB playerBounds = b2Shape_GetAABB(playerShape);
    const b2AABB supportBounds = b2Shape_GetAABB(otherShape);
    const float horizontalOverlap =
        std::min(playerBounds.upperBound.x, supportBounds.upperBound.x)
        - std::max(playerBounds.lowerBound.x, supportBounds.lowerBound.x);

    if (contact.manifold.pointCount > 0
        && playerToOther.y >= minimumGroundNormalY
        && horizontalOverlap >= minimumSupportOverlapMeters) {
        player->beginGroundContact(otherShape);
    }
}

void processGroundContactEnd(
    const b2ContactEndTouchEvent& event,
    GameObject* objectA,
    GameObject* objectB
) {
    Player* player = getPlayer(objectA, objectB);
    if (!player) {
        return;
    }

    const b2ShapeId otherShape =
        player == objectA ? event.shapeIdB : event.shapeIdA;
    player->endGroundContact(otherShape);
}
}

void WorldInteractionSystem::processContacts(b2ContactEvents events) {
    for (int index = 0; index < events.beginCount; ++index) {
        const b2ContactBeginTouchEvent& event = events.beginEvents[index];
        if (!b2Shape_IsValid(event.shapeIdA)
            || !b2Shape_IsValid(event.shapeIdB)) {
            continue;
        }

        auto* objectA = static_cast<GameObject*>(
            b2Shape_GetUserData(event.shapeIdA)
        );
        auto* objectB = static_cast<GameObject*>(
            b2Shape_GetUserData(event.shapeIdB)
        );
        if (!objectA || !objectB) {
            continue;
        }

        processGroundContactBegin(event, objectA, objectB);

        if (b2Contact_IsValid(event.contactId)) {
            const b2ContactData contact = b2Contact_GetData(event.contactId);
            objectA->onContact(*objectB, contact, event.shapeIdA);
            objectB->onContact(*objectA, contact, event.shapeIdB);
        }
    }

    for (int index = 0; index < events.endCount; ++index) {
        const b2ContactEndTouchEvent& event = events.endEvents[index];
        if (!b2Shape_IsValid(event.shapeIdA)
            || !b2Shape_IsValid(event.shapeIdB)) {
            continue;
        }

        auto* objectA = static_cast<GameObject*>(
            b2Shape_GetUserData(event.shapeIdA)
        );
        auto* objectB = static_cast<GameObject*>(
            b2Shape_GetUserData(event.shapeIdB)
        );
        if (objectA && objectB) {
            processGroundContactEnd(event, objectA, objectB);
        }
    }
}

void WorldInteractionSystem::processSensors(b2SensorEvents events) {
    for (int i = 0; i < events.beginCount; ++i) {
        const b2SensorBeginTouchEvent& event = events.beginEvents[i];
        if (!b2Shape_IsValid(event.sensorShapeId)
            || !b2Shape_IsValid(event.visitorShapeId)) {
            continue;
        }

        auto* sensorObj = static_cast<GameObject*>(
            b2Shape_GetUserData(event.sensorShapeId)
        );
        auto* visitorObj = static_cast<GameObject*>(
            b2Shape_GetUserData(event.visitorShapeId)
        );
        if (!sensorObj || !visitorObj) {
            continue;
        }

        b2ContactData dummy = {};
        sensorObj->onContact(*visitorObj, dummy, b2_nullShapeId);
        visitorObj->onContact(*sensorObj, dummy, b2_nullShapeId);
    }
}

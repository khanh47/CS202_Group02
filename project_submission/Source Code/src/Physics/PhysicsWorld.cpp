#include "Physics/PhysicsWorld.h"
#include "Game/Objects/GameObject.h"
#include "Game/Objects/GameObjectFactory.h"
#include "Game/World/GameWorld.h"
#include "box2d/box2d.h"
#include "box2d/id.h"
#include "box2d/types.h"
#include <memory>

PhysicsWorld::PhysicsWorld() {
    b2WorldDef worldDef = b2DefaultWorldDef();

    worldDef.gravity = {0.0f, 9.8f};

    _worldId = b2CreateWorld(&worldDef);
}

PhysicsWorld::~PhysicsWorld() {
    if(isValid()) {
        b2DestroyWorld(_worldId);
        _worldId = b2_nullWorldId;
    }
}

void PhysicsWorld::updateSimulation(const float &fixedDt){
    b2World_Step(_worldId, fixedDt, subSteps);

}

b2ContactEvents PhysicsWorld::getContactEvents() {
    b2ContactEvents contactEvents = b2World_GetContactEvents(_worldId);
    return contactEvents;
}

b2SensorEvents PhysicsWorld::getSensorEvents() {
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(_worldId);
    return sensorEvents; 
}
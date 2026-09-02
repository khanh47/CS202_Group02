#pragma once

#include <box2d/box2d.h>

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld operator = (const PhysicsWorld&) = delete;

    // queries
    b2WorldId getId() const { return _worldId; }
    bool isValid() const { return b2World_IsValid(_worldId); }


    b2ContactEvents getContactEvents();
    b2SensorEvents getSensorEvents() ; 

    // update simulation
    void updateSimulation(const float &fixedDt);

private:
    static const int subSteps = 12;
    b2WorldId _worldId = b2_nullWorldId;
};
#pragma once

#include <box2d/box2d.h>

#include "Physics/PhysicsWorld.h"
#include "box2d/math_functions.h"

class PhysicsBody {
public:
    // constructors and destructors
    PhysicsBody() = default;
    PhysicsBody(const PhysicsWorld &physicsWorld, const b2BodyDef &bodyDef);
    
    PhysicsBody(const PhysicsBody&) = delete;
    PhysicsBody& operator = (const PhysicsBody&) = delete;
    PhysicsBody(PhysicsBody&& other);
    PhysicsBody& operator=(PhysicsBody&& other);

    ~PhysicsBody();

    // stuff
    void destroy();

    // queries
    bool isValid() const { return b2Body_IsValid(_bodyId); }
    b2BodyId getId() const { return _bodyId; }

    void setHibox(b2ShapeId shapeId);
    b2ShapeId getHitbox() const;
    b2Vec2 getHitboxSize();

    

private:
    b2WorldId _worldId = b2_nullWorldId;
    b2BodyId _bodyId = b2_nullBodyId;
    b2ShapeId _shapeId = b2_nullShapeId;
};
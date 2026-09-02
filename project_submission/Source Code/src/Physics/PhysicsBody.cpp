#include "Physics/PhysicsBody.h"
#include "box2d/id.h"

PhysicsBody::PhysicsBody(const PhysicsWorld &physicsWorld, const b2BodyDef &bodyDef): _worldId(physicsWorld.getId()) {
    _bodyId = b2CreateBody(physicsWorld.getId(), &bodyDef);
}

PhysicsBody::PhysicsBody(PhysicsBody&& other) {
    _worldId = other._worldId;
    _bodyId = other._bodyId;
    other._worldId = b2_nullWorldId;
    other._bodyId = b2_nullBodyId;
}

PhysicsBody& PhysicsBody::operator = (PhysicsBody&& other) {
    if (this != &other) {
        destroy();

        _worldId = other._worldId;
        _bodyId = other._bodyId;

        other._worldId = b2_nullWorldId;
        other._bodyId = b2_nullBodyId;
    }

    return *this;
}

PhysicsBody::~PhysicsBody() {
    destroy();
}

void PhysicsBody::destroy() {
    if(isValid()){
        b2DestroyBody(_bodyId);
        _bodyId = b2_nullBodyId;
    }
}

void PhysicsBody::setHibox(b2ShapeId shapeId) {
    _shapeId = shapeId;
}

b2ShapeId PhysicsBody::getHitbox() const {
    return _shapeId;
}

b2Vec2 PhysicsBody::getHitboxSize() {
    if (B2_IS_NULL(_shapeId) || !b2Shape_IsValid(_shapeId)) return {0, 0};

    b2Polygon polygon = b2Shape_GetPolygon(_shapeId);
    b2Vec2 minV = polygon.vertices[0];
    b2Vec2 maxV = polygon.vertices[0];

    for (int i = 1; i < polygon.count; ++i) {
        if (polygon.vertices[i].x < minV.x) minV.x = polygon.vertices[i].x;
        if (polygon.vertices[i].y < minV.y) minV.y = polygon.vertices[i].y;
        
        if (polygon.vertices[i].x > maxV.x) maxV.x = polygon.vertices[i].x;
        if (polygon.vertices[i].y > maxV.y) maxV.y = polygon.vertices[i].y;
    }

    float width = maxV.x - minV.x;
    float height = maxV.y - minV.y;

    return {width, height};
}
#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <box2d/box2d.h>

class GameObject;
class PhysicsWorld;

class TerrainSeamFilter {
public:
    void install(PhysicsWorld& physicsWorld);
    void clear();
    void addBlock(
        const std::shared_ptr<GameObject>& block,
        int column,
        int row,
        float leftPixels,
        float rightPixels
    );
    void addOccupiedCell(
        const std::shared_ptr<GameObject>& block,
        int column,
        int row
    );
    void setBoundaryColumns(int leftColumn, int rightColumn);
    bool isCellOccupied(int column, int row) const;

private:
    struct Cell {
        std::weak_ptr<GameObject> owner;
        int column;
        int row;
        float leftMeters;
        float rightMeters;
    };

    static bool preSolve(
        b2ShapeId shapeA,
        b2ShapeId shapeB,
        b2Pos point,
        b2Vec2 normal,
        void* context
    );

    bool shouldEnableContact(
        b2ShapeId shapeA,
        b2ShapeId shapeB,
        b2Pos point,
        b2Vec2 normal
    ) const;
    bool hasLiveBlock(int column, int row) const;
    static std::int64_t cellKey(int column, int row);

    std::unordered_map<const GameObject*, Cell> _blockCells;
    std::unordered_map<
        std::int64_t,
        std::weak_ptr<GameObject>
    > _occupancy;
    int _boundaryLeftColumn = 0;
    int _boundaryRightColumn = -1;
};

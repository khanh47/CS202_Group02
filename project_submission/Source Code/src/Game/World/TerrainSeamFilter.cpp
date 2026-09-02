#include "Game/World/TerrainSeamFilter.h"

#include <cmath>

#include "Game/Objects/GameObject.h"
#include "Physics/CollisionFilter.h"
#include "Physics/PhysicsUnits.h"
#include "Physics/PhysicsWorld.h"

namespace {
constexpr float seamToleranceMeters = 3.0f / PhysicsUnits::pixelsPerMeter;
constexpr float minimumLateralNormal = 0.05f;

bool belongsTo(uint64_t category, b2ShapeId shape) {
    return (b2Shape_GetFilter(shape).categoryBits & category) != 0;
}
}

void TerrainSeamFilter::install(PhysicsWorld& physicsWorld) {
    b2World_SetPreSolveCallback(
        physicsWorld.getId(),
        &TerrainSeamFilter::preSolve,
        this
    );
}

void TerrainSeamFilter::clear() {
    _blockCells.clear();
    _occupancy.clear();
    _boundaryLeftColumn = 0;
    _boundaryRightColumn = -1;
}

void TerrainSeamFilter::addBlock(
    const std::shared_ptr<GameObject>& block,
    int column,
    int row,
    float leftPixels,
    float rightPixels
) {
    if (!block) {
        return;
    }

    _blockCells.insert_or_assign(
        block.get(),
        Cell{
            block,
            column,
            row,
            PhysicsUnits::toMeters(leftPixels),
            PhysicsUnits::toMeters(rightPixels)
        }
    );
    _occupancy.insert_or_assign(cellKey(column, row), block);
}

void TerrainSeamFilter::addOccupiedCell(
    const std::shared_ptr<GameObject>& block,
    int column,
    int row
) {
    if (!block) {
        return;
    }

    _occupancy.insert_or_assign(cellKey(column, row), block);
}

bool TerrainSeamFilter::preSolve(
    b2ShapeId shapeA,
    b2ShapeId shapeB,
    b2Pos point,
    b2Vec2 normal,
    void* context
) {
    const auto* filter = static_cast<const TerrainSeamFilter*>(context);
    return !filter
        || filter->shouldEnableContact(
            shapeA,
            shapeB,
            point,
            normal
        );
}

bool TerrainSeamFilter::shouldEnableContact(
    b2ShapeId shapeA,
    b2ShapeId shapeB,
    b2Pos point,
    b2Vec2 normal
) const {
    const bool walkerIsA = belongsTo(
        CollisionFilter::PLAYER | CollisionFilter::SHELL | CollisionFilter::ENEMY,
        shapeA
    );
    const bool walkerIsB = belongsTo(
        CollisionFilter::PLAYER | CollisionFilter::SHELL | CollisionFilter::ENEMY,
        shapeB
    );
    if (walkerIsA == walkerIsB) {
        return true;
    }

    const b2ShapeId terrainShape = walkerIsA ? shapeB : shapeA;
    if (!belongsTo(CollisionFilter::ENV, terrainShape)) {
        return true;
    }

    const auto* terrainObject = static_cast<const GameObject*>(
        b2Shape_GetUserData(terrainShape)
    );
    const auto blockCell = _blockCells.find(terrainObject);
    if (blockCell == _blockCells.end()) {
        return true;
    }

    const Cell& cell = blockCell->second;
    const std::shared_ptr<GameObject> registeredBlock = cell.owner.lock();
    if (!registeredBlock || registeredBlock.get() != terrainObject) {
        return true;
    }

    const float pointX = static_cast<float>(point.x);
    const bool onInternalLeftEdge =
        std::abs(pointX - cell.leftMeters) <= seamToleranceMeters
        && hasLiveBlock(cell.column - 1, cell.row);
    const bool onInternalRightEdge =
        std::abs(pointX - cell.rightMeters) <= seamToleranceMeters
        && hasLiveBlock(cell.column + 1, cell.row);
    if (!onInternalLeftEdge && !onInternalRightEdge) {
        return true;
    }

    const b2Vec2 walkerToTerrain = walkerIsA
        ? normal
        : b2Vec2{-normal.x, -normal.y};

    // A vertical top/underside manifold is legitimate. A lateral or diagonal
    // manifold on a shared tile edge describes an edge that is inside the
    // union of the two adjacent blocks and must not reach the solver.
    return std::abs(walkerToTerrain.x) < minimumLateralNormal;
}

void TerrainSeamFilter::setBoundaryColumns(int leftColumn, int rightColumn) {
    _boundaryLeftColumn = leftColumn;
    _boundaryRightColumn = rightColumn;
}

bool TerrainSeamFilter::isCellOccupied(int column, int row) const {
    if (_boundaryRightColumn > _boundaryLeftColumn
        && (column <= _boundaryLeftColumn || column >= _boundaryRightColumn)) {
        return true;
    }
    return hasLiveBlock(column, row);
}

bool TerrainSeamFilter::hasLiveBlock(int column, int row) const {
    const auto occupant = _occupancy.find(cellKey(column, row));
    if (occupant == _occupancy.end()) {
        return false;
    }

    const std::shared_ptr<GameObject> block = occupant->second.lock();
    return block && !block->isPendingDestroy();
}

std::int64_t TerrainSeamFilter::cellKey(int column, int row) {
    return (static_cast<std::int64_t>(row) << 32)
        ^ static_cast<std::uint32_t>(column);
}

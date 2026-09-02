#include "Game/Objects/Pipe/Pipe.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "Physics/CollisionFilter.h"
#include "Physics/PhysicsUnits.h"
#include "box2d/box2d.h"

namespace {
/// Each spritesheet block is 16x16 pixels with 1px gaps between them.
constexpr int kSpriteTileSize = 16;
constexpr int kSpriteGap = 1;

/// World-space size of each rendered tile (matches one 64px cell).
/// A pipe is 2 tiles wide = 128px = 2 grid cells.
constexpr float kRenderTileSize = 64.0f;

void drawPipeDebugRect(
    sf::RenderTarget& target,
    const sf::Vector2f& centerPixels,
    const sf::Vector2f& sizePixels,
    float angleDegrees
) {
    if (sizePixels.x <= 0.0f || sizePixels.y <= 0.0f) {
        return;
    }

    sf::RectangleShape rect(sizePixels);
    rect.setOrigin({sizePixels.x * 0.5f, sizePixels.y * 0.5f});
    rect.setPosition(centerPixels);
    rect.setRotation(sf::degrees(angleDegrees));
    rect.setFillColor(sf::Color(255, 0, 255, 80));
    rect.setOutlineThickness(1.0f);
    rect.setOutlineColor(sf::Color::Magenta);
    target.draw(rect);
}
}

Pipe::Pipe() : GameObject() {}

Pipe::Pipe(sf::Texture& texture, Orientation orientation, EndSide endSide,
           int bodyLength, bool isWarp, int warpID, int warpTarget,
           std::string warpLevel)
    : GameObject(),
      _orientation(orientation),
      _endSide(endSide),
      _bodyLength(std::max(bodyLength, 0)),
      _isWarp(isWarp),
      _warpID(warpID),
      _warpTarget(warpTarget),
      _warpLevel(std::move(warpLevel)),
      _texture(&texture) {
    buildVertexArray(kRenderTileSize);
}

sf::Vector2f Pipe::computePipeSize(Orientation orientation, int bodyLength,
                                   float renderTileSize) {
    // Pipe is always 2 tiles in the cross-axis.
    // Along the main axis: 1 end-cap row + bodyLength body rows.
    const int mainAxisTiles = 1 + std::max(bodyLength, 0);
    if (orientation == Orientation::Vertical) {
        return {2.0f * renderTileSize, mainAxisTiles * renderTileSize};
    }
    return {mainAxisTiles * renderTileSize, 2.0f * renderTileSize};
}

void Pipe::spawn(
    const PhysicsWorld& physicsWorld,
    sf::Vector2f spawnPixels,
    sf::Vector2f hitboxPixels
) {
    GameObject::spawn(physicsWorld, spawnPixels, hitboxPixels);

    if (!hasValidBody() || _segments.empty()) {
        return;
    }

    // GameObject creates one full-pipe fallback shape. Replace it with one
    // shape per visible segment so Mega can remove only the contacted part.
    const b2ShapeId fullPipeShape = _body->getHitbox();
    if (b2Shape_IsValid(fullPipeShape)) {
        b2DestroyShape(fullPipeShape, true);
    }
    _body->setHibox(b2_nullShapeId);

    for (Segment& segment : _segments) {
        if (segment.quads.empty() || !segment.active) {
            continue;
        }

        const sf::Vector2f firstPosition = segment.quads.front().worldPosition;
        const sf::Vector2f lastPosition = segment.quads.back().worldPosition
            + segment.quads.back().worldSize;
        const sf::Vector2f segmentSize = lastPosition - firstPosition;
        const sf::Vector2f segmentCenter = firstPosition
            + segmentSize * 0.5f;

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.enableContactEvents = true;
        shapeDef.enableSensorEvents = true;
        shapeDef.userData = this;
        onCreateShapeDef(shapeDef);

        const b2Polygon polygon = b2MakeOffsetBox(
            PhysicsUnits::toMeters(segmentSize.x * 0.5f),
            PhysicsUnits::toMeters(segmentSize.y * 0.5f),
            PhysicsUnits::toMeters(segmentCenter),
            b2Rot_identity
        );
        segment.shape = b2CreatePolygonShape(
            _body->getId(),
            &shapeDef,
            &polygon
        );
    }

    refreshPrimaryShape();
}

void Pipe::breakSegment(b2ShapeId segmentShape) {
    if (!b2Shape_IsValid(segmentShape)) {
        return;
    }

    for (Segment& segment : _segments) {
        if (!B2_ID_EQUALS(segment.shape, segmentShape) || !segment.active) {
            continue;
        }

        const b2ShapeId shape = segment.shape;
        segment.shape = b2_nullShapeId;
        segment.active = false;

        // Contact events are still being consumed here. Queue the physical
        // destruction until the complete event batch has been processed so
        // Box2D's buffered event data remains valid.
        _pendingShapeDestruction.push_back(shape);

        rebuildVertexArray();
        refreshPrimaryShape();
        return;
    }
}

void Pipe::flushBrokenSegments() {
    for (const b2ShapeId shape : _pendingShapeDestruction) {
        if (b2Shape_IsValid(shape)) {
            b2DestroyShape(shape, true);
        }
    }

    _pendingShapeDestruction.clear();
    refreshPrimaryShape();
}

std::vector<int> Pipe::getBrokenSegmentIndices() const {
    std::vector<int> brokenSegments;
    brokenSegments.reserve(_segments.size());

    for (std::size_t index = 0; index < _segments.size(); ++index) {
        if (!_segments[index].active) {
            brokenSegments.push_back(static_cast<int>(index));
        }
    }

    return brokenSegments;
}

void Pipe::restoreBrokenSegments(const std::vector<int>& segmentIndices) {
    for (const int index : segmentIndices) {
        if (index < 0
            || index >= static_cast<int>(_segments.size())
            || !_segments[static_cast<std::size_t>(index)].active) {
            continue;
        }

        Segment& segment = _segments[static_cast<std::size_t>(index)];
        segment.active = false;
        if (b2Shape_IsValid(segment.shape)) {
            _pendingShapeDestruction.push_back(segment.shape);
        }
        segment.shape = b2_nullShapeId;
    }

    rebuildVertexArray();
    refreshPrimaryShape();
    flushBrokenSegments();
}

std::optional<Pipe::SegmentBreakData> Pipe::getSegmentBreakData(
    b2ShapeId segmentShape
) const {
    if (!b2Shape_IsValid(segmentShape)) {
        return std::nullopt;
    }

    for (const Segment& segment : _segments) {
        if (!segment.active
            || !B2_ID_EQUALS(segment.shape, segmentShape)
            || segment.quads.empty()) {
            continue;
        }

        const sf::Vector2f firstPosition = segment.quads.front().worldPosition;
        const sf::Vector2f lastPosition = segment.quads.back().worldPosition
            + segment.quads.back().worldSize;
        const sf::Vector2f segmentSize = lastPosition - firstPosition;
        const sf::Vector2f segmentCenter = firstPosition
            + segmentSize * 0.5f;

        int left = std::numeric_limits<int>::max();
        int top = std::numeric_limits<int>::max();
        int right = std::numeric_limits<int>::min();
        int bottom = std::numeric_limits<int>::min();
        for (const SegmentQuad& quad : segment.quads) {
            left = std::min(left, quad.textureRect.position.x);
            top = std::min(top, quad.textureRect.position.y);
            right = std::max(
                right,
                quad.textureRect.position.x + quad.textureRect.size.x
            );
            bottom = std::max(
                bottom,
                quad.textureRect.position.y + quad.textureRect.size.y
            );
        }

        return SegmentBreakData{
            getPosition() + segmentCenter,
            segmentSize,
            sf::IntRect({left, top}, {right - left, bottom - top})
        };
    }

    return std::nullopt;
}

void Pipe::onCreateBodyDef(b2BodyDef& def) {
    def.type = b2_staticBody;
}

void Pipe::onCreateShapeDef(b2ShapeDef& def) {
    def.density = 10000.0f;
    def.material.friction = 0.0f;
    def.filter.categoryBits = CollisionFilter::ENV;
    def.filter.maskBits = CollisionFilter::PLAYER | CollisionFilter::ENEMY |
                          CollisionFilter::FIREBALL | CollisionFilter::SHELL;
}

void Pipe::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position,
                          float angleDegrees) {
    if (!_texture || _vertices.getVertexCount() == 0) {
        return;
    }

    // The vertex array is built relative to the pipe's center (0,0).
    // Translate to the body's world position.
    sf::RenderStates states;
    states.texture = _texture;
    states.transform.translate(position);
    target.draw(_vertices, states);
}

void Pipe::onRenderDebugHitbox(sf::RenderTarget& target) const {
    if (!hasValidBody()) {
        return;
    }

    const sf::Vector2f bodyPosition = getBodyPositionPixels();
    const float angleDegrees = getBodyAngleDegrees();
    const float angleRadians = angleDegrees * (3.14159265f / 180.0f);
    const float cosAngle = std::cos(angleRadians);
    const float sinAngle = std::sin(angleRadians);

    for (const Segment& segment : _segments) {
        if (!segment.active
            || !b2Shape_IsValid(segment.shape)
            || segment.quads.empty()) {
            continue;
        }

        const sf::Vector2f firstPosition = segment.quads.front().worldPosition;
        const sf::Vector2f lastPosition = segment.quads.back().worldPosition
            + segment.quads.back().worldSize;
        const sf::Vector2f segmentSize = lastPosition - firstPosition;
        const sf::Vector2f localCenter = firstPosition + segmentSize * 0.5f;
        const sf::Vector2f rotatedCenter{
            cosAngle * localCenter.x - sinAngle * localCenter.y,
            sinAngle * localCenter.x + cosAngle * localCenter.y
        };

        drawPipeDebugRect(
            target,
            bodyPosition + rotatedCenter,
            segmentSize,
            angleDegrees
        );
    }
}

sf::IntRect Pipe::blockRect(int gridCol, int gridRow) {
    // gridCol and gridRow are 1-indexed.
    const int x = (gridCol - 1) * (kSpriteTileSize + kSpriteGap) + kSpriteGap;
    const int y = (gridRow - 1) * (kSpriteTileSize + kSpriteGap) + kSpriteGap;
    return {{x, y}, {kSpriteTileSize, kSpriteTileSize}};
}

void Pipe::appendQuad(sf::Vector2f worldPos, sf::Vector2f worldSize,
                      const sf::IntRect& texRect) {
    const float x0 = worldPos.x;
    const float y0 = worldPos.y;
    const float x1 = worldPos.x + worldSize.x;
    const float y1 = worldPos.y + worldSize.y;

    const auto u0 = static_cast<float>(texRect.position.x);
    const auto v0 = static_cast<float>(texRect.position.y);
    const auto u1 = static_cast<float>(texRect.position.x + texRect.size.x);
    const auto v1 = static_cast<float>(texRect.position.y + texRect.size.y);

    // Triangle 1
    _vertices.append({{x0, y0}, sf::Color::White, {u0, v0}});
    _vertices.append({{x1, y0}, sf::Color::White, {u1, v0}});
    _vertices.append({{x1, y1}, sf::Color::White, {u1, v1}});

    // Triangle 2
    _vertices.append({{x0, y0}, sf::Color::White, {u0, v0}});
    _vertices.append({{x1, y1}, sf::Color::White, {u1, v1}});
    _vertices.append({{x0, y1}, sf::Color::White, {u0, v1}});
}

void Pipe::buildVertexArray(float tileSize) {
    _segments.clear();

    const sf::Vector2f pipeSize = computePipeSize(_orientation, _bodyLength, tileSize);
    // Offset so the vertex array is centered at (0,0) — matching Box2D body center.
    const float halfW = pipeSize.x * 0.5f;
    const float halfH = pipeSize.y * 0.5f;

    if (_orientation == Orientation::Vertical) {
        // Vertical pipe: 2 tiles wide, (1 + _bodyLength) tiles tall.
        // EndSide::Top  => cap row first, then body rows going down.
        // EndSide::Bottom => body rows first, then cap row at the bottom.

        const int totalRows = 1 + _bodyLength;

        for (int row = 0; row < totalRows; ++row) {
            Segment& segment = _segments.emplace_back();
            for (int col = 0; col < 2; ++col) {
                const float worldX = -halfW + col * tileSize;
                const float worldY = -halfH + row * tileSize;

                sf::IntRect texRect;

                if (_endSide == EndSide::Top) {
                    if (row == 0) {
                        // Cap row: blocks (1,1) and (2,1)
                        texRect = blockRect(col + 1, 1);
                    } else {
                        // Body row: blocks (1,2) and (2,2)
                        texRect = blockRect(col + 1, 2);
                    }
                } else {
                    // EndSide::Bottom — reversed pipe
                    if (row == totalRows - 1) {
                        // Cap row at bottom: blocks (1,3) and (2,3)
                        texRect = blockRect(col + 1, 3);
                    } else {
                        // Body row: blocks (1,2) and (2,2)
                        texRect = blockRect(col + 1, 2);
                    }
                }

                segment.quads.push_back({
                    {worldX, worldY},
                    {tileSize, tileSize},
                    texRect
                });
            }
        }
    } else {
        // Horizontal pipe: 2 tiles tall, (1 + _bodyLength) tiles wide.
        // EndSide::Left  => cap column first, then body columns going right.
        // EndSide::Right => body columns first, then cap column at the right.

        const int totalCols = 1 + _bodyLength;

        for (int col = 0; col < totalCols; ++col) {
            Segment& segment = _segments.emplace_back();
            for (int row = 0; row < 2; ++row) {
                const float worldX = -halfW + col * tileSize;
                const float worldY = -halfH + row * tileSize;

                sf::IntRect texRect;

                if (_endSide == EndSide::Left) {
                    if (col == 0) {
                        // Left cap: blocks (3,1)/(3,2) for upper/lower
                        texRect = blockRect(3, row + 1);
                    } else {
                        // Body: blocks (4,1)/(4,2)
                        texRect = blockRect(4, row + 1);
                    }
                } else {
                    // EndSide::Right
                    if (col == totalCols - 1) {
                        // Right cap: blocks (5,1)/(5,2)
                        texRect = blockRect(5, row + 1);
                    } else {
                        // Body: blocks (4,1)/(4,2)
                        texRect = blockRect(4, row + 1);
                    }
                }

                segment.quads.push_back({
                    {worldX, worldY},
                    {tileSize, tileSize},
                    texRect
                });
            }
        }
    }

    rebuildVertexArray();
}

void Pipe::rebuildVertexArray() {
    _vertices.clear();

    for (const Segment& segment : _segments) {
        if (!segment.active) {
            continue;
        }

        for (const SegmentQuad& quad : segment.quads) {
            appendQuad(quad.worldPosition, quad.worldSize, quad.textureRect);
        }
    }
}

void Pipe::refreshPrimaryShape() {
    if (!_body || !_body->isValid()) {
        return;
    }

    for (const Segment& segment : _segments) {
        if (segment.active && b2Shape_IsValid(segment.shape)) {
            _body->setHibox(segment.shape);
            return;
        }
    }

    _body->setHibox(b2_nullShapeId);
}

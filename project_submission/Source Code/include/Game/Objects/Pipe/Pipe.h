#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include <vector>

#include "Game/Objects/GameObject.h"

/// Multi-tile pipe object assembled from a 16x16-block spritesheet.
/// Pipes are static environment objects that can be vertical or horizontal,
/// with a decorative end cap on one side and repeating body segments.
class Pipe : public GameObject {
public:
    enum class Orientation { Vertical, Horizontal };

    /// Which end has the decorative cap (opening).
    /// A vertical pipe with EndSide::Top has the cap at the top,
    /// body extends downward. One end only — never both caps.
    enum class EndSide {
        Top,
        Bottom,
        Left,
        Right
    };

    Pipe();
    Pipe(sf::Texture& texture, Orientation orientation, EndSide endSide,
         int bodyLength, bool isWarp, int warpID, int warpTarget,
         std::string warpLevel = "");
    ~Pipe() override = default;

    Orientation getOrientation() const { return _orientation; }
    EndSide getEndSide() const { return _endSide; }
    int getBodyLength() const { return _bodyLength; }
    bool isWarp() const { return _isWarp; }
    int getWarpID() const { return _warpID; }
    int getWarpTarget() const { return _warpTarget; }
    const std::string& getWarpLevel() const noexcept { return _warpLevel; }

    void spawn(
        const PhysicsWorld& physicsWorld,
        sf::Vector2f spawnPixels,
        sf::Vector2f hitboxPixels
    ) override;

    /// Break only the pipe segment represented by the contacted shape.
    void breakSegment(b2ShapeId segmentShape);

    /// Physically removes pipe segments queued during contact callbacks.
    void flushBrokenSegments();

    /// Returns the zero-based indices of segments already destroyed.
    std::vector<int> getBrokenSegmentIndices() const;

    /// Restores segment damage captured in a save state.
    void restoreBrokenSegments(const std::vector<int>& segmentIndices);

    struct SegmentBreakData {
        sf::Vector2f position;
        sf::Vector2f size;
        sf::IntRect textureRect;
    };

    std::optional<SegmentBreakData> getSegmentBreakData(
        b2ShapeId segmentShape
    ) const;
    const sf::Texture* getTexture() const noexcept { return _texture; }

    /// Computes the total pixel size of this pipe based on orientation and body length.
    /// Each tile is rendered at renderTileSize (default 32px), and the pipe is
    /// 2 tiles wide in the cross-axis direction.
    static sf::Vector2f computePipeSize(Orientation orientation, int bodyLength,
                                        float renderTileSize = 32.0f);

protected:
    void onCreateBodyDef(b2BodyDef& def) override;
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position,
                        float angleDegrees) override;
    void onRenderDebugHitbox(sf::RenderTarget& target) const override;

private:
    struct SegmentQuad {
        sf::Vector2f worldPosition;
        sf::Vector2f worldSize;
        sf::IntRect textureRect;
    };

    struct Segment {
        std::vector<SegmentQuad> quads;
        b2ShapeId shape = b2_nullShapeId;
        bool active = true;
    };

    /// Builds the vertex array from the spritesheet tile coordinates.
    void buildVertexArray(float renderTileSize);
    void rebuildVertexArray();
    void refreshPrimaryShape();

    /// Returns the texture rect for a spritesheet block at (gridCol, gridRow) (1-indexed).
    /// Each block is 16x16 with 1px gaps.
    static sf::IntRect blockRect(int gridCol, int gridRow);

    /// Appends 6 vertices (2 triangles) for one tile quad.
    void appendQuad(sf::Vector2f worldPos, sf::Vector2f worldSize,
                    const sf::IntRect& texRect);

    Orientation _orientation = Orientation::Vertical;
    EndSide _endSide = EndSide::Top;
    int _bodyLength = 1;
    bool _isWarp = false;
    int _warpID = -1;
    int _warpTarget = -1;
    std::string _warpLevel;
    sf::Texture* _texture = nullptr;
    sf::VertexArray _vertices{sf::PrimitiveType::Triangles};
    std::vector<Segment> _segments;
    std::vector<b2ShapeId> _pendingShapeDestruction;
};

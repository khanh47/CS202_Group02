#include "Game/Objects/Block/SlopeBlock.h"
#include "Game/Behaviours/Animatable.h"
#include "Physics/PhysicsUnits.h"

SlopeBlock::SlopeBlock() : Block() {}

SlopeBlock::SlopeBlock(sf::Texture& texture, SlopeType slopeType)
    : Block(texture), _slopeType(slopeType) {
    configureSlopeVisuals(texture, slopeType);
}

SlopeBlock::SlopeType SlopeBlock::parseSlopeType(const std::string& str) {
    if (str == "up_right_top" || str == "26") return SlopeType::UpRightTop;
    if (str == "down_right_top" || str == "27") return SlopeType::DownRightTop;
    if (str == "down_right_bottom" || str == "28") return SlopeType::DownRightBottom;
    return SlopeType::UpRightBottom;
}

void SlopeBlock::setSlopeType(SlopeType type) {
    _slopeType = type;
}

void SlopeBlock::configureSlopeVisuals(sf::Texture& texture, SlopeType slopeType) {
    _slopeType = slopeType;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture);
        sf::IntRect rect;
        switch (_slopeType) {
            case SlopeType::UpRightBottom:
                rect = sf::IntRect({1, 1}, {16, 16});     // g(0,0) - Upward slope lower grass cap (2:1 slope)
                break;
            case SlopeType::UpRightTop:
                rect = sf::IntRect({18, 1}, {16, 16});    // g(1,0) - Upward slope upper grass cap (2:1 slope)
                break;
            case SlopeType::DownRightTop:
                rect = sf::IntRect({18, 35}, {16, 16});   // g(1,2) - Downward slope upper grass cap (2:1 slope)
                break;
            case SlopeType::DownRightBottom:
                rect = sf::IntRect({35, 35}, {16, 16});   // g(2,2) - Downward slope lower grass cap (2:1 slope)
                break;
        }
        animatable->setTextureRect(rect);
    }
}

void SlopeBlock::onCreateShapeDef(b2ShapeDef& def) {
    Block::onCreateShapeDef(def);
    def.material.friction = 0.0f;
}

b2Polygon SlopeBlock::makeHitbox(sf::Vector2f hitboxPixels) const {
    const float hx = PhysicsUnits::toMeters(hitboxPixels.x * 0.5f);
    const float hy = PhysicsUnits::toMeters(hitboxPixels.y * 0.5f);

    b2Vec2 vertices[4];
    int count = 3;

    switch (_slopeType) {
        case SlopeType::UpRightBottom: {
            // Lower half of upward slope: rises from 0 (y = hy) at left to half-height (y = 0) at right
            vertices[0] = {-hx, hy};
            vertices[1] = {hx, hy};
            vertices[2] = {hx, 0.0f};
            count = 3;
            break;
        }
        case SlopeType::UpRightTop: {
            // Upper half of upward slope: rises from half-height (y = 0) at left to full height (y = -hy) at right
            vertices[0] = {-hx, 0.0f};
            vertices[1] = {-hx, hy};
            vertices[2] = {hx, hy};
            vertices[3] = {hx, -hy};
            count = 4;
            break;
        }
        case SlopeType::DownRightTop: {
            // Upper half of downward slope: drops from full height (y = -hy) at left to half-height (y = 0) at right
            vertices[0] = {-hx, -hy};
            vertices[1] = {-hx, hy};
            vertices[2] = {hx, hy};
            vertices[3] = {hx, 0.0f};
            count = 4;
            break;
        }
        case SlopeType::DownRightBottom: {
            // Lower half of downward slope: drops from half-height (y = 0) at left to 0 (y = hy) at right
            vertices[0] = {-hx, 0.0f};
            vertices[1] = {-hx, hy};
            vertices[2] = {hx, hy};
            count = 3;
            break;
        }
    }

    const b2Hull hull = b2ComputeHull(vertices, count);
    return b2MakePolygon(&hull, 0.0f);
}

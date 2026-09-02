#pragma once

#include "Game/Objects/Block/Block.h"
#include <string>

class SlopeBlock : public Block {
public:
    enum class SlopeType {
        UpRightBottom,
        UpRightTop,
        DownRightTop,
        DownRightBottom
    };

    SlopeBlock();
    SlopeBlock(sf::Texture& texture, SlopeType slopeType = SlopeType::UpRightBottom);
    ~SlopeBlock() override = default;

    static SlopeType parseSlopeType(const std::string& str);
    void setSlopeType(SlopeType type);
    void configureSlopeVisuals(sf::Texture& texture, SlopeType slopeType);

    bool isRenderedByTileMap() const noexcept override { return false; }

protected:
    b2Polygon makeHitbox(sf::Vector2f hitboxPixels) const override;
    void onCreateShapeDef(b2ShapeDef& def) override;

private:
    SlopeType _slopeType = SlopeType::UpRightBottom;
};

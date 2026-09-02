#include "Game/World/TileMap.h"
#include <cmath>
#include <algorithm>

#include "Animation/Animation.h"

TileMap::TileMap() = default;

void TileMap::initialize(int gridWidth, int gridHeight, float cellSize) {
    _gridWidth = gridWidth;
    _gridHeight = gridHeight;
    _cellSize = cellSize;

    const std::size_t cellCount = static_cast<std::size_t>(_gridWidth)
        * static_cast<std::size_t>(_gridHeight);
    _tiles.assign(cellCount, TileInfo{});
    _overlayTiles.assign(cellCount, TileInfo{});
    _batches.clear();
}

void TileMap::setTile(
    int col,
    int row,
    char tileCharacter,
    const sf::Texture* texture,
    sf::IntRect textureRect,
    TileAnimation animation,
    int rotationDeg
) {
    if (row >= 0 && row < _gridHeight && col >= 0 && col < _gridWidth) {
        const std::size_t index = static_cast<std::size_t>(row)
            * static_cast<std::size_t>(_gridWidth)
            + static_cast<std::size_t>(col);
        _tiles[index] = TileInfo{
            tileCharacter,
            texture,
            textureRect,
            animation,
            rotationDeg
        };
    }
}

void TileMap::setOverlayTile(
    int col,
    int row,
    char tileCharacter,
    const sf::Texture* texture,
    sf::IntRect textureRect,
    int rotationDeg
) {
    if (row >= 0 && row < _gridHeight && col >= 0 && col < _gridWidth) {
        const std::size_t index = static_cast<std::size_t>(row)
            * static_cast<std::size_t>(_gridWidth)
            + static_cast<std::size_t>(col);
        _overlayTiles[index] = TileInfo{tileCharacter, texture, textureRect, TileAnimation::None, rotationDeg};
    }
}

void TileMap::clear() {
    const std::size_t cellCount = static_cast<std::size_t>(_gridWidth)
        * static_cast<std::size_t>(_gridHeight);
    _tiles.assign(cellCount, TileInfo{});
    _overlayTiles.assign(cellCount, TileInfo{});
    _batches.clear();
}

void TileMap::update(float deltaTime) {
    Animation::advanceBrickAnimationClock(deltaTime);
}

void TileMap::updateVisibleVertices(const sf::View& view) {
    _batches.clear();
    if (_tiles.empty()
        || _gridWidth <= 0 || _gridHeight <= 0) {
        return;
    }

    // Determine the view frustum bounds in pixel coordinates
    const sf::Vector2f viewCenter = view.getCenter();
    const sf::Vector2f viewSize = view.getSize();
    const sf::FloatRect viewBounds(viewCenter - viewSize / 2.f, viewSize);

    // Padding margin around view frustum (2 cells) to avoid visual popping on edges
    const float margin = _cellSize * 2.f;

    // Tile culling index ranges clamped within map grid boundaries
    const int startCol = std::max(0, static_cast<int>(std::floor((viewBounds.position.x - margin) / _cellSize)));
    const int endCol = std::min(_gridWidth - 1, static_cast<int>(std::ceil((viewBounds.position.x + viewBounds.size.x + margin) / _cellSize)));
    const int startRow = std::max(0, static_cast<int>(std::floor((viewBounds.position.y - margin) / _cellSize)));
    const int endRow = std::min(_gridHeight - 1, static_cast<int>(std::ceil((viewBounds.position.y + viewBounds.size.y + margin) / _cellSize)));

    // Helper lambda to append quads for a tile vector
    const auto appendTiles = [&](const std::vector<TileInfo>& tileVector) {
        for (int r = startRow; r <= endRow; ++r) {
            for (int c = startCol; c <= endCol; ++c) {
                const std::size_t index = static_cast<std::size_t>(r)
                    * static_cast<std::size_t>(_gridWidth)
                    + static_cast<std::size_t>(c);
                const TileInfo& tile = tileVector[index];
                if (tile.character == '.' || tile.texture == nullptr) {
                    continue;
                }

                const sf::Texture* texture = tile.texture;
                sf::IntRect textureRect = tile.textureRect;
                if (tile.animation == TileAnimation::Brick) {
                    textureRect = Animation::getBrickAnimationFrameRect();
                } else if (textureRect.size.x <= 0 || textureRect.size.y <= 0) {
                    textureRect = sf::IntRect(
                        {0, 0},
                        {
                            static_cast<int>(texture->getSize().x),
                            static_cast<int>(texture->getSize().y)
                        }
                    );
                }

                const float tu0 = static_cast<float>(textureRect.position.x);
                const float tv0 = static_cast<float>(textureRect.position.y);
                const float tu1 = static_cast<float>(
                    textureRect.position.x + textureRect.size.x
                );
                const float tv1 = static_cast<float>(
                    textureRect.position.y + textureRect.size.y
                );

                sf::VertexArray* batch = nullptr;
                for (Batch& candidate : _batches) {
                    if (candidate.texture == texture) {
                        batch = &candidate.vertices;
                        break;
                    }
                }
                if (batch == nullptr) {
                    _batches.push_back(Batch{
                        texture,
                        sf::VertexArray(sf::PrimitiveType::Triangles)
                    });
                    batch = &_batches.back().vertices;
                }

                const float left = c * _cellSize;
                const float top = r * _cellSize;
                const float right = left + _cellSize;
                const float bottom = top + _cellSize;

                sf::Vector2f uvTL, uvTR, uvBR, uvBL;
                if (tile.rotationDeg == 90) { // 90 deg clockwise (e.g. Right Wall)
                    uvTL = {tu0, tv1}; // texture BL -> quad TL
                    uvTR = {tu0, tv0}; // texture TL -> quad TR
                    uvBR = {tu1, tv0}; // texture TR -> quad BR
                    uvBL = {tu1, tv1}; // texture BR -> quad BL
                } else if (tile.rotationDeg == 270) { // 270 deg clockwise / 90 deg CCW (e.g. Left Wall)
                    uvTL = {tu1, tv0}; // texture TR -> quad TL
                    uvTR = {tu1, tv1}; // texture BR -> quad TR
                    uvBR = {tu0, tv1}; // texture BL -> quad BR
                    uvBL = {tu0, tv0}; // texture TL -> quad BL
                } else if (tile.rotationDeg == 180) { // 180 deg (e.g. Underhang / Ceiling)
                    uvTL = {tu1, tv1};
                    uvTR = {tu0, tv1};
                    uvBR = {tu0, tv0};
                    uvBL = {tu1, tv0};
                } else { // 0 deg (Normal)
                    uvTL = {tu0, tv0};
                    uvTR = {tu1, tv0};
                    uvBR = {tu1, tv1};
                    uvBL = {tu0, tv1};
                }

                // Quad construction using 2 triangles (6 vertices)
                batch->append(sf::Vertex({left, top}, sf::Color::White, uvTL));
                batch->append(sf::Vertex({right, top}, sf::Color::White, uvTR));
                batch->append(sf::Vertex({right, bottom}, sf::Color::White, uvBR));

                batch->append(sf::Vertex({left, top}, sf::Color::White, uvTL));
                batch->append(sf::Vertex({right, bottom}, sf::Color::White, uvBR));
                batch->append(sf::Vertex({left, bottom}, sf::Color::White, uvBL));
            }
        }
    };

    // Pass 1: Base tiles (Back Layer)
    appendTiles(_tiles);

    // Pass 2: Overlay tiles (Outer / Front Layer)
    appendTiles(_overlayTiles);
}

char TileMap::getTile(int col, int row) const {
    if (row >= 0 && row < _gridHeight && col >= 0 && col < _gridWidth) {
        const std::size_t index = static_cast<std::size_t>(row)
            * static_cast<std::size_t>(_gridWidth)
            + static_cast<std::size_t>(col);
        return _tiles[index].character;
    }
    return '.';
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    for (const Batch& batch : _batches) {
        if (batch.texture == nullptr || batch.vertices.getVertexCount() == 0) {
            continue;
        }
        states.texture = batch.texture;
        target.draw(batch.vertices, states);
    }
}

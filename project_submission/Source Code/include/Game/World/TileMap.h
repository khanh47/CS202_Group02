#pragma once

#include <SFML/Graphics.hpp>
#include <cstddef>
#include <vector>

// TileMap is the rendering half of the tile layer. It owns a dense character
// grid and texture references, but it never creates game objects or physics
// bodies. WorldMap owns the separate collision representation.
class TileMap : public sf::Drawable {
public:
    enum class TileAnimation {
        None,
        Brick
    };

    TileMap();
    ~TileMap() override = default;

    // Resizes the internal tile grid storage
    void initialize(int gridWidth, int gridHeight, float cellSize);

    // Sets the texture for a specific grid cell (screen space col/row).
    void setTile(
        int col,
        int row,
        char tileCharacter,
        const sf::Texture* texture,
        sf::IntRect textureRect = {},
        TileAnimation animation = TileAnimation::None,
        int rotationDeg = 0
    );

    // Sets an overlay texture for a specific grid cell (rendered on front/outer layer).
    void setOverlayTile(
        int col,
        int row,
        char tileCharacter,
        const sf::Texture* texture,
        sf::IntRect textureRect = {},
        int rotationDeg = 0
    );

    // Clears all tile data
    void clear();

    // Advances animations used by the dense tile layer.
    void update(float deltaTime);

    // Rebuilds the internal vertex arrays containing only visible tiles within view bounds
    void updateVisibleVertices(const sf::View& view);

    // Returns the dense tile character at a specified grid cell.
    char getTile(int col, int row) const;

private:
    // Overridden from sf::Drawable to draw batched vertices grouped by texture
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    int _gridWidth = 0;
    int _gridHeight = 0;
    float _cellSize = 64.0f;

    struct TileInfo {
        char character = '.';
        const sf::Texture* texture = nullptr;
        sf::IntRect textureRect{};
        TileAnimation animation = TileAnimation::None;
        int rotationDeg = 0;
    };

    std::vector<TileInfo> _tiles;
    std::vector<TileInfo> _overlayTiles;

    struct Batch {
        const sf::Texture* texture = nullptr;
        sf::VertexArray vertices{sf::PrimitiveType::Triangles};
    };
    std::vector<Batch> _batches;

};

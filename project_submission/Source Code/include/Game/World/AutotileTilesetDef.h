#pragma once

#include <array>
#include <string>
#include <SFML/Graphics/Rect.hpp>
#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// AutotileTilesetDef
//
// Describes one autotile family using an 8-neighbor bitmask system (256 states).
//
// Bitmask bit encoding:
//   bit 0 (  1) = N  (top)
//   bit 1 (  2) = NE (top-right)    — only meaningful when N AND E are set
//   bit 2 (  4) = E  (right)
//   bit 3 (  8) = SE (bottom-right) — only meaningful when S AND E are set
//   bit 4 ( 16) = S  (bottom)
//   bit 5 ( 32) = SW (bottom-left)  — only meaningful when S AND W are set
//   bit 6 ( 64) = W  (left)
//   bit 7 (128) = NW (top-left)     — only meaningful when N AND W are set
//
// Diagonal masking reduces 256 raw states to 47 unique meaningful states.
// maskToRect[mask] → sf::IntRect for that tile variant.
//
// JSON layout field:
//   "single"       — whole texture for every mask (placeholder)
//   "row_major_4x4"— 4×4 grid for the 16 4-bit states (legacy)
//   "custom8"      — explicit 256-entry "rects" array  [[x,y,w,h], ...]
// ---------------------------------------------------------------------------
struct AutotileTilesetDef {
    std::string                  textureAlias;
    std::array<sf::IntRect, 256> maskToRect{};

    // Entire texture for every mask — placeholder / fallback
    static AutotileTilesetDef singleTile(
        const std::string& textureAlias,
        int textureWidth  = 64,
        int textureHeight = 64
    );

    // Legacy 4×4 row-major layout (16 entries, diagonals ignored)
    static AutotileTilesetDef rowMajor4x4(
        const std::string& textureAlias,
        int tileW = 64,
        int tileH = 64
    );

    // Parse from JSON object:
    //   "texture"    : alias string     (required)
    //   "layout"     : "single" | "row_major_4x4" | "custom8"
    //   "tileWidth"  : int              (single / row_major_4x4)
    //   "tileHeight" : int              (single / row_major_4x4)
    //   "rects"      : [[x,y,w,h],...]  256 entries  (custom8)
    static AutotileTilesetDef fromJson(const nlohmann::json& json);
};

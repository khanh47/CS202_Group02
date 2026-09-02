#include "Game/World/AutotileTilesetDef.h"

#include <stdexcept>
#include <string>

AutotileTilesetDef AutotileTilesetDef::singleTile(
    const std::string& textureAlias,
    int /*textureWidth*/,
    int /*textureHeight*/
) {
    AutotileTilesetDef def;
    def.textureAlias = textureAlias;
    // {0,0,0,0} → TileMap samples the full texture
    def.maskToRect.fill(sf::IntRect({0, 0}, {0, 0}));
    return def;
}

AutotileTilesetDef AutotileTilesetDef::rowMajor4x4(
    const std::string& textureAlias,
    int tileW,
    int tileH
) {
    AutotileTilesetDef def;
    def.textureAlias = textureAlias;
    // Fill all 256 slots with the 4-bit (lower 4 bits) entry, ignoring diagonals
    for (int mask = 0; mask < 256; ++mask) {
        const int ortho = mask & 0x55; // bits 0,2,4,6 = N,E,S,W
        // Remap to 4-bit index: N→bit0, E→bit1, S→bit2, W→bit3
        const int n = (ortho >> 0) & 1;
        const int e = (ortho >> 2) & 1;
        const int s = (ortho >> 4) & 1;
        const int w = (ortho >> 6) & 1;
        const int idx4 = n | (e << 1) | (s << 2) | (w << 3);
        const int col  = idx4 % 4;
        const int row  = idx4 / 4;
        def.maskToRect[mask] = sf::IntRect({col * tileW, row * tileH}, {tileW, tileH});
    }
    return def;
}

AutotileTilesetDef AutotileTilesetDef::fromJson(const nlohmann::json& json) {
    if (!json.is_object()) {
        throw std::runtime_error("AutotileTilesetDef: expected a JSON object");
    }
    if (!json.contains("texture")) {
        throw std::runtime_error(
            "AutotileTilesetDef: missing required field \"texture\""
        );
    }

    const std::string textureAlias = json["texture"].get<std::string>();
    const std::string layout       = json.value("layout", "single");
    const int         tileW        = json.value("tileWidth",  64);
    const int         tileH        = json.value("tileHeight", 64);

    if (layout == "row_major_4x4") {
        return rowMajor4x4(textureAlias, tileW, tileH);
    }

    // "custom8" — 256-entry explicit rect table
    if (layout == "custom8") {
        if (!json.contains("rects") || !json["rects"].is_array()
            || json["rects"].size() != 256) {
            throw std::runtime_error(
                "AutotileTilesetDef: layout 'custom8' requires a \"rects\" "
                "array with exactly 256 entries"
            );
        }
        AutotileTilesetDef def;
        def.textureAlias = textureAlias;
        const auto& rects = json["rects"];
        for (int i = 0; i < 256; ++i) {
            const auto& r = rects[i];
            if (!r.is_array() || r.size() != 4) {
                throw std::runtime_error(
                    "AutotileTilesetDef: each rect must be [x, y, w, h]"
                );
            }
            def.maskToRect[i] = sf::IntRect(
                {r[0].get<int>(), r[1].get<int>()},
                {r[2].get<int>(), r[3].get<int>()}
            );
        }
        return def;
    }

    // Legacy "custom" (16 entries) — expand to all 256 by ortho index
    if (layout == "custom") {
        if (!json.contains("rects") || !json["rects"].is_array()
            || json["rects"].size() != 16) {
            throw std::runtime_error(
                "AutotileTilesetDef: layout 'custom' requires a \"rects\" "
                "array with exactly 16 entries"
            );
        }
        AutotileTilesetDef def;
        def.textureAlias = textureAlias;
        const auto& rects = json["rects"];

        // Read the 16 base rects
        sf::IntRect base[16];
        for (int i = 0; i < 16; ++i) {
            const auto& r = rects[i];
            if (!r.is_array() || r.size() != 4)
                throw std::runtime_error("AutotileTilesetDef: each rect must be [x,y,w,h]");
            base[i] = sf::IntRect({r[0].get<int>(), r[1].get<int>()},
                                  {r[2].get<int>(), r[3].get<int>()});
        }

        // Expand to 256 by mapping via ortho bits (N=bit0,E=bit2,S=bit4,W=bit6)
        for (int mask = 0; mask < 256; ++mask) {
            const int n   = (mask >> 0) & 1;
            const int e   = (mask >> 2) & 1;
            const int s   = (mask >> 4) & 1;
            const int w   = (mask >> 6) & 1;
            const int idx = n | (e << 1) | (s << 2) | (w << 3);
            def.maskToRect[mask] = base[idx];
        }
        return def;
    }

    // Default: "single"
    return singleTile(textureAlias, tileW, tileH);
}

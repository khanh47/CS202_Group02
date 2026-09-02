#pragma once

#include <string>
#include <string_view>

#include "Game/World/SpawnSpec.h"

// The map theme selects the visual variant for block artwork. Keep this
// mapping in one place so level loading, respawning, and the map editor agree
// about which texture a block should use.
namespace ThemeAssets {

inline bool isUnderground(std::string_view theme) noexcept {
    return theme == "underground";
}

inline const char* brickTextureAlias(std::string_view theme) noexcept {
    return isUnderground(theme)
        ? "underground_brick_spritesheet"
        : "brick_spritesheet";
}

inline const char* luckyBlockTextureAlias(std::string_view theme) noexcept {
    return isUnderground(theme)
        ? "underground_lucky_block_spritesheet"
        : "lucky_block_spritesheet";
}

inline std::string luckyBlockTextureFor(
    std::string_view textureMode,
    std::string_view theme
) {
    if (textureMode == "invisible") {
        return {};
    }
    if (textureMode == "brick") {
        return brickTextureAlias(theme);
    }
    return luckyBlockTextureAlias(theme);
}

inline bool isBrickTextureAlias(std::string_view textureKey) noexcept {
    return textureKey == "brick"
        || textureKey == "brick_spritesheet"
        || textureKey == "underground_brick_spritesheet";
}

inline bool isLuckyBlockTextureAlias(std::string_view textureKey) noexcept {
    return textureKey == "lucky_block_spritesheet"
        || textureKey == "underground_lucky_block_spritesheet";
}

inline std::string textureAliasFor(
    const SpawnSpec& spec,
    std::string_view theme
) {
    if (spec.typeKey == "LuckyBlock") {
        return luckyBlockTextureFor(spec.luckyTexture, theme);
    }
    if (isLuckyBlockTextureAlias(spec.textureKey)) {
        return luckyBlockTextureAlias(theme);
    }

    // CoinBlock currently uses the same animated brick artwork that was
    // previously misnamed as a coin-block spritesheet. Keep the gameplay
    // object and its coin-emitting behaviour, while giving it the themed
    // animated block texture.
    if (spec.typeKey == "CoinBlock"
        || isBrickTextureAlias(spec.textureKey)) {
        return brickTextureAlias(theme);
    }

    return spec.textureKey;
}

} // namespace ThemeAssets

#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <SFML/Graphics/Rect.hpp>

#include "Game/World/AutotileTilesetDef.h"

// ---------------------------------------------------------------------------
// AutotileResult – returned by resolveDetailed()
// ---------------------------------------------------------------------------
struct AutotileResult {
    sf::IntRect texRect;
    bool        hasOverlay  = false;
    sf::IntRect overlayRect{};
    bool        isSlope     = false;
    int         slopeType   = 0;   // 25 = UpRight, 27 = DownRight
    int         rotationDeg = 0;   // 0, 90, 180, 270 (degrees clockwise)
};

// ---------------------------------------------------------------------------
// AutotileResolver
//
// Uses a Disjoint-Set Union (DSU / Union-Find) algorithm to group adjacent
// solid terrain tiles into connected components. Each cell's position WITHIN
// its component (top-edge, bottom-edge, left-edge, right-edge, and its
// horizontal index within its row) is stored so that:
//
//  1. The correct 4-neighbor bitmask variant is chosen (top / left-wall /
//     inner / right-wall / bottom / corner etc.).
//
//  2. Horizontal wave alternation is driven by the cell's position INSIDE
//     its component row (posInRow % 2) rather than its absolute column index,
//     so even short 2-tile platforms wave correctly.
//
// Bit encoding for maskToRect (matches AutotileTilesetDef):
//   bit 0 (1) = top    solid
//   bit 1 (2) = right  solid
//   bit 2 (4) = bottom solid
//   bit 3 (8) = left   solid
//
// Call precompute() once after building screenGrid / solidIds, then call
// resolve() / resolveDetailed() for each cell.
// ---------------------------------------------------------------------------
class AutotileResolver {
public:
    // -----------------------------------------------------------------------
    // precompute  –  build DSU component map from the grid
    //
    // Must be called before resolve() / resolveDetailed().  Internally runs a
    // two-pass Union-Find over all solid cells to compute per-cell info.
    // -----------------------------------------------------------------------
    void precompute(
        const std::vector<std::vector<int>>& screenGrid,
        int                                  gridWidth,
        int                                  gridHeight,
        const std::unordered_set<int>&       solidIds
    );

    // -----------------------------------------------------------------------
    // resolveDetailed  –  full result including automatic slope detection
    // -----------------------------------------------------------------------
    AutotileResult resolveDetailed(
        const std::vector<std::vector<int>>& screenGrid,
        int                                  col,
        int                                  screenRow,
        int                                  gridWidth,
        int                                  gridHeight,
        const std::unordered_set<int>&       solidIds,
        const AutotileTilesetDef&            def
    ) const;

    // -----------------------------------------------------------------------
    // resolve  –  texture sub-rect for a single cell (no slope detection)
    // -----------------------------------------------------------------------
    sf::IntRect resolve(
        const std::vector<std::vector<int>>& screenGrid,
        int                                  col,
        int                                  screenRow,
        int                                  gridWidth,
        int                                  gridHeight,
        const std::unordered_set<int>&       solidIds,
        const AutotileTilesetDef&            def
    ) const;

private:
    // -----------------------------------------------------------------------
    // Per-cell component info computed by precompute()
    // -----------------------------------------------------------------------
    struct CellInfo {
        bool isTopEdge    = true;  // no solid neighbor above
        bool isBottomEdge = true;  // no solid neighbor below
        bool isLeftEdge   = true;  // no solid neighbor to the left
        bool isRightEdge  = true;  // no solid neighbor to the right
        // 0-based horizontal index within the continuous solid run on this
        // row (resets at every gap), used for alternating wave variation.
        int  posInRow     = 0;
    };

    // Dense key: row * gridWidth + col
    static std::int64_t cellKey(int col, int row, int gridWidth) noexcept {
        return static_cast<std::int64_t>(row) * gridWidth + col;
    }

    bool isSolid(
        const std::vector<std::vector<int>>& screenGrid,
        int                                  col,
        int                                  row,
        int                                  gridWidth,
        int                                  gridHeight,
        const std::unordered_set<int>&       solidIds
    ) const noexcept;

    std::unordered_map<std::int64_t, CellInfo> _cellInfo;
    int _precomputedGridWidth = 0;
};

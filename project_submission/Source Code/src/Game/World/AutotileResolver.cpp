#include "Game/World/AutotileResolver.h"

#include <numeric>   // std::iota
#include <algorithm> // std::min / std::max

// ===========================================================================
// DSU (Disjoint-Set Union) – local helper, path-compressed + union-by-rank
// ===========================================================================
namespace {

struct DSU {
    std::vector<int> parent;
    std::vector<int> rank_;

    explicit DSU(int n) : parent(n), rank_(n, 0) {
        std::iota(parent.begin(), parent.end(), 0);
    }

    int find(int x) {
        while (parent[x] != x) {
            parent[x] = parent[parent[x]]; // path halving
            x = parent[x];
        }
        return x;
    }

    void unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) return;
        if (rank_[a] < rank_[b]) std::swap(a, b);
        parent[b] = a;
        if (rank_[a] == rank_[b]) ++rank_[a];
    }
};

} // anonymous namespace

// ===========================================================================
// isSolid  –  shared boundary-safe helper
// ===========================================================================
bool AutotileResolver::isSolid(
    const std::vector<std::vector<int>>& screenGrid,
    int col, int row,
    int gridWidth, int gridHeight,
    const std::unordered_set<int>& solidIds
) const noexcept {
    if (row >= gridHeight) return true;   // below the map = solid ground
    if (row < 0 || col < 0 || col >= gridWidth) return false;
    if (row >= static_cast<int>(screenGrid.size())) return false;
    const auto& rowVec = screenGrid[row];
    if (col >= static_cast<int>(rowVec.size())) return false;
    const int id = rowVec[col];
    return id != 0 && solidIds.count(id) > 0;
}

// ===========================================================================
// precompute  –  DSU component pass + per-cell info
// ===========================================================================
void AutotileResolver::precompute(
    const std::vector<std::vector<int>>& screenGrid,
    int gridWidth,
    int gridHeight,
    const std::unordered_set<int>& solidIds
) {
    _precomputedGridWidth = gridWidth;
    _cellInfo.clear();

    const int total = gridWidth * gridHeight;
    DSU dsu(total);

    // -----------------------------------------------------------------------
    // Pass 1 – union every solid cell with its right and bottom neighbours
    //   NOTE: isSolid() returns true for row >= gridHeight (virtual ground),
    //   but those cells have no DSU index, so we guard with r+1 < gridHeight.
    // -----------------------------------------------------------------------
    for (int r = 0; r < gridHeight; ++r) {
        for (int c = 0; c < gridWidth; ++c) {
            if (!isSolid(screenGrid, c, r, gridWidth, gridHeight, solidIds))
                continue;

            const int idx = r * gridWidth + c;

            // Right neighbour (c+1 < gridWidth guaranteed by isSolid check)
            if (c + 1 < gridWidth
                && isSolid(screenGrid, c + 1, r, gridWidth, gridHeight, solidIds))
            {
                dsu.unite(idx, r * gridWidth + (c + 1));
            }

            // Bottom neighbour — ONLY if within grid bounds
            if (r + 1 < gridHeight
                && isSolid(screenGrid, c, r + 1, gridWidth, gridHeight, solidIds))
            {
                dsu.unite(idx, (r + 1) * gridWidth + c);
            }
        }
    }

    // -----------------------------------------------------------------------
    // Pass 2 – fill CellInfo for every solid cell
    // -----------------------------------------------------------------------
    for (int r = 0; r < gridHeight; ++r) {
        int runStart = -1; // leftmost column of current solid horizontal run

        for (int c = 0; c <= gridWidth; ++c) {
            const bool solid = (c < gridWidth)
                && isSolid(screenGrid, c, r, gridWidth, gridHeight, solidIds);

            if (solid && runStart < 0) {
                runStart = c; // start of a new horizontal run
            }

            if (!solid && runStart >= 0) {
                // End of run: [runStart .. c-1] are all solid on this row.
                // They share the same component (we verified by union above
                // for horizontally adjacent cells).  Assign posInRow for each.
                for (int cc = runStart; cc < c; ++cc) {
                    CellInfo info;
                    info.isTopEdge    = !isSolid(screenGrid, cc, r - 1, gridWidth, gridHeight, solidIds);
                    info.isBottomEdge = !isSolid(screenGrid, cc, r + 1, gridWidth, gridHeight, solidIds);
                    info.isLeftEdge   = !isSolid(screenGrid, cc - 1, r, gridWidth, gridHeight, solidIds);
                    info.isRightEdge  = !isSolid(screenGrid, cc + 1, r, gridWidth, gridHeight, solidIds);
                    info.posInRow     = cc - runStart;   // 0 = leftmost in this run

                    _cellInfo[static_cast<std::int64_t>(r) * gridWidth + cc] = info;
                }
                runStart = -1;
            }
        }
    }
}

// ===========================================================================
// resolve  –  choose texture sub-rect using DSU component info
// ===========================================================================
sf::IntRect AutotileResolver::resolve(
    const std::vector<std::vector<int>>& screenGrid,
    int col, int screenRow,
    int gridWidth, int gridHeight,
    const std::unordered_set<int>& solidIds,
    const AutotileTilesetDef& def
) const {
    // -----------------------------------------------------------------------
    // 8-bit neighbor mask
    //   bit 0 (  1) = N    bit 1 (  2) = NE
    //   bit 2 (  4) = E    bit 3 (  8) = SE
    //   bit 4 ( 16) = S    bit 5 ( 32) = SW
    //   bit 6 ( 64) = W    bit 7 (128) = NW
    // -----------------------------------------------------------------------
    const bool N  = isSolid(screenGrid, col,     screenRow - 1, gridWidth, gridHeight, solidIds);
    const bool E  = isSolid(screenGrid, col + 1, screenRow,     gridWidth, gridHeight, solidIds);
    const bool S  = isSolid(screenGrid, col,     screenRow + 1, gridWidth, gridHeight, solidIds);
    const bool W  = isSolid(screenGrid, col - 1, screenRow,     gridWidth, gridHeight, solidIds);

    // NE/NW raw solidity (needed for top-edge connectivity rule below)
    const bool NE_solid = isSolid(screenGrid, col + 1, screenRow - 1, gridWidth, gridHeight, solidIds);
    const bool NW_solid = isSolid(screenGrid, col - 1, screenRow - 1, gridWidth, gridHeight, solidIds);

    // -----------------------------------------------------------------------
    // Top-edge connectivity rule:
    //
    // When this tile is on the TOP SURFACE of terrain (N = 0), its left/right
    // neighbours are only "connected" for grass-cap selection if they are ALSO
    // on the top surface (i.e., they have no solid tile above them).
    //
    // Without this rule, a taller adjacent column (which has solid above it)
    // would merge into the wide block's grass cap, producing a flat top-middle
    // tile instead of the expected corner tile with the grass overhang.
    //
    // Example: 3-wide block sits next to a taller thin column.  At the wide
    // block's top row the thin column is solid to the right BUT it has solid
    // above it (E's own N = NE_solid).  So maskE becomes 0, and the wide
    // block's right-top tile renders as top-right-corner (grass overhangs
    // right) rather than top-middle — exactly the NSMB look.
    // -----------------------------------------------------------------------
    const bool maskE = E;  // connect top surface flatly to adjacent solid
    const bool maskW = W;  // connect top surface flatly to adjacent solid

    // Diagonals are only meaningful when BOTH adjacent orthogonals are solid
    const bool NE = (N && maskE) && NE_solid;
    const bool SE = (S && maskE) && isSolid(screenGrid, col + 1, screenRow + 1, gridWidth, gridHeight, solidIds);
    const bool SW = (S && maskW) && isSolid(screenGrid, col - 1, screenRow + 1, gridWidth, gridHeight, solidIds);
    const bool NW = (N && maskW) && NW_solid;

    const int mask = (N     ?   1 : 0)
                   | (NE    ?   2 : 0)
                   | (maskE ?   4 : 0)
                   | (SE    ?   8 : 0)
                   | (S     ?  16 : 0)
                   | (SW    ?  32 : 0)
                   | (maskW ?  64 : 0)
                   | (NW    ? 128 : 0);

    sf::IntRect rect = def.maskToRect[mask];

    // Horizontal wave alternation for inner / top-mid / floor tiles
    int posInRow = col;
    const auto key = static_cast<std::int64_t>(screenRow) * _precomputedGridWidth + col;
    const auto it  = _cellInfo.find(key);
    if (it != _cellInfo.end()) {
        posInRow = it->second.posInRow;
    }

    if (def.textureAlias == "at_grassland") {
        if ((posInRow % 2) != 0
            && rect.size.x == 16
            && rect.position.x == 69)   // col 4: inner-A or top-mid-A
        {
            rect.position.x = 86;       // col 5: inner-B or top-mid-B
        }
    } else if (def.textureAlias == "at_underground") {
        const bool SE_solid = isSolid(screenGrid, col + 1, screenRow + 1, gridWidth, gridHeight, solidIds);
        const bool SW_solid = isSolid(screenGrid, col - 1, screenRow + 1, gridWidth, gridHeight, solidIds);

        // 1. Convex Corners
        if (!N && !W && S && E) {
            return sf::IntRect({1, 18}, {16, 16}); // Top-Left Convex Corner
        }
        if (!N && !E && S && W) {
            return sf::IntRect({86, 18}, {16, 16}); // Top-Right Convex Corner
        }
        if (!S && !W && N && E) {
            return sf::IntRect({1, 103}, {16, 16}); // Bottom-Left Corner
        }
        if (!S && !E && N && W) {
            return sf::IntRect({86, 103}, {16, 16}); // Bottom-Right Corner
        }

        // 2. Top Floor & Bottom/Ceiling Flat Surfaces
        // A one-tile-thick horizontal platform has neither N nor S, so it
        // needs an explicit surface rule instead of the interior-rock fallback.
        if (!N && !S && (W || E)) {
            const int waveIndex = posInRow % 4;
            return sf::IntRect({18 + waveIndex * 17, 18}, {16, 16});
        }
        if (!N && S && (W || E)) {
            const int waveIndex = posInRow % 4;
            return sf::IntRect({18 + waveIndex * 17, 18}, {16, 16}); // Top Floor Wave
        }
        if (!S && N && (W || E)) {
            const int waveIndex = posInRow % 4;
            return sf::IntRect({18 + waveIndex * 17, 103}, {16, 16}); // Ceiling Wave
        }

        // 3. Vertical Wall Bodies
        if (!W && E && (N || S)) {
            const int waveIndex = screenRow % 4;
            return sf::IntRect({1, 35 + waveIndex * 17}, {16, 16}); // Left Wall Body
        }
        if (!E && W && (N || S)) {
            const int waveIndex = screenRow % 4;
            return sf::IntRect({86, 35 + waveIndex * 17}, {16, 16}); // Right Wall Body
        }
        if (!W && !E && (N || S)) {
            const int waveIndex = screenRow % 4;
            return sf::IntRect({1, 35 + waveIndex * 17}, {16, 16}); // 1-wide vertical column
        }

        // 4. Interior Solid & Concave Inner Turns
        if (N && S && E && W) {
            if (!NE_solid && NW_solid && SE_solid && SW_solid) {
                return sf::IntRect({120, 18}, {16, 16}); // NE Concave Inner Turn
            }
            if (!NW_solid && NE_solid && SE_solid && SW_solid) {
                return sf::IntRect({103, 18}, {16, 16}); // NW Concave Inner Turn
            }
            return sf::IntRect({52, 52}, {16, 16}); // Pure Solid Rock Core
        }

        // Fallback for single isolated blocks
        if (!N && !S && !W && !E) {
            return sf::IntRect({18, 18}, {16, 16});
        }

        return sf::IntRect({52, 52}, {16, 16});
    }

    return rect;
}


// ===========================================================================
// resolveDetailed  –  slope detection + texture rect + front layer overlays
// ===========================================================================
AutotileResult AutotileResolver::resolveDetailed(
    const std::vector<std::vector<int>>& screenGrid,
    int col, int screenRow,
    int gridWidth, int gridHeight,
    const std::unordered_set<int>& solidIds,
    const AutotileTilesetDef& def
) const {
    AutotileResult result;

    // Base tile for the Back Layer
    result.texRect = resolve(
        screenGrid, col, screenRow,
        gridWidth, gridHeight,
        solidIds, def
    );

    // Underground theme has custom full-body resolution and no front overlays
    if (def.textureAlias == "at_underground") {
        result.rotationDeg = 0;
        return result;
    }

    // Front Layer overlays apply ONLY to grassland_terrain (at_grassland)
    if (def.textureAlias != "at_grassland") {
        return result;
    }

    // -----------------------------------------------------------------------
    // Outer/Front Layer Overlay: Expanded shorter column boundary
    //
    // 1. Top Surface Rows (!N): Render Top Corner Grass g(8,0) or g(3,0)
    // 2. Wall Body Rows (N):   Render Wall Tile g(8,1) or g(3,1)
    // -----------------------------------------------------------------------
    const bool N = isSolid(screenGrid, col, screenRow - 1, gridWidth, gridHeight, solidIds);
    const bool S = isSolid(screenGrid, col, screenRow + 1, gridWidth, gridHeight, solidIds);
    const bool W = isSolid(screenGrid, col - 1, screenRow, gridWidth, gridHeight, solidIds);
    const bool E = isSolid(screenGrid, col + 1, screenRow, gridWidth, gridHeight, solidIds);

    auto isSlopeTile = [&](int c, int r) -> bool {
        if (r < 0 || r >= gridHeight || c < 0 || c >= gridWidth) return false;
        const int id = screenGrid[r][c];
        return id == '1' || id == '2' || id == '3' || id == '4';
    };

    // 1. Expansion from LEFT shorter column into THIS taller column
    if (W) {
        int topLeftRow = -1;
        for (int r = screenRow; r >= 0; --r) {
            if (!isSolid(screenGrid, col - 1, r - 1, gridWidth, gridHeight, solidIds)) {
                if (isSolid(screenGrid, col - 1, r, gridWidth, gridHeight, solidIds)) {
                    topLeftRow = r;
                }
                break;
            }
        }

        if (topLeftRow != -1 && screenRow >= topLeftRow) {
            if (isSolid(screenGrid, col, topLeftRow - 1, gridWidth, gridHeight, solidIds)) {
                // Slope transitions do not create rectangular column overlays
                if (isSlopeTile(col, topLeftRow - 1) || isSlopeTile(col - 1, topLeftRow)
                    || isSlopeTile(col, screenRow) || isSlopeTile(col - 1, screenRow)
                    || isSlopeTile(col, screenRow - 1)) {
                    return result;
                }

                result.hasOverlay = true;
                if (!N || screenRow == topLeftRow) {
                    // Top Surface Row of Shorter Column Expansion: Top-Right Corner Grass g(8,0) [137, 1]
                    result.overlayRect = def.maskToRect[80];
                } else {
                    const bool S_left = isSolid(screenGrid, col - 1, screenRow + 1, gridWidth, gridHeight, solidIds);
                    if (!S_left && !S) {
                        // Bottom-Right Corner g(8,5) [137, 86]
                        result.overlayRect = def.maskToRect[65];
                    } else {
                        // Right Wall g(8,1) [137, 18]
                        result.overlayRect = def.maskToRect[243];
                    }
                }
                return result;
            }
        }
    }

    // 2. Expansion from RIGHT shorter column into THIS taller column
    if (E) {
        int topRightRow = -1;
        for (int r = screenRow; r >= 0; --r) {
            if (!isSolid(screenGrid, col + 1, r - 1, gridWidth, gridHeight, solidIds)) {
                if (isSolid(screenGrid, col + 1, r, gridWidth, gridHeight, solidIds)) {
                    topRightRow = r;
                }
                break;
            }
        }

        if (topRightRow != -1 && screenRow >= topRightRow) {
            if (isSolid(screenGrid, col, topRightRow - 1, gridWidth, gridHeight, solidIds)) {
                // Slope transitions do not create rectangular column overlays
                if (isSlopeTile(col, topRightRow - 1) || isSlopeTile(col + 1, topRightRow)
                    || isSlopeTile(col, screenRow) || isSlopeTile(col + 1, screenRow)
                    || isSlopeTile(col, screenRow - 1)) {
                    return result;
                }

                result.hasOverlay = true;
                if (!N || screenRow == topRightRow) {
                    // Top Surface Row of Shorter Column Expansion: Top-Left Corner Grass g(3,0) [52, 1]
                    result.overlayRect = def.maskToRect[20];
                } else {
                    const bool S_right = isSolid(screenGrid, col + 1, screenRow + 1, gridWidth, gridHeight, solidIds);
                    if (!S_right && !S) {
                        // Bottom-Left Corner g(3,5) [52, 86]
                        result.overlayRect = def.maskToRect[5];
                    } else {
                        // Left Wall g(3,1) [52, 18]
                        result.overlayRect = def.maskToRect[31];
                    }
                }
                return result;
            }
        }
    }

    return result;
}

#include "Game/World/LevelDataLoader.h"

#include <fstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

#include <nlohmann/json.hpp>

namespace {
void loadPrefabDefinitions(
    const nlohmann::json& document,
    PrefabRegistry& registry
) {
    if (!document.is_object() || !document.contains("prefabs")) {
        throw std::runtime_error(
            "Prefab document must contain a prefabs object or array"
        );
    }

    const nlohmann::json& prefabs = document["prefabs"];
    if (prefabs.is_object()) {
        for (const auto& [prefabId, definition] : prefabs.items()) {
            registry.registerPrefab(prefabId, definition);
        }
        return;
    }

    if (prefabs.is_array()) {
        for (const auto& entry : prefabs) {
            if (!entry.is_object() || !entry.contains("id")
                || !entry["id"].is_string()) {
                throw std::runtime_error(
                    "Every prefab array entry must contain a string id"
                );
            }
            const std::string prefabId = entry["id"].get<std::string>();
            nlohmann::json definition = entry;
            definition.erase("id");
            registry.registerPrefab(prefabId, definition);
        }
        return;
    }

    throw std::runtime_error("prefabs must be a JSON object or array");
}

nlohmann::json readJsonFile(const std::filesystem::path& filePath) {
    std::ifstream input(filePath);
    if (!input) {
        throw std::runtime_error(
            "Unable to open JSON file: " + filePath.string()
        );
    }

    nlohmann::json document;
    try {
        input >> document;
    } catch (const nlohmann::json::exception& error) {
        throw std::runtime_error(
            "Malformed JSON in '" + filePath.string() + "': "
            + std::string(error.what())
        );
    }
    return document;
}

void parseTileMapping(
    const nlohmann::json& json,
    LevelData& levelData
) {
    if (!json.is_object() || json.empty()) {
        throw std::runtime_error("tileMapping must be a non-empty object");
    }

    for (const auto& [tileCharacter, prefabId] : json.items()) {
        if (tileCharacter.size() != 1
            || static_cast<unsigned char>(tileCharacter.front()) > 0x7F
            || !prefabId.is_string()) {
            throw std::runtime_error(
                "Every tileMapping entry must map one ASCII character to a prefab id"
            );
        }
        levelData.tileMapping.insert_or_assign(
            tileCharacter.front(),
            prefabId.get<std::string>()
        );
    }
}

void parseLayer(
    const nlohmann::json& json,
    int maximumWidth,
    int maximumHeight,
    LevelData& levelData
) {
    if (!json.is_array() || json.empty()
        || json.size() > static_cast<std::size_t>(maximumHeight)) {
        throw std::runtime_error(
            "layer must be a non-empty array within the configured height"
        );
    }

    std::size_t expectedWidth = 0;
    for (const auto& rowJson : json) {
        if (!rowJson.is_string()) {
            throw std::runtime_error("Every layer row must be a string");
        }

        const std::string row = rowJson.get<std::string>();
        if (row.empty() || row.size() > static_cast<std::size_t>(maximumWidth)) {
            throw std::runtime_error(
                "Every layer row must be a dense string within the configured width"
                " (row width=" + std::to_string(row.size())
                + ", maximum=" + std::to_string(maximumWidth) + ")"
            );
        }
        if (expectedWidth == 0) {
            expectedWidth = row.size();
        } else if (row.size() != expectedWidth) {
            throw std::runtime_error(
                "layer must be rectangular; all rows need the same width"
            );
        }

        for (const char tileCharacter : row) {
            if (levelData.tileMapping.find(tileCharacter)
                == levelData.tileMapping.end()) {
                throw std::runtime_error(
                    std::string("layer contains unmapped character: '")
                    + tileCharacter + "'"
                );
            }
        }
        levelData.layer.push_back(row);
    }
}

void validatePrefabReferences(const LevelData& levelData) {
    for (const auto& [mapCharacter, prefabId] : levelData.tileMapping) {
        (void)mapCharacter;
        // A single mapping is intentionally allowed to resolve to either a
        // tile or an object. The dense layer determines every instance's
        // position, while the prefab determines how that cell behaves.
        (void)levelData.prefabs.resolve(prefabId);
    }
}

void parsePlacements(
    const nlohmann::json& json,
    int maximumWidth,
    int maximumHeight,
    LevelData& levelData
) {
    if (!json.is_array()) {
        throw std::runtime_error("placements must be an array");
    }

    std::unordered_set<int> occupiedCells;
    for (const auto& placementJson : json) {
        if (!placementJson.is_object()
            || !placementJson.contains("column")
            || !placementJson.contains("row")
            || !placementJson.contains("spec")
            || !placementJson["column"].is_number_integer()
            || !placementJson["row"].is_number_integer()
            || !placementJson["spec"].is_object()) {
            throw std::runtime_error(
                "Every placement must contain integer column/row and an object spec"
            );
        }

        const int column = placementJson["column"].get<int>();
        const int row = placementJson["row"].get<int>();
        if (column < 0 || column >= maximumWidth
            || row < 0 || row >= maximumHeight) {
            throw std::runtime_error("Placement coordinate is outside the level limits");
        }

        const int cellKey = row * maximumWidth + column;
        if (!occupiedCells.insert(cellKey).second) {
            throw std::runtime_error("Multiple placements cannot share a cell");
        }

        LevelData::Placement placement;
        placement.column = column;
        placement.row = row;
        placement.spec = placementJson["spec"].get<SpawnSpec>();
        if (!placement.spec.objectKind) {
            throw std::runtime_error("Placement spec must describe a live object");
        }
        levelData.placements.push_back(std::move(placement));
    }
}
}

LevelData LevelDataLoader::load(
    const std::filesystem::path& filePath,
    int maximumWidth,
    int maximumHeight,
    const std::filesystem::path& sharedPrefabsPath
) {
    if (maximumWidth <= 0 || maximumHeight <= 0) {
        throw std::invalid_argument("Level limits must be positive");
    }

    const nlohmann::json document = readJsonFile(filePath);

    if (!document.is_object()) {
        throw std::runtime_error("Level JSON must be a JSON object");
    }

    LevelData levelData;
    try {
        loadPrefabDefinitions(readJsonFile(sharedPrefabsPath), levelData.prefabs);
        if (document.contains("prefabs")) {
            loadPrefabDefinitions(document, levelData.prefabs);
        }

        if (!document.contains("tileMapping")
            || !document.contains("layer")) {
            throw std::runtime_error(
                "Level JSON must contain tileMapping and layer"
            );
        }

        parseTileMapping(document["tileMapping"], levelData);
        parseLayer(
            document["layer"],
            maximumWidth,
            maximumHeight,
            levelData
        );
        validatePrefabReferences(levelData);
        if (document.contains("placements")) {
            parsePlacements(
                document["placements"],
                maximumWidth,
                maximumHeight,
                levelData
            );
        }
    } catch (const std::exception& error) {
        throw std::runtime_error(
            "Invalid level data: " + std::string(error.what())
        );
    }
    levelData.theme = document.value("theme", "");
    levelData.background = document.value("background", "");
    levelData.music = document.value("music", "");

    if (levelData.theme.empty()) {
        if (levelData.background == "parallax_underground"
            || levelData.music == "underground_theme") {
            levelData.theme = "underground";
        } else if (levelData.background == "parallax_sky"
                   || levelData.music == "ground_theme") {
            levelData.theme = "sky";
        }
    }

    if (levelData.theme == "sky") {
        if (levelData.background.empty()) {
            levelData.background = "parallax_sky";
        }
        if (levelData.music.empty()) {
            levelData.music = "ground_theme";
        }
    } else if (levelData.theme == "underground") {
        if (levelData.background.empty()) {
            levelData.background = "parallax_underground";
        }
        if (levelData.music.empty()) {
            levelData.music = "underground_theme";
        }
    }

    return levelData;
}

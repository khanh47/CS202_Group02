#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include "Game/World/PrefabRegistry.h"
#include "Game/World/SpawnSpec.h"

struct LevelData {
    // Every cell uses one character. The character can resolve to either a
    // drawable/solid tile prefab or a live object prefab.
    std::unordered_map<char, std::string> tileMapping;
    std::vector<std::string> layer;
    PrefabRegistry prefabs;
    struct Placement {
        int column = 0;
        int row = 0;
        SpawnSpec spec;
    };
    std::vector<Placement> placements;
    std::string theme;
    std::string background;
    std::string music;
};

class LevelDataLoader {
public:
    static LevelData load(
        const std::filesystem::path& filePath,
        int maximumWidth = 500,
        int maximumHeight = 60,
        const std::filesystem::path& sharedPrefabsPath =
            "assets/datas/prefabs.json"
    );
};

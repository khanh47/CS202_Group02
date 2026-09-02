#pragma once

#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "Game/World/SpawnSpec.h"

// A prefab is a named, reusable description of an object or tile. Levels
// store only prefab references. A level can add local definitions when a
// prefab needs map-specific parameters.
class PrefabRegistry {
public:
    void registerPrefab(
        const std::string& prefabId,
        const nlohmann::json& definition
    );

    SpawnSpec resolve(
        const std::string& prefabId
    ) const;

private:
    SpawnSpec resolveInternal(
        const std::string& prefabId,
        std::unordered_map<std::string, bool>& resolving
    ) const;

    std::unordered_map<std::string, nlohmann::json> _definitions;
};

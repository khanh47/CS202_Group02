#include "Game/World/PrefabRegistry.h"

#include <stdexcept>

void PrefabRegistry::registerPrefab(
    const std::string& prefabId,
    const nlohmann::json& definition
) {
    if (prefabId.empty()) {
        throw std::runtime_error("Prefab id must not be empty");
    }
    if (!definition.is_object()) {
        throw std::runtime_error(
            "Prefab '" + prefabId + "' must be a JSON object"
        );
    }

    // Validate the definition when it enters the registry. This keeps bad
    // prefabs from failing later during a level rebuild.
    (void)definition.get<SpawnSpec>();
    _definitions.insert_or_assign(prefabId, definition);
}

SpawnSpec PrefabRegistry::resolve(
    const std::string& prefabId
) const {
    std::unordered_map<std::string, bool> resolving;
    return resolveInternal(prefabId, resolving);
}

SpawnSpec PrefabRegistry::resolveInternal(
    const std::string& prefabId,
    std::unordered_map<std::string, bool>& resolving
) const {
    const auto it = _definitions.find(prefabId);
    if (it == _definitions.end()) {
        throw std::runtime_error("Unknown prefab: " + prefabId);
    }
    if (resolving[prefabId]) {
        throw std::runtime_error(
            "Prefab contents cycle detected at: " + prefabId
        );
    }

    resolving[prefabId] = true;

    SpawnSpec result;
    try {
        result = it->second.get<SpawnSpec>();
        const auto contentsIt = it->second.find("contents");
        if (contentsIt != it->second.end() && contentsIt->is_string()) {
            result.contents = std::make_shared<SpawnSpec>(
                resolveInternal(contentsIt->get<std::string>(), resolving)
            );
        }
    } catch (const nlohmann::json::exception& error) {
        resolving[prefabId] = false;
        throw std::runtime_error(
            "Invalid prefab '" + prefabId + "': " + error.what()
        );
    }
    resolving[prefabId] = false;
    return result;
}

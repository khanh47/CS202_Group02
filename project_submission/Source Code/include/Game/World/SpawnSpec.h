#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <SFML/System/Vector2.hpp>
#include <nlohmann/json.hpp>

enum class ObjectKind {
    Block,
    Player,
    Enemy,
    Item,
    Pipe
};

struct LuckyOptionSpec {
    std::string itemTypeKey;
    float weight = 1.0f;
};

struct SpawnSpec {
    // A missing object kind identifies a static tile prefab. Object prefabs
    // carry a kind so the spawner can select the matching factory.
    std::optional<ObjectKind> objectKind;
    std::string typeKey;
    std::string animationId;
    std::string textureKey;
    sf::Vector2f size{64.0f, 64.0f};
    sf::Vector2f offset;
    bool centerVertically = false;
    bool addSeamFilter = false;
    bool addController = false;
    bool solid = false;
    bool breakable = false;
    std::string autotileId;
    std::string slopeType;
    std::shared_ptr<SpawnSpec> contents;
    std::vector<LuckyOptionSpec> luckyOptions;
    int coinCapacity = 10;
    std::string luckyTexture = "default";
    int luckyCapacity = 1;

    // Pipe-specific fields
    std::string pipeOrientation;
    std::string pipeEndSide;
    int pipeBodyLength = 1;
    bool pipeIsWarp = false;
    int warpID = -1;
    int warpTarget = -1;
    std::string warpLevel;
    bool contentsStatic = false;

    // Optional motion configuration for Piranha Plant pipe contents.
    std::string piranhaMotion = "timed";
    float piranhaWavePeriod = 6.0f;
    float piranhaWavePhase = 0.0f;
};

void from_json(const nlohmann::json& json, SpawnSpec& spec);

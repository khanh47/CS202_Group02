#include "Game/World/SpawnSpec.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
ObjectKind parseKind(const nlohmann::json& json) {
    std::string kind = json.get<std::string>();
    std::transform(
        kind.begin(),
        kind.end(),
        kind.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        }
    );

    if (kind == "block") {
        return ObjectKind::Block;
    }
    if (kind == "player") {
        return ObjectKind::Player;
    }
    if (kind == "enemy") {
        return ObjectKind::Enemy;
    }
    if (kind == "item") {
        return ObjectKind::Item;
    }
    if (kind == "pipe") {
        return ObjectKind::Pipe;
    }
    throw std::runtime_error("Unknown spawn kind: " + kind);
}

sf::Vector2f parseVector2(
    const nlohmann::json& json,
    const char* fieldName
) {
    if (!json.is_array() || json.size() != 2
        || !json[0].is_number() || !json[1].is_number()) {
        throw std::runtime_error(
            std::string(fieldName) + " must be a [x, y] pair"
        );
    }
    return {json[0].get<float>(), json[1].get<float>()};
}
}

void from_json(const nlohmann::json& json, SpawnSpec& spec) {
    if (!json.is_object()) {
        throw std::runtime_error("Spawn spec must be a JSON object");
    }

    spec = SpawnSpec{};
    if (json.contains("kind")) {
        spec.objectKind = parseKind(json["kind"]);
    }
    if (spec.objectKind && !json.contains("typeKey")) {
        throw std::runtime_error("Spawn spec is missing required field: typeKey");
    }
    if (spec.objectKind && !json.contains("texture")) {
        throw std::runtime_error("Spawn spec is missing required field: texture");
    }
    if (json.contains("size")) {
        spec.size = parseVector2(json["size"], "Spawn size");
    }

    spec.typeKey = json.value("typeKey", "");
    spec.textureKey = json.value("texture", "");

    spec.animationId = json.value("animationId", "");
    if (json.contains("offset")) {
        spec.offset = parseVector2(json["offset"], "Spawn offset");
    }
    spec.centerVertically = json.value("centerVertically", false);
    spec.addSeamFilter = json.value("addSeamFilter", false);
    spec.addController = json.value("addController", false);
    spec.solid = json.value(
        "solid",
        !spec.objectKind.has_value()
    );
    spec.breakable = json.value("breakable", false);
    spec.autotileId = json.value("autotile", "");
    spec.slopeType = json.value("slopeType", "");
    spec.coinCapacity = json.value("coinCapacity", 10);
    spec.luckyTexture = json.value("luckyTexture", "default");
    spec.luckyCapacity = json.value("luckyCapacity", 1);
    if (json.contains("luckyOptions")) {
        if (!json["luckyOptions"].is_array()) {
            throw std::runtime_error("luckyOptions must be an array");
        }
        for (const auto& optionJson : json["luckyOptions"]) {
            if (!optionJson.is_object()
                || !optionJson.contains("item")
                || !optionJson["item"].is_string()) {
                throw std::runtime_error(
                    "Every luckyOptions entry must contain an item string"
                );
            }
            LuckyOptionSpec option;
            option.itemTypeKey = optionJson["item"].get<std::string>();
            option.weight = optionJson.value("weight", 1.0f);
            if (option.itemTypeKey.empty() || option.weight <= 0.0f) {
                throw std::runtime_error(
                    "Lucky block options need a non-empty item and positive weight"
                );
            }
            spec.luckyOptions.push_back(std::move(option));
        }
    }
    if (spec.coinCapacity <= 0) {
        throw std::runtime_error("coinCapacity must be positive");
    }
    if (spec.luckyTexture != "default"
        && spec.luckyTexture != "invisible"
        && spec.luckyTexture != "brick") {
        throw std::runtime_error(
            "luckyTexture must be default, invisible, or brick"
        );
    }
    if (spec.luckyCapacity <= 0) {
        throw std::runtime_error("luckyCapacity must be positive");
    }

    // Pipe-specific fields
    spec.pipeOrientation = json.value("pipeOrientation", "");
    spec.pipeEndSide = json.value("pipeEndSide", "");
    spec.pipeBodyLength = json.value("pipeBodyLength", 1);
    spec.pipeIsWarp = json.value("pipeIsWarp", false);
    spec.warpID = json.value("warpID", -1);
    spec.warpTarget = json.value("warpTarget", -1);
    spec.warpLevel = json.value("warpLevel", "");
    spec.contentsStatic = json.value("contentsStatic", false);
    spec.piranhaMotion = json.value("piranhaMotion", "timed");
    spec.piranhaWavePeriod = json.value("piranhaWavePeriod", 6.0f);
    spec.piranhaWavePhase = json.value("piranhaWavePhase", 0.0f);
    if (spec.piranhaMotion != "timed" && spec.piranhaMotion != "sine") {
        throw std::runtime_error(
            "piranhaMotion must be timed or sine"
        );
    }
    if (spec.piranhaWavePeriod <= 0.0f) {
        throw std::runtime_error("piranhaWavePeriod must be positive");
    }

    if (json.contains("contents")) {
        if (json["contents"].is_null()) {
            spec.contents.reset();
        } else if (json["contents"].is_string()) {
            // A string is a prefab reference. PrefabRegistry resolves it
            // after this value has been parsed, so inline contents and
            // reusable contents use the same SpawnSpec representation.
            spec.contents.reset();
        } else {
            spec.contents = std::make_shared<SpawnSpec>(
                json["contents"].get<SpawnSpec>()
            );
        }
    }
}

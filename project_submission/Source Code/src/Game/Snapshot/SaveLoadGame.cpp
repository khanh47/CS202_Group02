#include "Game/Snapshot/SaveLoadGame.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
constexpr int saveFormatVersion = 1;

std::string defaultSlotName(int index) {
    return "Slot " + std::to_string(index + 1);
}

std::string currentDateTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &now_c);
#else
    localtime_r(&now_c, &localTime);
#endif

    std::ostringstream output;
    output << std::put_time(&localTime, "%Y-%m-%d %H:%M");
    return output.str();
}

std::string generateTimestampId() {
    const auto now = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return "save_" + std::to_string(ms);
}
} // namespace

SaveLoadGame& SaveLoadGame::getInstance() {
    static SaveLoadGame instance;
    return instance;
}

SaveLoadGame::SaveLoadGame() {
    std::error_code error;
    std::filesystem::create_directories(saveFilePath, error);

    saveFiles.reserve(SlotCount);
    for (int index = 0; index < SlotCount; ++index) {
        saveFiles.push_back(slotPath(index).string());
    }
}

bool SaveLoadGame::isValidSlot(int index) const noexcept {
    return index >= 0 && index < SlotCount;
}

std::filesystem::path SaveLoadGame::slotPath(int index) const {
    return saveFilePath / ("slot-" + std::to_string(index + 1) + ".json");
}

std::filesystem::path SaveLoadGame::savePathForId(const std::string& saveId) const {
    return saveFilePath / (saveId + ".json");
}

std::vector<SaveLoadGame::SaveInfo> SaveLoadGame::getAllSaves() const {
    std::vector<SaveInfo> saves;
    std::error_code error;
    if (!std::filesystem::exists(saveFilePath, error)) {
        return saves;
    }

    for (const auto& entry : std::filesystem::directory_iterator(saveFilePath, error)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") {
            continue;
        }

        std::ifstream input(entry.path());
        if (!input.is_open()) {
            continue;
        }

        try {
            nlohmann::json root;
            input >> root;
            if (root.is_object() && root.contains("state") && root["state"].is_object()) {
                SaveInfo info;
                info.id = entry.path().stem().string();
                info.filePath = entry.path().string();
                info.name = root.value("slotName", info.id);
                info.savedDate = root.value("savedAt", "");
                info.levelPath = root["state"].value("levelPath", "");
                int scoreVal = 0;
                if (root["state"].contains("score")) {
                    if (root["state"]["score"].is_number()) {
                        scoreVal = root["state"]["score"].get<int>();
                    } else if (root["state"]["score"].is_object()) {
                        scoreVal = root["state"]["score"].value("points", 0);
                    }
                }
                info.score = scoreVal;
                info.exists = true;
                saves.push_back(std::move(info));
            }
        } catch (const std::exception&) {
            // Ignore malformed files
        }
    }

    // Sort by savedDate descending (newest first), falling back to filename ID
    std::sort(saves.begin(), saves.end(), [](const SaveInfo& a, const SaveInfo& b) {
        if (a.savedDate != b.savedDate) {
            return a.savedDate > b.savedDate;
        }
        return a.id > b.id;
    });

    return saves;
}

bool SaveLoadGame::hasAnySave() const {
    return !getAllSaves().empty();
}

std::string SaveLoadGame::getDefaultSaveName(const nlohmann::json& gameState) const {
    std::string levelName = "World";
    if (gameState.is_object() && gameState.contains("levelPath") && gameState["levelPath"].is_string()) {
        std::string pathStr = gameState["levelPath"].get<std::string>();
        std::filesystem::path p(pathStr);
        std::string stem = p.stem().string();
        if (stem == "map-1") levelName = "World 1";
        else if (stem == "map-2") levelName = "World 2";
        else if (stem == "map-3") levelName = "World 3";
        else if (stem == "custom-map") levelName = "Custom Map";
        else if (!stem.empty()) levelName = stem;
    }

    std::string charName = "";
    if (gameState.is_object() && gameState.contains("player1Character") && gameState["player1Character"].is_string()) {
        std::string c = gameState["player1Character"].get<std::string>();
        if (c == "luigi") charName = "Luigi";
        else if (c == "mario") charName = "Mario";
    }

    if (!charName.empty()) {
        return levelName + " (" + charName + ") - " + currentDateTime();
    }
    return levelName + " - " + currentDateTime();
}

bool SaveLoadGame::createSave(
    const std::string& saveName,
    const nlohmann::json& gameState
) {
    if (!gameState.is_object()) {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(saveFilePath, error);
    if (error) {
        return false;
    }

    const std::string saveId = generateTimestampId();
    const std::string finalName = saveName.empty() ? getDefaultSaveName(gameState) : saveName;

    nlohmann::json root;
    root["formatVersion"] = saveFormatVersion;
    root["id"] = saveId;
    root["slotName"] = finalName;
    root["savedAt"] = currentDateTime();
    root["state"] = gameState;

    std::ofstream output(savePathForId(saveId), std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << root.dump(4);
    return output.good();
}

bool SaveLoadGame::overwriteSave(
    const std::string& saveId,
    const std::string& newName,
    const nlohmann::json& gameState
) {
    if (saveId.empty() || !gameState.is_object()) {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(saveFilePath, error);
    if (error) {
        return false;
    }

    nlohmann::json root;
    root["formatVersion"] = saveFormatVersion;
    root["id"] = saveId;
    root["slotName"] = newName.empty() ? saveId : newName;
    root["savedAt"] = currentDateTime();
    root["state"] = gameState;

    std::ofstream output(savePathForId(saveId), std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << root.dump(4);
    return output.good();
}

bool SaveLoadGame::deleteSave(const std::string& saveId) {
    if (saveId.empty()) {
        return false;
    }
    std::error_code error;
    return std::filesystem::remove(savePathForId(saveId), error);
}

bool SaveLoadGame::loadSave(
    const std::string& saveId,
    SaveInfo& info,
    nlohmann::json& gameState
) const {
    info = SaveInfo{};
    gameState = nlohmann::json{};

    const std::filesystem::path path = savePathForId(saveId);
    std::ifstream input(path);
    if (!input.is_open()) {
        return false;
    }

    try {
        nlohmann::json root;
        input >> root;
        if (!root.is_object()
            || !root.contains("state")
            || !root["state"].is_object()) {
            return false;
        }

        info.id = saveId;
        info.filePath = path.string();
        info.name = root.value("slotName", saveId);
        info.savedDate = root.value("savedAt", "");
        info.levelPath = root["state"].value("levelPath", "");
        int scoreVal = 0;
        if (root["state"].contains("score")) {
            if (root["state"]["score"].is_number()) {
                scoreVal = root["state"]["score"].get<int>();
            } else if (root["state"]["score"].is_object()) {
                scoreVal = root["state"]["score"].value("points", 0);
            }
        }
        info.score = scoreVal;
        info.exists = true;
        gameState = root["state"];
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<SaveLoadGame::SlotInfo> SaveLoadGame::getSlots() const {
    return getAllSaves();
}

bool SaveLoadGame::saveSlot(
    int index,
    const std::string& slotName,
    const nlohmann::json& gameState
) {
    if (!isValidSlot(index) || !gameState.is_object()) {
        return false;
    }

    const std::string saveId = "slot-" + std::to_string(index + 1);
    return overwriteSave(saveId, slotName, gameState);
}

bool SaveLoadGame::loadSlot(
    int index,
    SlotInfo& slot,
    nlohmann::json& gameState
) const {
    const std::string saveId = "slot-" + std::to_string(index + 1);
    return loadSave(saveId, slot, gameState);
}

void SaveLoadGame::setCurrentSession(
    const nlohmann::json& gameState,
    bool hasUnsavedChanges
) {
    if (!gameState.is_object()) {
        clearCurrentSession();
        return;
    }
    _currentSession = gameState;
    _sessionDirty = hasUnsavedChanges;
}

void SaveLoadGame::clearCurrentSession() {
    _currentSession.reset();
    _sessionDirty = false;
}

std::string SaveLoadGame::GetFile(int index) const {
    return (index >= 0 && index < static_cast<int>(saveFiles.size()))
        ? saveFiles[static_cast<std::size_t>(index)]
        : std::string{};
}

void SaveLoadGame::SaveGame(
    const std::string& saveFileName,
    const std::string& gameState
) {
    std::error_code error;
    std::filesystem::create_directories(saveFilePath, error);
    if (error) {
        return;
    }

    std::ofstream output(saveFilePath / (saveFileName + ".json"));
    if (output.is_open()) {
        output << gameState;
    }
}

void SaveLoadGame::LoadGame(
    const std::string& saveFileName,
    std::string& gameState
) const {
    gameState.clear();
    std::ifstream input(saveFilePath / (saveFileName + ".json"));
    if (!input.is_open()) {
        return;
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    gameState = contents.str();
}

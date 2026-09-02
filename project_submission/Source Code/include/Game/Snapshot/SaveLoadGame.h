#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

class SaveLoadGame {
public:
    struct SaveInfo {
        std::string id;          // Unique save ID / filename without extension
        std::string name;        // User-defined save name
        std::string savedDate;   // Formatted timestamp
        std::string levelPath;   // Map level file path
        std::string filePath;   // Full path on disk
        int score = 0;           // Optional saved score
        bool exists = true;
    };

    // Backward compatibility alias
    using SlotInfo = SaveInfo;
    static constexpr int SlotCount = 3;

    static SaveLoadGame& getInstance();

    SaveLoadGame();
    virtual ~SaveLoadGame() = default;

    // Dynamic Save Management (Infinite saves)
    std::vector<SaveInfo> getAllSaves() const;
    bool hasAnySave() const;

    bool createSave(
        const std::string& saveName,
        const nlohmann::json& gameState
    );

    bool overwriteSave(
        const std::string& saveId,
        const std::string& newName,
        const nlohmann::json& gameState
    );

    bool deleteSave(const std::string& saveId);

    bool loadSave(
        const std::string& saveId,
        SaveInfo& info,
        nlohmann::json& gameState
    ) const;

    std::string getDefaultSaveName(const nlohmann::json& gameState) const;

    // Session State Management
    void setCurrentSession(
        const nlohmann::json& gameState,
        bool hasUnsavedChanges = true
    );
    void clearCurrentSession();
    bool hasCurrentSession() const noexcept {
        return _currentSession.has_value();
    }
    const nlohmann::json* getCurrentSession() const noexcept {
        return _currentSession ? &*_currentSession : nullptr;
    }
    bool hasUnsavedSession() const noexcept {
        return _currentSession.has_value() && _sessionDirty;
    }
    void markSessionSaved() noexcept { _sessionDirty = false; }

    // Legacy slot-based API compatibility
    std::vector<SlotInfo> getSlots() const;
    bool saveSlot(
        int index,
        const std::string& slotName,
        const nlohmann::json& gameState
    );
    bool loadSlot(
        int index,
        SlotInfo& slot,
        nlohmann::json& gameState
    ) const;

    std::string GetFile(int index) const;
    virtual void SaveGame(
        const std::string& saveFileName,
        const std::string& gameState
    );
    virtual void LoadGame(
        const std::string& saveFileName,
        std::string& gameState
    ) const;

    std::string operator[](int index) const {
        return (index >= 0 && index < static_cast<int>(saveFiles.size()))
            ? saveFiles[static_cast<std::size_t>(index)]
            : std::string{};
    }

private:
    bool isValidSlot(int index) const noexcept;
    std::filesystem::path slotPath(int index) const;
    std::filesystem::path savePathForId(const std::string& saveId) const;

    std::vector<std::string> saveFiles;
    std::filesystem::path saveFilePath = "assets/SaveGameFiles";
    std::optional<nlohmann::json> _currentSession;
    bool _sessionDirty = false;
};

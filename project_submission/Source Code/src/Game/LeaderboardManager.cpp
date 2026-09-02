#include "Game/LeaderboardManager.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace {
using json = nlohmann::json;

std::string getCurrentDateString() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tm, &now_c);
#else
    localtime_r(&now_c, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d");
    return ss.str();
}

std::string normalizeKey(const std::string& key) {
    if (key.find("map-1") != std::string::npos || key.find("Level 1") != std::string::npos || key.find("map1") != std::string::npos) {
        return "map-1";
    }
    if (key.find("map-2") != std::string::npos || key.find("Level 2") != std::string::npos || key.find("map2") != std::string::npos) {
        return "map-2";
    }
    if (key.find("map-3") != std::string::npos || key.find("Level 3") != std::string::npos || key.find("map3") != std::string::npos) {
        return "map-3";
    }
    if (key.find("custom-map") != std::string::npos || key.find("Custom Map") != std::string::npos || key.find("custom") != std::string::npos) {
        return "custom-map";
    }
    if (key.find("minigame") != std::string::npos) {
        return "minigame";
    }
    return key;
}
}

const std::vector<LeaderboardEntry> LeaderboardManager::kEmptyEntries{};

LeaderboardManager& LeaderboardManager::getInstance() {
    static LeaderboardManager instance;
    return instance;
}

LeaderboardManager::LeaderboardManager() {
    load();
}

void LeaderboardManager::seedDefaults() {
    _leaderboards["map-1"] = {
        {"mario", "MARIO", 25000, 32, 280, "2026-08-20"},
        {"luigi", "LUIGI", 18500, 24, 250, "2026-08-22"},
        {"mario", "PLAYER 1", 12000, 15, 210, "2026-08-25"},
        {"luigi", "PLAYER 2", 8500, 10, 180, "2026-08-28"}
    };

    _leaderboards["map-2"] = {
        {"luigi", "LUIGI", 32000, 45, 310, "2026-08-21"},
        {"mario", "MARIO", 24000, 30, 260, "2026-08-23"},
        {"mario", "SUPER MARIO", 16000, 18, 220, "2026-08-26"},
        {"luigi", "PLAYER 2", 11000, 12, 190, "2026-08-29"}
    };

    _leaderboards["map-3"] = {
        {"mario", "MARIO", 45000, 50, 340, "2026-08-22"},
        {"luigi", "LUIGI", 38000, 42, 320, "2026-08-24"},
        {"mario", "CHAMPION", 28000, 28, 270, "2026-08-27"},
        {"luigi", "PLAYER 2", 19500, 20, 230, "2026-08-30"}
    };

    _leaderboards["minigame"] = {
        {"mario", "MARIO", 15000, 50, 60, "2026-08-28"},
        {"luigi", "LUIGI", 12000, 40, 60, "2026-08-29"}
    };

    _leaderboards["custom-map"] = {
        {"mario", "BUILDER", 20000, 25, 200, "2026-08-30"},
        {"luigi", "CREATOR", 15000, 18, 180, "2026-08-31"},
        {"mario", "PLAYER 1", 10000, 12, 150, "2026-09-01"}
    };
}

void LeaderboardManager::load(const std::string& filepath) {
    _currentFilePath = filepath;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        seedDefaults();
        save(filepath);
        return;
    }

    try {
        json j;
        file >> j;
        _leaderboards.clear();

        for (auto& [key, listJson] : j.items()) {
            std::vector<LeaderboardEntry> entries;
            if (listJson.is_array()) {
                for (const auto& item : listJson) {
                    LeaderboardEntry entry;
                    entry.character = item.value("character", "mario");
                    entry.playerName = item.value("playerName", "MARIO");
                    entry.score = item.value("score", 0);
                    entry.coins = item.value("coins", 0);
                    entry.timeRemaining = item.value("timeRemaining", 0);
                    entry.date = item.value("date", "");
                    entries.push_back(entry);
                }
            }
            _leaderboards[normalizeKey(key)] = entries;
        }

        if (_leaderboards.empty()) {
            seedDefaults();
            save(filepath);
        } else if (_leaderboards.find("custom-map") == _leaderboards.end()) {
            _leaderboards["custom-map"] = {
                {"mario", "BUILDER", 20000, 25, 200, "2026-08-30"},
                {"luigi", "CREATOR", 15000, 18, 180, "2026-08-31"},
                {"mario", "PLAYER 1", 10000, 12, 150, "2026-09-01"}
            };
            save(filepath);
        }
    } catch (const std::exception& e) {
        std::cerr << "[LeaderboardManager] Error loading " << filepath << ": " << e.what() << std::endl;
        seedDefaults();
    }
}

void LeaderboardManager::save(const std::string& filepath) {
    const std::string path = filepath.empty() ? _currentFilePath : filepath;
    json j;

    for (const auto& [key, entries] : _leaderboards) {
        json arr = json::array();
        for (const auto& entry : entries) {
            arr.push_back({
                {"character", entry.character},
                {"playerName", entry.playerName},
                {"score", entry.score},
                {"coins", entry.coins},
                {"timeRemaining", entry.timeRemaining},
                {"date", entry.date}
            });
        }
        j[key] = arr;
    }

    std::ofstream file(path);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

int LeaderboardManager::addEntry(const std::string& levelKey, const LeaderboardEntry& entry) {
    const std::string key = normalizeKey(levelKey);
    auto& list = _leaderboards[key];

    LeaderboardEntry newEntry = entry;
    if (newEntry.date.empty()) {
        newEntry.date = getCurrentDateString();
    }

    list.push_back(newEntry);

    // Sort descending by score
    std::sort(list.begin(), list.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
        if (a.score != b.score) return a.score > b.score;
        if (a.timeRemaining != b.timeRemaining) return a.timeRemaining > b.timeRemaining;
        return a.coins > b.coins;
    });

    int rank = -1;
    for (size_t i = 0; i < list.size(); ++i) {
        if (&list[i] == &newEntry || (list[i].score == newEntry.score && list[i].date == newEntry.date && list[i].playerName == newEntry.playerName)) {
            rank = static_cast<int>(i + 1);
            break;
        }
    }

    if (list.size() > kMaxEntriesPerLevel) {
        list.resize(kMaxEntriesPerLevel);
    }

    save();
    return rank <= static_cast<int>(kMaxEntriesPerLevel) ? rank : -1;
}

const std::vector<LeaderboardEntry>& LeaderboardManager::getEntries(const std::string& levelKey) const {
    const std::string key = normalizeKey(levelKey);
    auto it = _leaderboards.find(key);
    if (it != _leaderboards.end()) {
        return it->second;
    }
    return kEmptyEntries;
}

int LeaderboardManager::getHighScore(const std::string& levelKey) const {
    const auto& entries = getEntries(levelKey);
    return entries.empty() ? 0 : entries.front().score;
}

bool LeaderboardManager::isHighScore(const std::string& levelKey, int score) const {
    const auto& entries = getEntries(levelKey);
    if (entries.size() < kMaxEntriesPerLevel) return true;
    return score > entries.back().score;
}

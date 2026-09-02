#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>

struct LeaderboardEntry {
    std::string character = "mario";
    std::string playerName = "MARIO";
    int score = 0;
    int coins = 0;
    int timeRemaining = 0;
    std::string date = "";
};

class LeaderboardManager {
public:
    static LeaderboardManager& getInstance();

    void load(const std::string& filepath = "assets/datas/leaderboards.json");
    void save(const std::string& filepath = "assets/datas/leaderboards.json");

    int addEntry(const std::string& levelKey, const LeaderboardEntry& entry);
    const std::vector<LeaderboardEntry>& getEntries(const std::string& levelKey) const;
    int getHighScore(const std::string& levelKey) const;
    bool isHighScore(const std::string& levelKey, int score) const;

private:
    LeaderboardManager();
    void seedDefaults();

    std::unordered_map<std::string, std::vector<LeaderboardEntry>> _leaderboards;
    std::string _currentFilePath = "assets/datas/leaderboards.json";
    static constexpr size_t kMaxEntriesPerLevel = 8;
    static const std::vector<LeaderboardEntry> kEmptyEntries;
};

#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <algorithm>
#include "Scene/Scene.h"

struct ScoreSummaryData {
    std::string levelPath = "assets/datas/levels/map-1.json";
    std::string levelName = "WORLD 1 - GRASSLAND";
    std::string character = "mario";
    bool isWin = true;
    int baseScore = 0;
    int coinsCollected = 0;
    int timeRemaining = 0;
    int livesRemaining = 3;
    int highScore = 0;
    bool returnToMapEditor = false;
};

class ScoreComputationScene : public Scene {
public:
    explicit ScoreComputationScene(const ScoreSummaryData& data = {});
    ~ScoreComputationScene() override = default;

    void init() override;
    void onEnter() override;
    void onExit() override;
    void handleInput(const sf::Event& event) override;
    void updateVisuals(float deltaTime) override;
    void render(sf::RenderTarget& target) override;

private:
    void _finishTallyInstantly();
    void _onContinue();
    void _updateTextDisplays();

    ScoreSummaryData _data;

    static constexpr int POINTS_PER_COIN = 200;
    static constexpr int POINTS_PER_SECOND = 50;
    static constexpr int POINTS_PER_LIFE = 1000;

    int _baseLevelScore = 0;
    int _coinsBonus = 0;
    int _timeBonus = 0;
    int _livesBonus = 0;
    int _finalTotalScore = 0;
    bool _isNewHighScore = false;

    enum class TallyPhase {
        FadeIn,
        BaseScore,
        CoinsTally,
        TimeTally,
        LivesTally,
        TotalRollup,
        Finished
    };

    TallyPhase _phase = TallyPhase::FadeIn;
    float _phaseTimer = 0.0f;
    float _sfxTimer = 0.0f;

    // Animated rolling counters
    int _displayedBaseScore = 0;
    int _displayedCoins = 0;
    int _displayedCoinsScore = 0;
    int _displayedTime = 0;
    int _displayedTimeScore = 0;
    int _displayedLives = 0;
    int _displayedLivesScore = 0;
    int _displayedTotalScore = 0;

    // Visual texts & shapes
    sf::Text _titleText;
    sf::Text _levelText;
    sf::Text _baseScoreLabel;
    sf::Text _baseScoreValue;
    sf::Text _coinsLabel;
    sf::Text _coinsValue;
    sf::Text _timeLabel;
    sf::Text _timeValue;
    sf::Text _livesLabel;
    sf::Text _livesValue;
    sf::Text _dividerLine;
    sf::Text _totalLabel;
    sf::Text _totalValue;
    sf::Text _highScoreText;
    sf::Text _newRecordText;
    sf::Text _promptText;

    sf::RectangleShape _backdrop;
    sf::RectangleShape _panel;
    sf::RectangleShape _panelBorder;

    float _promptPulseTimer = 0.0f;
    float _alphaFade = 0.0f;
};

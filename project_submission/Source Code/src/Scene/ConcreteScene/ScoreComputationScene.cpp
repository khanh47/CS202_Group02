#include "Scene/ConcreteScene/ScoreComputationScene.h"
#include "ResourceManager.h"
#include "Scene/SceneManager.h"
#include "Audio/SoundManager.h"
#include "Audio/MusicManager.h"
#include "Game/LeaderboardManager.h"
#include <iomanip>
#include <sstream>
#include <cmath>

namespace {
std::string formatNumber(int number) {
    std::string s = std::to_string(number);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(n, ",");
        n -= 3;
    }
    return s;
}
}

ScoreComputationScene::ScoreComputationScene(const ScoreSummaryData& data)
    : Scene("ScoreComputationScene"),
      _data(data),
      _titleText(ResourceManager::getInstance().getFont("SuperMario"), "", 56),
      _levelText(ResourceManager::getInstance().getFont("SuperMario"), "", 26),
      _baseScoreLabel(ResourceManager::getInstance().getFont("SuperMario"), "BASE SCORE", 30),
      _baseScoreValue(ResourceManager::getInstance().getFont("SuperMario"), "0 PTS", 30),
      _coinsLabel(ResourceManager::getInstance().getFont("SuperMario"), "COINS BONUS", 30),
      _coinsValue(ResourceManager::getInstance().getFont("SuperMario"), "0 PTS", 30),
      _timeLabel(ResourceManager::getInstance().getFont("SuperMario"), "TIME BONUS", 30),
      _timeValue(ResourceManager::getInstance().getFont("SuperMario"), "0 PTS", 30),
      _livesLabel(ResourceManager::getInstance().getFont("SuperMario"), "LIVES BONUS", 30),
      _livesValue(ResourceManager::getInstance().getFont("SuperMario"), "0 PTS", 30),
      _dividerLine(ResourceManager::getInstance().getFont("SuperMario"), "----------------------------------------", 26),
      _totalLabel(ResourceManager::getInstance().getFont("SuperMario"), "TOTAL SCORE", 40),
      _totalValue(ResourceManager::getInstance().getFont("SuperMario"), "0 PTS", 44),
      _highScoreText(ResourceManager::getInstance().getFont("SuperMario"), "", 26),
      _newRecordText(ResourceManager::getInstance().getFont("SuperMario"), "* NEW HIGH SCORE RECORD! *", 30),
      _promptText(ResourceManager::getInstance().getFont("SuperMario"), "PRESS ANY KEY TO CONTINUE", 26) {

    // Precalculate bonus components
    _coinsBonus = _data.coinsCollected * POINTS_PER_COIN;
    _baseLevelScore = std::max(0, _data.baseScore - _coinsBonus);
    _timeBonus = _data.timeRemaining * POINTS_PER_SECOND;
    _livesBonus = _data.isWin ? (_data.livesRemaining * POINTS_PER_LIFE) : 0;
    _finalTotalScore = _baseLevelScore + _coinsBonus + _timeBonus + _livesBonus;
    _isNewHighScore = (_finalTotalScore > _data.highScore);

    // Automatically submit score to persistent Leaderboard
    LeaderboardEntry entry;
    entry.character = _data.character.empty() ? "mario" : _data.character;
    entry.playerName = entry.character == "luigi" ? "LUIGI" : "MARIO";
    entry.score = _finalTotalScore;
    entry.coins = _data.coinsCollected;
    entry.timeRemaining = _data.timeRemaining;
    LeaderboardManager::getInstance().addEntry(_data.levelPath, entry);

    init();
}

void ScoreComputationScene::init() {
    const sf::Vector2f viewSize(1920.f, 1080.f);

    _backdrop.setSize(viewSize);
    _backdrop.setFillColor(sf::Color(8, 10, 18, 220));

    const sf::Vector2f panelSize(1200.f, 740.f);
    _panel.setSize(panelSize);
    _panel.setOrigin(panelSize * 0.5f);
    _panel.setPosition({viewSize.x * 0.5f, viewSize.y * 0.48f});
    _panel.setFillColor(sf::Color(18, 22, 36, 240));
    _panel.setOutlineColor(sf::Color(255, 215, 60, 220));
    _panel.setOutlineThickness(4.0f);

    // Title configuration
    if (_data.isWin) {
        _titleText.setString("COURSE CLEAR!");
        _titleText.setFillColor(sf::Color(255, 225, 60));
    } else {
        _titleText.setString("GAME OVER - RESULTS");
        _titleText.setFillColor(sf::Color(255, 75, 75));
    }
    _titleText.setOutlineColor(sf::Color::Black);
    _titleText.setOutlineThickness(3.0f);
    sf::FloatRect tb = _titleText.getLocalBounds();
    _titleText.setOrigin({tb.position.x + tb.size.x * 0.5f, tb.position.y + tb.size.y * 0.5f});
    _titleText.setPosition({viewSize.x * 0.5f, 210.0f});

    // Level subtitle
    _levelText.setString(_data.levelName);
    _levelText.setFillColor(sf::Color(180, 210, 255));
    _levelText.setOutlineColor(sf::Color::Black);
    _levelText.setOutlineThickness(2.0f);
    sf::FloatRect lb = _levelText.getLocalBounds();
    _levelText.setOrigin({lb.position.x + lb.size.x * 0.5f, lb.position.y + lb.size.y * 0.5f});
    _levelText.setPosition({viewSize.x * 0.5f, 275.0f});

    // Configure row styling
    auto setupRow = [](sf::Text& label, sf::Text& val, float y, sf::Color color) {
        label.setFillColor(sf::Color::White);
        label.setOutlineColor(sf::Color::Black);
        label.setOutlineThickness(2.0f);
        label.setPosition({440.0f, y});

        val.setFillColor(color);
        val.setOutlineColor(sf::Color::Black);
        val.setOutlineThickness(2.0f);
    };

    setupRow(_baseScoreLabel, _baseScoreValue, 340.0f, sf::Color(220, 235, 255));
    setupRow(_coinsLabel, _coinsValue, 405.0f, sf::Color(255, 225, 80));
    setupRow(_timeLabel, _timeValue, 470.0f, sf::Color(120, 255, 160));
    setupRow(_livesLabel, _livesValue, 535.0f, sf::Color(255, 150, 150));

    // Divider
    _dividerLine.setFillColor(sf::Color(100, 120, 160));
    _dividerLine.setOutlineColor(sf::Color::Black);
    _dividerLine.setOutlineThickness(1.0f);
    sf::FloatRect divB = _dividerLine.getLocalBounds();
    _dividerLine.setOrigin({divB.position.x + divB.size.x * 0.5f, divB.position.y + divB.size.y * 0.5f});
    _dividerLine.setPosition({viewSize.x * 0.5f, 595.0f});

    // Total Score
    _totalLabel.setFillColor(sf::Color(255, 235, 90));
    _totalLabel.setOutlineColor(sf::Color::Black);
    _totalLabel.setOutlineThickness(3.0f);
    _totalLabel.setPosition({440.0f, 630.0f});

    _totalValue.setFillColor(sf::Color(255, 235, 90));
    _totalValue.setOutlineColor(sf::Color::Black);
    _totalValue.setOutlineThickness(3.0f);

    // High Score
    _highScoreText.setString("BEST SCORE: " + formatNumber(std::max(_data.highScore, _finalTotalScore)) + " PTS");
    _highScoreText.setFillColor(sf::Color(180, 190, 210));
    _highScoreText.setOutlineColor(sf::Color::Black);
    _highScoreText.setOutlineThickness(2.0f);
    sf::FloatRect hb = _highScoreText.getLocalBounds();
    _highScoreText.setOrigin({hb.position.x + hb.size.x * 0.5f, hb.position.y + hb.size.y * 0.5f});
    _highScoreText.setPosition({viewSize.x * 0.5f, 720.0f});

    // New High Score Banner
    _newRecordText.setFillColor(sf::Color(255, 80, 80));
    _newRecordText.setOutlineColor(sf::Color::Black);
    _newRecordText.setOutlineThickness(2.5f);
    sf::FloatRect nb = _newRecordText.getLocalBounds();
    _newRecordText.setOrigin({nb.position.x + nb.size.x * 0.5f, nb.position.y + nb.size.y * 0.5f});
    _newRecordText.setPosition({viewSize.x * 0.5f, 770.0f});

    // Prompt
    _promptText.setFillColor(sf::Color::White);
    _promptText.setOutlineColor(sf::Color::Black);
    _promptText.setOutlineThickness(2.0f);
    sf::FloatRect pb = _promptText.getLocalBounds();
    _promptText.setOrigin({pb.position.x + pb.size.x * 0.5f, pb.position.y + pb.size.y * 0.5f});
    _promptText.setPosition({viewSize.x * 0.5f, 920.0f});

    _updateTextDisplays();
}

void ScoreComputationScene::onEnter() {
    _isActive = true;
    _phase = TallyPhase::FadeIn;
    _phaseTimer = 0.0f;
    _sfxTimer = 0.0f;
    _alphaFade = 0.0f;

    _displayedBaseScore = _baseLevelScore;
    _displayedCoins = 0;
    _displayedCoinsScore = 0;
    _displayedTime = 0;
    _displayedTimeScore = 0;
    _displayedLives = 0;
    _displayedLivesScore = 0;
    _displayedTotalScore = _baseLevelScore;

    _updateTextDisplays();
}

void ScoreComputationScene::onExit() {
    _isActive = false;
}

void ScoreComputationScene::handleInput(const sf::Event& event) {
    if (event.is<sf::Event::KeyPressed>()
        || event.is<sf::Event::MouseButtonPressed>()
        || event.is<sf::Event::JoystickButtonPressed>()) {

        if (_phase != TallyPhase::Finished) {
            // Skip animation and finalize tally immediately
            _finishTallyInstantly();
        } else {
            // Proceed to menu / level selection
            _onContinue();
        }
    }
}

void ScoreComputationScene::_finishTallyInstantly() {
    _displayedBaseScore = _baseLevelScore;
    _displayedCoins = _data.coinsCollected;
    _displayedCoinsScore = _coinsBonus;
    _displayedTime = _data.timeRemaining;
    _displayedTimeScore = _timeBonus;
    _displayedLives = _data.livesRemaining;
    _displayedLivesScore = _livesBonus;
    _displayedTotalScore = _finalTotalScore;

    _phase = TallyPhase::Finished;
    _alphaFade = 1.0f;
    _updateTextDisplays();
    Audio::SoundManager::getInstance().playEffect("coin");
}

void ScoreComputationScene::_onContinue() {
    if (auto* mgr = getSceneManager()) {
        if (_data.returnToMapEditor) {
            mgr->requestReturnToMapEditor();
        } else {
            mgr->requestReturnToModeMenu();
        }
    }
}

void ScoreComputationScene::_updateTextDisplays() {
    auto alignRight = [](sf::Text& txt, float rightX, float y) {
        sf::FloatRect b = txt.getLocalBounds();
        txt.setOrigin({b.position.x + b.size.x, b.position.y});
        txt.setPosition({rightX, y});
    };

    const float rightAlignX = 1480.0f;

    // Base score display
    _baseScoreValue.setString(formatNumber(_displayedBaseScore) + " PTS");
    alignRight(_baseScoreValue, rightAlignX, 340.0f);

    // Coins display
    std::string coinStr = std::to_string(_displayedCoins) + " x 200 = " + formatNumber(_displayedCoinsScore) + " PTS";
    _coinsValue.setString(coinStr);
    alignRight(_coinsValue, rightAlignX, 405.0f);

    // Time display
    std::string timeStr = std::to_string(_displayedTime) + "s x 50 = " + formatNumber(_displayedTimeScore) + " PTS";
    _timeValue.setString(timeStr);
    alignRight(_timeValue, rightAlignX, 470.0f);

    // Lives display
    std::string livesStr = std::to_string(_displayedLives) + " x 1,000 = " + formatNumber(_displayedLivesScore) + " PTS";
    _livesValue.setString(livesStr);
    alignRight(_livesValue, rightAlignX, 535.0f);

    // Total score display
    _totalValue.setString(formatNumber(_displayedTotalScore) + " PTS");
    alignRight(_totalValue, rightAlignX, 630.0f);
}

void ScoreComputationScene::updateVisuals(float deltaTime) {
    _phaseTimer += deltaTime;
    _sfxTimer += deltaTime;
    _promptPulseTimer += deltaTime * 4.0f;

    switch (_phase) {
        case TallyPhase::FadeIn:
            _alphaFade = std::min(1.0f, _phaseTimer / 0.4f);
            if (_phaseTimer >= 0.4f) {
                _phase = TallyPhase::BaseScore;
                _phaseTimer = 0.0f;
            }
            break;

        case TallyPhase::BaseScore:
            _displayedBaseScore = _baseLevelScore;
            _displayedTotalScore = _baseLevelScore;
            _updateTextDisplays();
            if (_phaseTimer >= 0.4f) {
                _phase = TallyPhase::CoinsTally;
                _phaseTimer = 0.0f;
            }
            break;

        case TallyPhase::CoinsTally: {
            if (_data.coinsCollected > 0) {
                float progress = std::min(1.0f, _phaseTimer / 0.8f);
                int targetCoins = static_cast<int>(progress * _data.coinsCollected);
                if (targetCoins > _displayedCoins) {
                    _displayedCoins = targetCoins;
                    _displayedCoinsScore = _displayedCoins * POINTS_PER_COIN;
                    _displayedTotalScore = _baseLevelScore + _displayedCoinsScore;
                    if (_sfxTimer >= 0.08f) {
                        Audio::SoundManager::getInstance().playEffect("coin");
                        _sfxTimer = 0.0f;
                    }
                }
            } else {
                _displayedCoins = 0;
                _displayedCoinsScore = 0;
                _displayedTotalScore = _baseLevelScore;
            }
            _updateTextDisplays();
            if (_phaseTimer >= 1.0f) {
                _displayedCoins = _data.coinsCollected;
                _displayedCoinsScore = _coinsBonus;
                _displayedTotalScore = _baseLevelScore + _coinsBonus;
                _phase = TallyPhase::TimeTally;
                _phaseTimer = 0.0f;
            }
            break;
        }

        case TallyPhase::TimeTally: {
            if (_data.timeRemaining > 0) {
                float progress = std::min(1.0f, _phaseTimer / 1.0f);
                int targetTime = static_cast<int>(progress * _data.timeRemaining);
                if (targetTime > _displayedTime) {
                    _displayedTime = targetTime;
                    _displayedTimeScore = _displayedTime * POINTS_PER_SECOND;
                    _displayedTotalScore = _baseLevelScore + _coinsBonus + _displayedTimeScore;
                    if (_sfxTimer >= 0.06f) {
                        Audio::SoundManager::getInstance().playEffect("coin");
                        _sfxTimer = 0.0f;
                    }
                }
            } else {
                _displayedTime = 0;
                _displayedTimeScore = 0;
                _displayedTotalScore = _baseLevelScore + _coinsBonus;
            }
            _updateTextDisplays();
            if (_phaseTimer >= 1.2f) {
                _displayedTime = _data.timeRemaining;
                _displayedTimeScore = _timeBonus;
                _displayedTotalScore = _baseLevelScore + _coinsBonus + _timeBonus;
                _phase = TallyPhase::LivesTally;
                _phaseTimer = 0.0f;
            }
            break;
        }

        case TallyPhase::LivesTally: {
            if (_data.isWin && _data.livesRemaining > 0) {
                float progress = std::min(1.0f, _phaseTimer / 0.6f);
                int targetLives = static_cast<int>(progress * _data.livesRemaining);
                if (targetLives > _displayedLives) {
                    _displayedLives = targetLives;
                    _displayedLivesScore = _displayedLives * POINTS_PER_LIFE;
                    _displayedTotalScore = _baseLevelScore + _coinsBonus + _timeBonus + _displayedLivesScore;
                    Audio::SoundManager::getInstance().playEffect("power_up");
                }
            } else {
                _displayedLives = _data.livesRemaining;
                _displayedLivesScore = _livesBonus;
                _displayedTotalScore = _baseLevelScore + _coinsBonus + _timeBonus + _livesBonus;
            }
            _updateTextDisplays();
            if (_phaseTimer >= 0.8f) {
                _phase = TallyPhase::TotalRollup;
                _phaseTimer = 0.0f;
            }
            break;
        }

        case TallyPhase::TotalRollup: {
            _displayedTotalScore = _finalTotalScore;
            _updateTextDisplays();
            if (_phaseTimer >= 0.3f) {
                _phase = TallyPhase::Finished;
                _phaseTimer = 0.0f;
                Audio::SoundManager::getInstance().playEffect("one_up");
            }
            break;
        }

        case TallyPhase::Finished:
            // Pulsing continue prompt
            break;
    }
}

void ScoreComputationScene::render(sf::RenderTarget& target) {
    const sf::View defaultView = target.getDefaultView();
    target.setView(defaultView);

    // Draw backdrop and panel
    target.draw(_backdrop);
    target.draw(_panel);

    // Header & Subtitle
    target.draw(_titleText);
    target.draw(_levelText);

    // Rows
    target.draw(_baseScoreLabel);
    target.draw(_baseScoreValue);
    target.draw(_coinsLabel);
    target.draw(_coinsValue);
    target.draw(_timeLabel);
    target.draw(_timeValue);
    target.draw(_livesLabel);
    target.draw(_livesValue);

    // Total line
    target.draw(_dividerLine);
    target.draw(_totalLabel);
    target.draw(_totalValue);

    // High Score and New Record
    target.draw(_highScoreText);
    if (_isNewHighScore && _phase == TallyPhase::Finished) {
        float pulse = 0.5f + 0.5f * std::sin(_promptPulseTimer * 1.5f);
        _newRecordText.setFillColor(sf::Color(255, static_cast<std::uint8_t>(180 + 75 * pulse), 50));
        target.draw(_newRecordText);
    }

    // Prompt
    if (_phase == TallyPhase::Finished) {
        std::uint8_t alpha = static_cast<std::uint8_t>(140 + 115 * (0.5f + 0.5f * std::sin(_promptPulseTimer)));
        _promptText.setFillColor(sf::Color(255, 255, 255, alpha));
        target.draw(_promptText);
    }
}

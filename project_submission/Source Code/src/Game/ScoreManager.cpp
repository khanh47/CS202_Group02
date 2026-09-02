#include "Game/ScoreManager.h"
#include "Game/Objects/Block/CoinBlock.h"
#include "Game/Objects/Player/Player.h"
#include "Game/Objects/Player/State/SuperState.h"
#include "Game/Objects/Player/State/FireState.h"
#include "Game/World/GameWorld.h"
#include "Game/GameSettings.h"
#include "ResourceManager.h"
#include <cmath>

ScoreManager::ScoreManager() {
    _score = 0;
    _marioScore = 0;
    _luigiScore = 0;
    _marioCoins = 0;
    _luigiCoins = 0;
    _marioLives = 3;
    _luigiLives = 3;
    _highScore = 0;
    _stompComboIndex = 0;
    _timeRemaining = 400.0f;
    _initialTime = 400.0f;
    _timePaused = false;
    _coinAnimTimer = 0.0f;
}

void ScoreManager::handleEvent(ScoreEventType event, sf::Vector2f position, int detail, const std::string& character) {
    int pointsAwarded = 0;
    std::string displayText = "";

    const bool isLuigi = (character == "luigi");

    switch (event) {
        case ScoreEventType::CoinCollected:
            addCoins(1, character);
            pointsAwarded = 200;
            displayText = "200";
            break;

        case ScoreEventType::MegaCoinCollected:
            // Reset stomp combo ladder when a Mega Coin is collected
            _stompComboIndex = 0;
            pointsAwarded = 1000;
            displayText = "1000";
            break;

        case ScoreEventType::EnemyStomped: {
            size_t index = std::min(static_cast<size_t>(_stompComboIndex), _stompSequence.size() - 1);
            auto reward = _stompSequence[index];

            if (std::holds_alternative<std::string>(reward)) {
                if (isLuigi) {
                    _luigiLives++;
                } else {
                    _marioLives++;
                }
                displayText = std::get<std::string>(reward); // "1UP"
            } else {
                pointsAwarded = std::get<int>(reward);
                displayText = std::to_string(pointsAwarded);
            }

            // Increment combo count for consecutive airborne stomps
            _stompComboIndex++;
            break;
        }

        case ScoreEventType::MarioLanded:
            // Touch ground -> Reset stomp combo ladder
            _stompComboIndex = 0;
            return;

        case ScoreEventType::BlockBroken:
            pointsAwarded = 50;
            displayText = "50";
            break;

        case ScoreEventType::PowerupCollected:
            pointsAwarded = 1000;
            displayText = "1000";
            break;

        case ScoreEventType::OneUpCollected:
            if (isLuigi) {
                _luigiLives++;
            } else {
                _marioLives++;
            }
            displayText = "1UP";
            break;

        case ScoreEventType::FlagpoleReached: {
            int flagpolePoints = (detail > 0) ? detail : 1000;
            pointsAwarded = flagpolePoints;
            displayText = std::to_string(pointsAwarded);
            _timePaused = true;
            break;
        }

        case ScoreEventType::LostLive:
            // Reset stomp combo ladder when a life is lost
            _stompComboIndex = 0;
            if (isLuigi) {
                _luigiLives = std::max(0, _luigiLives - 1);
            } else {
                _marioLives = std::max(0, _marioLives - 1);
            }
            break;

        case ScoreEventType::CoinBlockTouched:
            addCoins(1, character);
            pointsAwarded = 200;
            displayText = "200";
            break;
    }

    if (pointsAwarded > 0) {
        addScore(pointsAwarded, character);
    }

    if (!displayText.empty() && (position.x != 0.f || position.y != 0.f)) {
        spawnFloatingText(position, displayText);
    }
}

void ScoreManager::update(float deltaTime) {
    // Update level timer
    if (!_timePaused && _timeRemaining > 0.0f) {
        _timeRemaining -= deltaTime;
        if (_timeRemaining < 0.0f) {
            _timeRemaining = 0.0f;
        }
    }

    _coinAnimTimer += deltaTime;
    if (_coinAnimTimer > 1000.0f) {
        _coinAnimTimer = 0.0f;
    }

    for (auto& popup : _floatingTexts) {
        popup.update(deltaTime);
    }

    _floatingTexts.erase(
        std::remove_if(_floatingTexts.begin(), _floatingTexts.end(),
            [](const FloatingText& text) { return text.isDead(); }),
        _floatingTexts.end()
    );
}

void ScoreManager::renderFloatingTexts(sf::RenderTarget& target, const sf::Font& font) const {
    for (const auto& popup : _floatingTexts) {
        sf::Text text(font, popup.text, 28);
        text.setPosition(popup.position);
        
        std::uint8_t alphaByte = static_cast<std::uint8_t>(std::clamp(popup.alpha * 255.0f, 0.0f, 255.0f));
        text.setFillColor(sf::Color(255, 255, 255, alphaByte));
        text.setOutlineColor(sf::Color(0, 0, 0, alphaByte));
        text.setOutlineThickness(1.0f);

        target.draw(text);
    }
}

void ScoreManager::renderHUD(
    sf::RenderTarget& target,
    const sf::Font& font,
    const GameWorld* gameWorld,
    sf::Vector2f hudPosition
) const {
    auto& resources = ResourceManager::getInstance();
    const sf::Texture* marioTex = nullptr;
    const sf::Texture* luigiTex = nullptr;
    const sf::Texture* coinTex = nullptr;
    const sf::Texture* itemsTex = nullptr;
    const sf::Texture* megaTex = nullptr;

    try { marioTex = &resources.getTexture("mario_spritesheet"); } catch (...) {}
    try { luigiTex = &resources.getTexture("luigi_spritesheet"); } catch (...) {}
    try { coinTex = &resources.getTexture("coin_spritesheet"); } catch (...) {}
    try { itemsTex = &resources.getTexture("mario_and_items"); } catch (...) {}
    try { megaTex = &resources.getTexture("mega_mushroom_spritesheet"); } catch (...) {}

    // Find active powerup state for players if gameWorld is available
    enum class PowerupBadge { None, Super, Fire, Star, Mega };
    PowerupBadge marioPowerup = PowerupBadge::None;
    PowerupBadge luigiPowerup = PowerupBadge::None;

    bool marioAlive = false;
    bool luigiAlive = false;

    if (gameWorld) {
        for (const auto& player : gameWorld->getPlayers()) {
            if (!player || player->isEliminated()) continue;
            PowerupBadge badge = PowerupBadge::None;
            if (player->isMegaState()) {
                badge = PowerupBadge::Mega;
            } else if (player->isStarManState()) {
                badge = PowerupBadge::Star;
            } else if (dynamic_cast<FireState*>(player->getState())) {
                badge = PowerupBadge::Fire;
            } else if (dynamic_cast<SuperState*>(player->getState())) {
                badge = PowerupBadge::Super;
            }

            if (player->getCharacter() == "luigi") {
                luigiPowerup = badge;
                luigiAlive = true;
            } else {
                marioPowerup = badge;
                marioAlive = true;
            }
        }
    }

    // Helper: Draw Power-up status badge sprite
    auto drawPowerupBadge = [&](PowerupBadge badge, sf::Vector2f pos, float size) {
        if (badge == PowerupBadge::None) return;
        if (badge == PowerupBadge::Mega && megaTex) {
            sf::Sprite sprite(*megaTex);
            sprite.setTextureRect(sf::IntRect({0, 0}, {1402, 1122}));
            sprite.setScale({size / 1402.0f, size / 1122.0f});
            sprite.setPosition(pos);
            target.draw(sprite);
        } else if (itemsTex) {
            sf::Sprite sprite(*itemsTex);
            if (badge == PowerupBadge::Super) {
                sprite.setTextureRect(sf::IntRect({0, 90}, {18, 18}));
            } else if (badge == PowerupBadge::Fire) {
                sprite.setTextureRect(sf::IntRect({0, 108}, {18, 18}));
            } else if (badge == PowerupBadge::Star) {
                sprite.setTextureRect(sf::IntRect({0, 126}, {18, 18}));
            }
            sprite.setScale({size / 18.0f, size / 18.0f});
            sprite.setPosition(pos);
            target.draw(sprite);
        }
    };

    // Helper: Animated coin frame
    const int coinFrame = static_cast<int>(_coinAnimTimer * 6.0f) % 4;
    const sf::IntRect coinRect({coinFrame * 72, 0}, {64, 64});

    const GameMode mode = GameSettings::getInstance().gameMode;

    if (mode == GameMode::Coop || mode == GameMode::Minigame) {
        // ===================================================================
        // 2-PLAYER CO-OP & MINIGAME DUAL HUD LAYOUT
        // ===================================================================
        const float y = hudPosition.y;

        // 1. MARIO BLOCK (X = 40)
        {
            const float startX = hudPosition.x;
            // Title & Score
            sf::Text nameText(font, "MARIO", 32);
            nameText.setPosition({startX, y});
            nameText.setFillColor(sf::Color(255, 65, 65));
            nameText.setOutlineColor(sf::Color::Black);
            nameText.setOutlineThickness(2.0f);
            target.draw(nameText);

            sf::Text scoreText(font, getFormattedScore("mario"), 26);
            scoreText.setPosition({startX, y + 42.0f});
            scoreText.setFillColor(sf::Color::White);
            scoreText.setOutlineColor(sf::Color::Black);
            scoreText.setOutlineThickness(2.0f);
            target.draw(scoreText);

            // Mario Head Icon & Lives
            if (marioTex) {
                sf::Sprite head(*marioTex);
                head.setTextureRect(sf::IntRect({48, 32}, {32, 32}));
                head.setScale({30.0f / 32.0f, 30.0f / 32.0f});
                head.setPosition({startX + 170.0f, y + 2.0f});
                target.draw(head);
            }

            const int marioLives = (mode == GameMode::Minigame)
                ? (marioAlive ? 1 : 0)
                : _marioLives;
            sf::Text livesText(font, "x" + std::to_string(marioLives), 26);
            livesText.setPosition({startX + 208.0f, y + 4.0f});
            livesText.setFillColor(sf::Color::White);
            livesText.setOutlineColor(sf::Color::Black);
            livesText.setOutlineThickness(2.0f);
            target.draw(livesText);

            // Mario Powerup Badge
            drawPowerupBadge(marioPowerup, {startX + 270.0f, y + 4.0f}, 26.0f);

            // Mario Coin Icon & Count
            if (coinTex) {
                sf::Sprite coinSprite(*coinTex);
                coinSprite.setTextureRect(coinRect);
                coinSprite.setScale({26.0f / 64.0f, 26.0f / 64.0f});
                coinSprite.setPosition({startX + 172.0f, y + 44.0f});
                target.draw(coinSprite);
            }

            sf::Text coinsText(font, "x" + getFormattedCoins("mario"), 26);
            coinsText.setPosition({startX + 208.0f, y + 42.0f});
            coinsText.setFillColor(sf::Color(255, 220, 0));
            coinsText.setOutlineColor(sf::Color::Black);
            coinsText.setOutlineThickness(2.0f);
            target.draw(coinsText);
        }

        // 2. LUIGI BLOCK (X = 560)
        {
            const float startX = hudPosition.x + 520.0f;
            // Title & Score
            sf::Text nameText(font, "LUIGI", 32);
            nameText.setPosition({startX, y});
            nameText.setFillColor(sf::Color(60, 255, 100));
            nameText.setOutlineColor(sf::Color::Black);
            nameText.setOutlineThickness(2.0f);
            target.draw(nameText);

            sf::Text scoreText(font, getFormattedScore("luigi"), 26);
            scoreText.setPosition({startX, y + 42.0f});
            scoreText.setFillColor(sf::Color::White);
            scoreText.setOutlineColor(sf::Color::Black);
            scoreText.setOutlineThickness(2.0f);
            target.draw(scoreText);

            // Luigi Head Icon & Lives
            if (luigiTex) {
                sf::Sprite head(*luigiTex);
                head.setTextureRect(sf::IntRect({48, 32}, {32, 32}));
                head.setScale({30.0f / 32.0f, 30.0f / 32.0f});
                head.setPosition({startX + 170.0f, y + 2.0f});
                target.draw(head);
            }

            const int luigiLives = (mode == GameMode::Minigame)
                ? (luigiAlive ? 1 : 0)
                : _luigiLives;
            sf::Text livesText(font, "x" + std::to_string(luigiLives), 26);
            livesText.setPosition({startX + 208.0f, y + 4.0f});
            livesText.setFillColor(sf::Color::White);
            livesText.setOutlineColor(sf::Color::Black);
            livesText.setOutlineThickness(2.0f);
            target.draw(livesText);

            // Luigi Powerup Badge
            drawPowerupBadge(luigiPowerup, {startX + 270.0f, y + 4.0f}, 26.0f);

            // Luigi Coin Icon & Count
            if (coinTex) {
                sf::Sprite coinSprite(*coinTex);
                coinSprite.setTextureRect(coinRect);
                coinSprite.setScale({26.0f / 64.0f, 26.0f / 64.0f});
                coinSprite.setPosition({startX + 172.0f, y + 44.0f});
                target.draw(coinSprite);
            }

            sf::Text coinsText(font, "x" + getFormattedCoins("luigi"), 26);
            coinsText.setPosition({startX + 208.0f, y + 42.0f});
            coinsText.setFillColor(sf::Color(255, 220, 0));
            coinsText.setOutlineColor(sf::Color::Black);
            coinsText.setOutlineThickness(2.0f);
            target.draw(coinsText);
        }

        // 3. TIME BLOCK (X = 1080)
        {
            const float startX = hudPosition.x + 1040.0f;
            sf::Text timeHeader(font, "TIME", 32);
            timeHeader.setPosition({startX, y});
            timeHeader.setFillColor(sf::Color::White);
            timeHeader.setOutlineColor(sf::Color::Black);
            timeHeader.setOutlineThickness(2.0f);
            target.draw(timeHeader);

            sf::Text timeText(font, getFormattedTime(), 26);
            timeText.setPosition({startX, y + 42.0f});
            timeText.setFillColor(sf::Color::White);
            timeText.setOutlineColor(sf::Color::Black);
            timeText.setOutlineThickness(2.0f);
            target.draw(timeText);
        }

        // 4. SUDDEN DEATH BADGE (FOR MINIGAME MODE)
        if (mode == GameMode::Minigame) {
            const float startX = hudPosition.x + 1380.0f;
            sf::Text duelHeader(font, "DUEL", 32);
            duelHeader.setPosition({startX, y});
            duelHeader.setFillColor(sf::Color(255, 215, 0));
            duelHeader.setOutlineColor(sf::Color::Black);
            duelHeader.setOutlineThickness(2.0f);
            target.draw(duelHeader);

            sf::Text duelSub(font, "SUDDEN DEATH", 20);
            duelSub.setPosition({startX, y + 44.0f});
            duelSub.setFillColor(sf::Color(255, 120, 120));
            duelSub.setOutlineColor(sf::Color::Black);
            duelSub.setOutlineThickness(2.0f);
            target.draw(duelSub);
        }
    } else {
        // ===================================================================
        // 1-PLAYER SOLO HUD LAYOUT
        // ===================================================================
        const bool isLuigiSolo = (GameSettings::getInstance().player1Character == "luigi");
        const std::string charName = isLuigiSolo ? "luigi" : "mario";
        const int charLives = isLuigiSolo ? _luigiLives : _marioLives;
        const PowerupBadge charPowerup = isLuigiSolo ? luigiPowerup : marioPowerup;
        const sf::Texture* charTex = isLuigiSolo ? luigiTex : marioTex;
        const sf::Color charColor = isLuigiSolo ? sf::Color(60, 255, 100) : sf::Color(255, 65, 65);

        const float y = hudPosition.y;

        // 1. CHARACTER & SCORE
        {
            const float x = hudPosition.x;
            sf::Text nameText(font, isLuigiSolo ? "LUIGI" : "MARIO", 34);
            nameText.setPosition({x, y});
            nameText.setFillColor(charColor);
            nameText.setOutlineColor(sf::Color::Black);
            nameText.setOutlineThickness(2.0f);
            target.draw(nameText);

            sf::Text scoreText(font, getFormattedScore(charName), 30);
            scoreText.setPosition({x, y + 44.0f});
            scoreText.setFillColor(sf::Color::White);
            scoreText.setOutlineColor(sf::Color::Black);
            scoreText.setOutlineThickness(2.0f);
            target.draw(scoreText);
        }

        // 2. LIVES & POWER-UP
        {
            const float x = hudPosition.x + 360.0f;
            sf::Text livesHeader(font, "LIVES", 34);
            livesHeader.setPosition({x, y});
            livesHeader.setFillColor(sf::Color::White);
            livesHeader.setOutlineColor(sf::Color::Black);
            livesHeader.setOutlineThickness(2.0f);
            target.draw(livesHeader);

            if (charTex) {
                sf::Sprite head(*charTex);
                head.setTextureRect(sf::IntRect({48, 32}, {32, 32}));
                head.setScale({32.0f / 32.0f, 32.0f / 32.0f});
                head.setPosition({x, y + 44.0f});
                target.draw(head);
            }

            sf::Text livesText(font, "x" + std::to_string(charLives), 30);
            livesText.setPosition({x + 40.0f, y + 44.0f});
            livesText.setFillColor(sf::Color::White);
            livesText.setOutlineColor(sf::Color::Black);
            livesText.setOutlineThickness(2.0f);
            target.draw(livesText);

            // Powerup status badge
            drawPowerupBadge(charPowerup, {x + 105.0f, y + 46.0f}, 28.0f);
        }

        // 3. COINS
        {
            const float x = hudPosition.x + 700.0f;
            sf::Text coinsHeader(font, "COINS", 34);
            coinsHeader.setPosition({x, y});
            coinsHeader.setFillColor(sf::Color::White);
            coinsHeader.setOutlineColor(sf::Color::Black);
            coinsHeader.setOutlineThickness(2.0f);
            target.draw(coinsHeader);

            if (coinTex) {
                sf::Sprite coinSprite(*coinTex);
                coinSprite.setTextureRect(coinRect);
                coinSprite.setScale({30.0f / 64.0f, 30.0f / 64.0f});
                coinSprite.setPosition({x, y + 44.0f});
                target.draw(coinSprite);
            }

            sf::Text coinsText(font, "x" + getFormattedCoins(charName), 30);
            coinsText.setPosition({x + 38.0f, y + 44.0f});
            coinsText.setFillColor(sf::Color(255, 220, 0));
            coinsText.setOutlineColor(sf::Color::Black);
            coinsText.setOutlineThickness(2.0f);
            target.draw(coinsText);
        }

        // 4. TIME
        {
            const float x = hudPosition.x + 1040.0f;
            sf::Text timeHeader(font, "TIME", 34);
            timeHeader.setPosition({x, y});
            timeHeader.setFillColor(sf::Color::White);
            timeHeader.setOutlineColor(sf::Color::Black);
            timeHeader.setOutlineThickness(2.0f);
            target.draw(timeHeader);

            sf::Text timeText(font, getFormattedTime(), 30);
            timeText.setPosition({x, y + 44.0f});
            timeText.setFillColor(sf::Color::White);
            timeText.setOutlineColor(sf::Color::Black);
            timeText.setOutlineThickness(2.0f);
            target.draw(timeText);
        }
    }
}

int ScoreManager::convertRemainingTimeToScore(int pointsPerSecond) {
    int secondsLeft = getIntTimeRemaining();
    int bonusScore = secondsLeft * pointsPerSecond;
    addScore(bonusScore, "mario");
    _timeRemaining = 0.0f;
    return bonusScore;
}

int ScoreManager::convertRemainingTimeToScore(int secondsLeft, int pointsPerSecond) {
    int bonusScore = secondsLeft * pointsPerSecond;
    addScore(bonusScore, "mario");
    return bonusScore;
}

void ScoreManager::addScore(int amount, const std::string& character) {
    _score += amount;
    if (character == "luigi") {
        _luigiScore += amount;
    } else {
        _marioScore += amount;
    }
    if (_score > _highScore) {
        _highScore = _score;
    }
}

void ScoreManager::addCoins(int amount, const std::string& character) {
    if (character == "luigi") {
        _luigiCoins += amount;
        if (_luigiCoins >= 100) {
            _luigiCoins -= 100;
            _luigiLives++;
        }
    } else {
        _marioCoins += amount;
        if (_marioCoins >= 100) {
            _marioCoins -= 100;
            _marioLives++;
        }
    }
}

void ScoreManager::spawnFloatingText(sf::Vector2f position, const std::string& text) {
    _floatingTexts.push_back({ position, text, 1.0f, -40.0f });
}

std::string ScoreManager::getFormattedScore(const std::string& charName) const {
    const int val = (charName == "luigi") ? _luigiScore : (_marioScore > 0 ? _marioScore : _score);
    std::ostringstream ss;
    ss << std::setw(6) << std::setfill('0') << val;
    return ss.str();
}

std::string ScoreManager::getFormattedCoins(const std::string& charName) const {
    const int val = (charName == "luigi") ? _luigiCoins : _marioCoins;
    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << val;
    return ss.str();
}

std::string ScoreManager::getFormattedTime() const {
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << getIntTimeRemaining();
    return ss.str();
}

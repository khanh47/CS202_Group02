#include "Game/Objects/Block/CoinBlock.h"
#include <cmath>

#include "Audio/SoundManager.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Player/Player.h"
#include "Game/World/GameWorld.h"
#include "Game/ScoreManager.h"
#include "ResourceManager.h"

CoinBlock::CoinBlock() : Block() {
    // addBehaviour<Animatable>();
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(
            ResourceManager::getInstance().getTexture("brick_spritesheet"),
            "coin_block"
        );
    }
}

CoinBlock::CoinBlock(sf::Texture &texture) : Block() {
    // addBehaviour<Animatable>();
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "coin_block");
    }
}

CoinBlock::~CoinBlock() = default;

void CoinBlock::restoreCapacity(int value) noexcept {
    capacity = std::max(value, 0);
    if (capacity <= 0) {
        if (auto* animatable = getBehaviour<Animatable>()) {
            animatable->playAnimation("empty");
        }
    }
}

void CoinBlock::onCreateShapeDef(b2ShapeDef& def) {
    def.density = 10000.0f;
    def.material.friction = 0.0f;
    def.enableContactEvents = true;
}

void CoinBlock::onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) {
    if (tryBreakOnContact(other, contactData, ownShape)) {
        return;
    }

    if (_hitCooldown > 0.0f || capacity <= 0) {
        return;
    }

    if (auto* player = dynamic_cast<Player*>(&other)) {
        if (isBumped(other, contactData, ownShape)) {
            capacity--;
            _hitCooldown = 0.0f; // Cooldown to prevent multi-hits in 1 jump
            _bumpTimer = 0.15f;  // Trigger bump/enlarge animation (0.15 seconds)
            Audio::SoundManager::getInstance().playEffect("coin");

            // Spawn popping coin animation
            sf::Texture& itemsTexture = ResourceManager::getInstance().getTexture("coin_spritesheet");
            _bouncingCoin.spawn(getPosition(), itemsTexture);

            // 🪙 Trigger ScoreManager coin event (+200 pts, +1 coin, floating text)
            if (player->getGameWorld() && player->getGameWorld()->getScoreManager()) {
                player->getGameWorld()->getScoreManager()->handleEvent(
                    ScoreEventType::CoinCollected,
                    getPosition(),
                    0,
                    player->getCharacter()
                );
            }

            // If empty, switch animation to empty block
            auto* animatable = getBehaviour<Animatable>();
            if (capacity <= 0) {
                if(animatable)
                    animatable->playAnimation("empty");
            }
        }  
    }
}

void CoinBlock::onUpdateVisuals(float deltaTime) {
    if (_hitCooldown > 0.0f) {
        _hitCooldown -= deltaTime;
    }

    if (_bumpTimer > 0.0f) {
        _bumpTimer -= deltaTime;
        if (_bumpTimer < 0.0f) {
            _bumpTimer = 0.0f;
        }
    }

    _bouncingCoin.update(deltaTime);

    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(deltaTime, _hitboxPixels);
    }
}

void CoinBlock::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    sf::Vector2f renderPos = position;

    // Apply bump displacement and scale impulse when hit
    if (auto* animatable = getBehaviour<Animatable>()) {
        if (_bumpTimer > 0.0f) {
            float progress = 1.0f - (_bumpTimer / 0.15f); // 0.0 to 1.0
            float bumpOffset = -std::sin(progress * 3.14159f) * 14.0f; // Bumps upward by 14px
            float scale = 1.0f + std::sin(progress * 3.14159f) * 0.25f; // Scales up to 1.25x

            renderPos.y += bumpOffset;
            animatable->setVisualScale({scale, scale});
        } else {
            animatable->setVisualScale({1.0f, 1.0f});
        }

        animatable->renderVisualState(target, renderPos, angleDegrees);
    }

    // Render the coin popping out of the block
    _bouncingCoin.render(target);
}

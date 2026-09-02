#include "Game/Objects/Block/LuckyBlock.h"
#include <cmath>
#include <random>

#include "Audio/SoundManager.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Item/ConcreteItems/MegaMushroom.h"
#include "Game/Objects/Item/Item.h"
#include "Game/Objects/Player/Player.h"
#include "Game/World/GameWorld.h"
#include "Game/ScoreManager.h"
#include "ResourceManager.h"

LuckyBlock::LuckyBlock() : Block() {
    // addBehaviour<Animatable>();
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(
            ResourceManager::getInstance().getTexture("lucky_block_spritesheet"),
            "lucky_block"
        );
    }
    setupDefaultItemPool();
}

LuckyBlock::LuckyBlock(sf::Texture &texture) : Block() {
    // addBehaviour<Animatable>();
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(texture, "lucky_block");
    }
    setupDefaultItemPool();
}

LuckyBlock::~LuckyBlock() = default;

void LuckyBlock::restoreCapacity(int value) noexcept {
    capacity = std::max(value, 0);
    if (capacity <= 0) {
        _visualVisible = true;
        if (auto* animatable = getBehaviour<Animatable>()) {
            animatable->playAnimation("empty");
        }
    }
}

void LuckyBlock::configureTexture(
    sf::Texture& texture,
    bool useBrickAnimation
) {
    _visualVisible = true;
    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->configureVisuals(
            texture,
            useBrickAnimation ? "brick" : "lucky_block"
        );
    }
}

void LuckyBlock::setupDefaultItemPool() {
    clearOptions();
    // Default optional objects pool with weighted probability:
    addItemOption("Coin", 4.0f);
    addItemOption("SuperMushroom", 2.0f);
    addItemOption("OneUpMushroom", 1.0f);
    addItemOption("FireFlower", 2.0f);
    addItemOption("SuperStar", 1.0f);
}

void LuckyBlock::addItemOption(const std::string& itemTypeKey, float weight) {
    if (weight <= 0.0f) return;
    _itemOptions.push_back({itemTypeKey, weight, nullptr});
}

void LuckyBlock::addCustomOption(std::function<void(GameWorld&, sf::Vector2f)> spawner, float weight) {
    if (!spawner || weight <= 0.0f) return;
    _itemOptions.push_back({"", weight, std::move(spawner)});
}

void LuckyBlock::setItemPool(const std::vector<std::string>& itemTypeKeys) {
    clearOptions();
    for (const auto& key : itemTypeKeys) {
        addItemOption(key, 1.0f);
    }
}

void LuckyBlock::clearOptions() {
    _itemOptions.clear();
}

const ItemOption* LuckyBlock::selectRandomOption() const {
    if (_itemOptions.empty()) return nullptr;

    float totalWeight = 0.0f;
    for (const auto& opt : _itemOptions) {
        totalWeight += opt.weight;
    }
    if (totalWeight <= 0.0f) return nullptr;

    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, totalWeight);
    float roll = dist(rng);

    float current = 0.0f;
    for (const auto& opt : _itemOptions) {
        current += opt.weight;
        if (roll <= current) {
            return &opt;
        }
    }
    return &_itemOptions.back();
}

void LuckyBlock::onCreateShapeDef(b2ShapeDef& def) {
    def.density = 10000.0f;
    def.material.friction = 0.0f;
    def.enableContactEvents = true;
}

void LuckyBlock::onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) {
    if (tryBreakOnContact(other, contactData, ownShape)) {
        return;
    }

    if (auto* player = dynamic_cast<Player*>(&other)) {
        if (isBumped(other, contactData, ownShape)) {
            if (_hitCooldown > 0.0f || capacity <= 0)  return;

            capacity--;
            _hitCooldown = 0.0f; // Cooldown to prevent multi-hits in 1 jump
            _bumpTimer = 0.15f;  // Trigger bump/enlarge animation (0.15 seconds)

            GameWorld* world = player->getGameWorld();
            const ItemOption* chosenOption = selectRandomOption();

            sf::Vector2f itemSpawnPos = getPosition();
            if (chosenOption) {
                const sf::Vector2f itemSize = GameWorld::defaultItemSize(
                    chosenOption->itemTypeKey
                );
                // Keep the pickup's bottom edge at the block's top edge.
                // The minimum preserves the established offset for normal
                // 54x54 pickups while allowing large pickups to emerge fully.
                itemSpawnPos.y -= std::max(
                    64.0f,
                    32.0f + itemSize.y * 0.5f
                );
            }

            if (chosenOption) {
                if (chosenOption->customSpawner && world) {
                    chosenOption->customSpawner(*world, itemSpawnPos);
                } else if (chosenOption->itemTypeKey == "Coin" || chosenOption->itemTypeKey.empty()) {
                    Audio::SoundManager::getInstance().playEffect("coin");
                    // Spawn popping coin animation
                    sf::Texture& itemsTexture = ResourceManager::getInstance().getTexture("coin_spritesheet");
                    _bouncingCoin.spawn(getPosition(), itemsTexture);

                    // Trigger ScoreManager coin event (+200 pts, +1 coin, floating text)
                    if (world && world->getScoreManager()) {
                        world->getScoreManager()->handleEvent(
                            ScoreEventType::CoinCollected,
                            getPosition(),
                            0,
                            player ? player->getCharacter() : "mario"
                        );
                    }
                } else if (world) {
                    // Spawn the power-up inside the block and let it emerge
                    // to the normal above-block position.
                    const std::shared_ptr<GameObject> item = world->spawnItem(
                        chosenOption->itemTypeKey,
                        getPosition(),
                        GameWorld::defaultItemSize(
                            chosenOption->itemTypeKey
                        )
                    );
                    if (auto* powerup = dynamic_cast<Item*>(item.get())) {
                        if (dynamic_cast<MegaMushroom*>(powerup)) {
                            powerup->startEmerging(
                                itemSpawnPos,
                                MegaMushroom::emergenceDurationSeconds
                            );
                        } else {
                            powerup->startEmerging(itemSpawnPos);
                        }
                        _emergingPowerup = std::dynamic_pointer_cast<Item>(item);
                        Audio::SoundManager::getInstance().playEffect("sprout");
                    }
                }
            }

            // If empty, switch animation to empty block
            auto* animatable = getBehaviour<Animatable>();
            if (capacity <= 0) {
                // Invisible lucky blocks reveal their used texture once their
                // configured capacity has been exhausted.
                _visualVisible = true;
                if (animatable) {
                    animatable->playAnimation("empty");
                }
            }
        }
    }
}

void LuckyBlock::onUpdateVisuals(float deltaTime) {
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

    if (const std::shared_ptr<Item> powerup = _emergingPowerup.lock()) {
        sf::Vector2f renderOffset{};
        if (_bumpTimer > 0.0f) {
            const float progress = 1.0f - (_bumpTimer / 0.15f);
            renderOffset.y = -std::sin(progress * 3.14159f) * 14.0f;
        }
        powerup->setEmergenceRenderOffset(renderOffset);

        if (!powerup->isEmerging()) {
            _emergingPowerup.reset();
        }
    }

    if (auto* animatable = getBehaviour<Animatable>()) {
        animatable->updateVisualState(deltaTime, _hitboxPixels);
    }
}

void LuckyBlock::onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) {
    sf::Vector2f renderPos = position;

    // Apply bump displacement and scale impulse when hit
    if (_visualVisible) {
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
    }

    // Render the coin popping out of the block
    _bouncingCoin.render(target);
}

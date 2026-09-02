#pragma once

#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>
#include <box2d/box2d.h>
#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include "Game/Objects/GameObject.h"
#include "Physics/PhysicsWorld.h"
#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/Block/CoinBlock.h"

class GameWorld;
class Item;

/**
 * @brief Represents an outcome option when hitting a LuckyBlock.
 * Can be a standard item key ("Coin", "SuperMushroom", "FireFlower", "SuperStar"),
 * or a custom spawning callback function.
 */
struct ItemOption {
    std::string itemTypeKey;
    float weight = 1.0f;
    std::function<void(GameWorld&, sf::Vector2f)> customSpawner = nullptr;
};

class LuckyBlock: public Block {
public:
    LuckyBlock();
    LuckyBlock(sf::Texture &texture);
    ~LuckyBlock() override;

    void onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) override;
    bool isRenderedByTileMap() const noexcept override { return false; }

    /**
     * @brief Add a standard item type key to the random spawn pool.
     * @param itemTypeKey Registered item name (e.g., "Coin", "SuperMushroom", "FireFlower", "SuperStar")
     * @param weight Relative chance weight for this item (default 1.0)
     */
    void addItemOption(const std::string& itemTypeKey, float weight = 1.0f);

    /**
     * @brief Add a custom lambda/callback spawner to the random pool for maximum flexibility.
     * @param spawner Function taking (GameWorld&, sf::Vector2f spawnPosition)
     * @param weight Relative chance weight (default 1.0)
     */
    void addCustomOption(std::function<void(GameWorld&, sf::Vector2f)> spawner, float weight = 1.0f);

    /**
     * @brief Replace the current pool with a set of item type keys (equal weight).
     */
    void setItemPool(const std::vector<std::string>& itemTypeKeys);

    /**
     * @brief Clear all options from the item pool.
     */
    void clearOptions();

    void setCapacity(int value) noexcept {
        capacity = value > 0 ? value : 1;
    }
    int getCapacity() const noexcept { return capacity; }
    void restoreCapacity(int value) noexcept;

    void configureTexture(sf::Texture& texture, bool useBrickAnimation);
    void setVisualVisible(bool visible) noexcept { _visualVisible = visible; }
    bool isVisualVisible() const noexcept { return _visualVisible; }

protected:
    void onCreateShapeDef(b2ShapeDef& def) override;
    void onUpdateVisuals(float deltaTime) override;
    void onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees) override;

private:
    void setupDefaultItemPool();
    const ItemOption* selectRandomOption() const;

    int capacity = 1;          // Number of items the block can give
    float _hitCooldown = 0.0f;  // Cooldown time after being hit
    float _bumpTimer = 0.0f;    // Bumping/enlarging visual effect timer
    bool _visualVisible = true;
    BouncingCoin _bouncingCoin; // Animated coin popping out
    std::weak_ptr<Item> _emergingPowerup;
    std::vector<ItemOption> _itemOptions;
};

#include <stdexcept>

#include "Game/Objects/GameObjectFactory.h"
#include "Game/Objects/Block/Block.h"
#include "Game/Objects/Enemy/ConcreteEnemy/Goomba.h"
#include "Game/Objects/Enemy/ConcreteEnemy/Koopa.h"
#include "Game/Objects/Enemy/ConcreteEnemy/PiranhaPlant.h"
#include "Game/Objects/Item/Item.h"
#include "Game/Objects/Item/ConcreteItems/FireFlower.h"
#include "Game/Objects/Item/ConcreteItems/SuperMushroom.h"
#include "Game/Objects/Item/ConcreteItems/OneUpMushroom.h"
#include "Game/Objects/Item/ConcreteItems/MegaMushroom.h"
#include "Game/Objects/Item/ConcreteItems/SuperStar.h"
#include "Game/Objects/Player/Player.h"
#include "Game/Objects/Item/ConcreteItems/Coin.h"
#include "Game/Objects/Item/ConcreteItems/Flagpole.h"
#include "Game/Objects/Item/ConcreteItems/CheckpointFlag.h"
#include "Game/Objects/Item/ConcreteItems/MegaCoin.h"
#include "Game/Objects/Block/CoinBlock.h"
#include "Game/Objects/Block/LuckyBlock.h"
#include "Game/Objects/Block/SlopeBlock.h"
#include "Game/Objects/Pipe/Pipe.h"

GameObjectFactory::GameObjectFactory() {
    registerPlayer("Player", createAnimated<Player>);
    registerBlock("Block", createStatic<Block>);
    registerBlock("CoinBlock", createStatic<CoinBlock>);
    registerBlock("LuckyBlock", createStatic<LuckyBlock>);
    registerBlock("SlopeBlock", createStatic<SlopeBlock>);
    registerEnemy("Goomba", createAnimated<Goomba>);
    registerEnemy("Koopa", createAnimated<Koopa>);
    registerEnemy("PiranhaPlant", createAnimated<PiranhaPlant>);
    registerItem("Item", createStatic<Item>);
    registerItem("FireFlower", createStatic<FireFlower>);
    registerItem("SuperMushroom", createStatic<SuperMushroom>);
    registerItem("OneUpMushroom", createStatic<OneUpMushroom>);
    registerItem("MegaMushroom", createStatic<MegaMushroom>);
    registerItem("SuperStar", createStatic<SuperStar>);
    registerItem("Coin", createStatic<Coin>);
    registerItem("Flagpole", createStatic<Flagpole>);
    registerItem("CheckpointFlag", createStatic<CheckpointFlag>);
    registerItem("MegaCoin", createStatic<MegaCoin>);

    registerPipe("Pipe", [](sf::Texture* texture, const std::string& orientationStr,
                            const std::string& endSideStr, int bodyLength, bool isWarp,
                            int warpID, int warpTarget, const std::string& warpLevel)
        -> std::shared_ptr<GameObject> {
        // Parse orientation
        Pipe::Orientation orientation = Pipe::Orientation::Vertical;
        if (orientationStr == "horizontal") {
            orientation = Pipe::Orientation::Horizontal;
        }

        // Parse end side
        Pipe::EndSide endSide = Pipe::EndSide::Top;
        if (endSideStr == "bottom") endSide = Pipe::EndSide::Bottom;
        else if (endSideStr == "left") endSide = Pipe::EndSide::Left;
        else if (endSideStr == "right") endSide = Pipe::EndSide::Right;

        if (texture) {
            return std::make_shared<Pipe>(*texture, orientation, endSide, bodyLength, isWarp, warpID, warpTarget, warpLevel);
        }
        return std::make_shared<Pipe>();
    });
}

void GameObjectFactory::registerPlayer(const std::string& key, AnimatedCreator creator) {
    _playerCreators[key] = std::move(creator);
}

void GameObjectFactory::registerBlock(const std::string& key, Creator creator) {
    _blockCreators[key] = std::move(creator);
}

void GameObjectFactory::registerEnemy(const std::string& key, AnimatedCreator creator) {
    _enemyCreators[key] = std::move(creator);
}

void GameObjectFactory::registerItem(const std::string& key, Creator creator) {
    _itemCreators[key] = std::move(creator);
}

void GameObjectFactory::registerPipe(const std::string& key, PipeCreator creator) {
    _pipeCreators[key] = std::move(creator);
}

std::shared_ptr<GameObject> GameObjectFactory::createPlayer(const std::string& key, sf::Texture* texture, const std::string& animationSetId) const {
    const auto it = _playerCreators.find(key);
    if (it == _playerCreators.end()) {
        throw std::runtime_error("Unknown player type: " + key);
    }

    return it->second(texture, animationSetId);
}

std::shared_ptr<GameObject> GameObjectFactory::createBlock(const std::string& key, sf::Texture* texture) const {
    const auto it = _blockCreators.find(key);
    if (it == _blockCreators.end()) {
        throw std::runtime_error("Unknown block type: " + key);
    }

    return it->second(texture);
}

std::shared_ptr<GameObject> GameObjectFactory::createEnemy(const std::string& key, sf::Texture* texture, const std::string& animationSetId) const {
    const auto it = _enemyCreators.find(key);
    if (it == _enemyCreators.end()) {
        throw std::runtime_error("Unknown enemy type: " + key);
    }

    return it->second(texture, animationSetId);
}

std::shared_ptr<GameObject> GameObjectFactory::createItem(const std::string& key, sf::Texture* texture) const {
    const auto it = _itemCreators.find(key);
    if (it == _itemCreators.end()) {
        throw std::runtime_error("Unknown item type: " + key);
    }

    return it->second(texture);
}

std::shared_ptr<GameObject> GameObjectFactory::createPipe(
    const std::string& key, sf::Texture* texture,
    const std::string& orientation, const std::string& endSide,
    int bodyLength, bool isWarp, int warpID, int warpTarget,
    const std::string& warpLevel
) const {
    const auto it = _pipeCreators.find(key);
    if (it == _pipeCreators.end()) {
        throw std::runtime_error("Unknown pipe type: " + key);
    }

    return it->second(texture, orientation, endSide, bodyLength, isWarp, warpID, warpTarget, warpLevel);
}

#include "Game/World/WorldObjectStore.h"
#include "Game/AI/HeuristicAiController.h"
#include "Game/Objects/GameObject.h"
#include "Game/Objects/Player/Player.h"
#include "Game/UserInput/PlayerController.h"
#include <memory>

WorldObjectStore::WorldObjectStore() = default;
WorldObjectStore::~WorldObjectStore() = default;

void WorldObjectStore::clear() {
    _aiControllers.clear();
    _controllers.clear();
    _objects.clear();
}

void WorldObjectStore::addAiController(
    std::unique_ptr<HeuristicAiController> controller
) {
    if (controller) {
        _aiControllers.push_back(std::move(controller));
    }
}

void WorldObjectStore::addObject(std::shared_ptr<GameObject> object) {
    if (object) {
        _objects.push_back(std::move(object));
    }
}

void WorldObjectStore::removeObject(
    const std::shared_ptr<GameObject>& object
) {
    if (!object) {
        return;
    }

    std::erase_if(
        _objects,
        [&object](const std::shared_ptr<GameObject>& candidate) {
            return candidate == object;
        }
    );
}

void WorldObjectStore::addController(
    std::unique_ptr<PlayerController> controller
) {
    if (controller) {
        _controllers.push_back(std::move(controller));
    }
}

bool WorldObjectStore::handleInput(const sf::Event& event) {
    bool handled = false;
    for (const std::unique_ptr<PlayerController>& controller : _controllers) {
        if (controller) {
            handled = controller->handleEvent(event) || handled;
        }
    }
    return handled;
}

void WorldObjectStore::updateSimulation(float fixedDt) {
    for (const std::unique_ptr<HeuristicAiController>& controller
         : _aiControllers) {
        if (controller) {
            controller->fixedUpdate(fixedDt);
        }
    }

    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (object) {
            object->updateSimulation(fixedDt);

            // A pipe warp freezes the world as soon as the player starts it.
            // Stop dispatching this tick so later objects do not advance once
            // the transition has taken control of the simulation.
            if (const auto player = std::dynamic_pointer_cast<Player>(object);
                player && player->isPipeWarping()) {
                break;
            }
        }
    }
}

void WorldObjectStore::finalizeSimulation(float fixedDt) {
    finalizeGroundContacts();
    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (object) {
            object->finalizeSimulation(fixedDt);
        }
    }
}

void WorldObjectStore::updateVisuals(float deltaTime) {
    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (object) {
            object->updateVisuals(deltaTime);
        }
    }
}

void WorldObjectStore::cleanupDestroyed() {
    std::erase_if(
        _aiControllers,
        [](const std::unique_ptr<HeuristicAiController>& controller) {
            return !controller || controller->isPlayerEliminated();
        }
    );
    std::erase_if(_controllers, [](const std::unique_ptr<PlayerController>& controller) {
        return !controller || controller->isPlayerPendingDestroy();
    });
    std::erase_if(_objects, [](const std::shared_ptr<GameObject>& object) {
        return !object || object->isPendingDestroy();
    });
}

void WorldObjectStore::suspendPlayerMotion() {
    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (const auto player = std::dynamic_pointer_cast<Player>(object)) {
            player->stopMoveLeft();
            player->stopMoveRight();
            player->stopJump();
        }
    }
}

void WorldObjectStore::finalizeGroundContacts() {
    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (object) {
            object->finalizeGroundContacts();
        }
    }
}

void WorldObjectStore::syncControllersWithKeyboard() {
    for (const std::unique_ptr<PlayerController>& controller : _controllers) {
        if (controller) {
            controller->syncStateWithKeyboard();
        }
    }
}

std::shared_ptr<GameObject> WorldObjectStore::getPrimaryPlayer() const {
    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (std::dynamic_pointer_cast<Player>(object)) {
            return object;
        }
    }
    return nullptr;
}

bool WorldObjectStore::hasLivingPlayers() const {
    for (const std::shared_ptr<GameObject>& object : _objects) {
        const auto player = std::dynamic_pointer_cast<Player>(object);
        if (player) {
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<Player>> WorldObjectStore::getPlayers() const {
    std::vector<std::shared_ptr<Player>> players;
    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (auto player = std::dynamic_pointer_cast<Player>(object)) {
            players.push_back(std::move(player));
        }
    }
    return players;
}

std::vector<std::shared_ptr<Player>> WorldObjectStore::getLivingPlayers() const {
    std::vector<std::shared_ptr<Player>> players;
    for (const std::shared_ptr<GameObject>& object : _objects) {
        if (auto player = std::dynamic_pointer_cast<Player>(object);
            player && !player->isEliminated()) {
            players.push_back(std::move(player));
        }
    }
    return players;
}

#include "Scene/SceneManager.h"
#include "Scene/Scene.h"
#include "Scene/SceneFactory.h"
#include "ResourceManager.h"
#include <SFML/Graphics/RenderTarget.hpp>
#include <cmath>
#include <iostream>
#include <optional>
#include <vector>

namespace {
sf::Vector2i logicalMousePosition(
    const sf::RenderWindow& window,
    sf::Vector2i pixelPosition
) {
    const sf::Vector2f logicalPosition = window.mapPixelToCoords(
        pixelPosition,
        window.getDefaultView()
    );
    return {
        static_cast<int>(std::lround(logicalPosition.x)),
        static_cast<int>(std::lround(logicalPosition.y))
    };
}

std::optional<sf::Event> normalizeMouseEvent(
    const sf::RenderWindow& window,
    const sf::Event& event
) {
    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        return sf::Event(sf::Event::MouseMoved{
            logicalMousePosition(window, mouseMoved->position)
        });
    }

    if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        return sf::Event(sf::Event::MouseButtonPressed{
            mousePressed->button,
            logicalMousePosition(window, mousePressed->position)
        });
    }

    if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
        return sf::Event(sf::Event::MouseButtonReleased{
            mouseReleased->button,
            logicalMousePosition(window, mouseReleased->position)
        });
    }

    if (const auto* mouseWheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        return sf::Event(sf::Event::MouseWheelScrolled{
            mouseWheel->wheel,
            mouseWheel->delta,
            logicalMousePosition(window, mouseWheel->position)
        });
    }

    return std::nullopt;
}
}

SceneManager::SceneManager(SceneFactory *factory)
    : _factory(factory) {
    _gameWorld.loadLevel("assets/datas/levels/menu_map.json");
    _worldView = sf::View(sf::FloatRect(
        {0.0f, 0.0f},
        {static_cast<float>(_gameWorld.getGridWidth()) * _gameWorld.getCellSize(),
         static_cast<float>(_gameWorld.getGridHeight()) * _gameWorld.getCellSize()}
    ));
}

SceneManager::~SceneManager() {
    while (!isEmpty()) {
        auto &top = _sceneStack.top();
        top->onExit();
        top->cleanup();
        _sceneStack.pop();
    }
}

void SceneManager::pushScene(std::unique_ptr<Scene> scene) {
    if (!scene) return;

    scene->setSceneManager(this);
    if (!_sceneStack.empty()) {
        _sceneStack.top()->onExit();
    }
    _sceneStack.push(std::move(scene));
    _sceneStack.top()->init();
    _sceneStack.top()->onEnter();
    std::cout << "Pushed scene: " << _sceneStack.top()->getName() << std::endl;
}

void SceneManager::popScene() {
    if (!_sceneStack.empty()) {
        auto &top = _sceneStack.top();
        top->onExit();
        top->cleanup();
        _sceneStack.pop();
        if (!_sceneStack.empty()) {
            _sceneStack.top()->onEnter();
        }
    }
}

void SceneManager::replaceScene(std::unique_ptr<Scene> scene) {
    if (!scene) return;

    scene->setSceneManager(this);
    if (!_sceneStack.empty()) {
        auto &top = _sceneStack.top();
        top->onExit();
        top->cleanup();
        _sceneStack.pop();
    }
    _sceneStack.push(std::move(scene));
    _sceneStack.top()->init();
    _sceneStack.top()->onEnter();
    std::cout << "Replaced scene with: " << _sceneStack.top()->getName() << std::endl;
}

void SceneManager::pushSceneByName(const std::string &sceneName) {
    if (!_factory) {
        std::cerr << "Error: SceneFactory not set" << std::endl;
        return;
    }
    auto scene = _factory->createScene(sceneName);
    if (scene) {
        pushScene(std::move(scene));
    }
}

void SceneManager::requestPopScene() {
    requestDeferredAction([this]() {
        popScene();
    });
}

void SceneManager::requestReturnToModeMenu() {
    requestDeferredAction([this]() {
        while (!_sceneStack.empty()
               && _sceneStack.top()->getName() != "ModeSelectScene") {
            if (_sceneStack.size() == 1) {
                break;
            }
            popScene();
        }

        if (_sceneStack.empty()
            || _sceneStack.top()->getName() != "ModeSelectScene") {
            pushSceneByName("MODE_SELECT");
        }
    });
}

void SceneManager::requestReturnToMainMenu() {
    requestDeferredAction([this]() {
        while (!_sceneStack.empty()
               && _sceneStack.top()->getName() != "MainMenuScene") {
            if (_sceneStack.size() == 1) {
                break;
            }
            popScene();
        }

        if (_sceneStack.empty()
            || _sceneStack.top()->getName() != "MainMenuScene") {
            pushSceneByName("MAIN_MENU");
        }
    });
}

void SceneManager::requestReturnToMapEditor() {
    requestDeferredAction([this]() {
        while (!_sceneStack.empty()
               && _sceneStack.top()->getName() != "MapEditorScene") {
            if (_sceneStack.size() == 1) {
                break;
            }
            popScene();
        }

        if (_sceneStack.empty()
            || _sceneStack.top()->getName() != "MapEditorScene") {
            pushSceneByName("MAP_EDITOR");
        }
    });
}

void SceneManager::processEvents(const sf::Event& event) {
    if (!_sceneStack.empty()) {
        auto &currentScene = _sceneStack.top();
        if (currentScene->isActive()) {
            std::optional<sf::Event> normalizedEvent;
            if (_window != nullptr) {
                normalizedEvent = normalizeMouseEvent(*_window, event);
            }
            currentScene->handleInput(
                normalizedEvent.has_value() ? *normalizedEvent : event
            );
        }
    }
    processDeferredActions();
}

void SceneManager::updateSimulation(const float &fixedDt) {
    _gameWorld.updateSimulation(fixedDt);
    if (!_sceneStack.empty()) {
        auto &currentScene = _sceneStack.top();
        if (currentScene->isActive()) {
            currentScene->updateSimulation(fixedDt);
        }
    } else {
    }
    processDeferredActions();
}

void SceneManager::updateVisuals(float deltaTime) {
    _gameWorld.updateVisuals(deltaTime);
    _gameWorld.playVictoryAnimation();
    if (!_sceneStack.empty()) {
        auto &currentScene = _sceneStack.top();
        if (currentScene->isActive()) {
            currentScene->updateVisuals(deltaTime);
        }
    }
    processDeferredActions();
}

void SceneManager::renderGameWorld(sf::RenderTarget& target) {
    target.setView(_worldView);
    _gameWorld.render(target);
    target.setView(target.getDefaultView());
}

void SceneManager::render(sf::RenderTarget& target) {

    if (_sceneStack.empty()) {
        std::cerr << "Warning: No scene to render." << std::endl;
        processDeferredActions();
        return;
    }

    auto &currentScene = _sceneStack.top();

    if (currentScene->isActive()) {
        renderGameWorld(target);
        currentScene->render(target);
    }

    processDeferredActions();
}

Scene *SceneManager::getCurrentScene() const {
    if (!isEmpty()) {
        return _sceneStack.top().get();
    }
    return nullptr;
}

std::string SceneManager::getSceneName() const {
    if (isEmpty()) return "NONE";
    return getCurrentScene()->getName();
}

bool SceneManager::isEmpty() const {
    return _sceneStack.empty();
}

void SceneManager::processDeferredActions() {
    while (!_deferredActions.empty()) {
        _deferredActions.front()();
        _deferredActions.pop();
    }
}

void SceneManager::requestDeferredAction(std::function<void()> action) {
    if (action) {
        _deferredActions.push(std::move(action));
    }
}


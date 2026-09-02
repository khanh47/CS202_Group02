#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <functional>
#include <memory>
#include <queue>
#include <stack>
#include <string>
#include "Game/World/GameWorld.h"

// Forward declaration
class Scene;
class SceneFactory;

class SceneManager {
public:
    explicit SceneManager(SceneFactory *factory = nullptr);
    ~SceneManager();

    // Scene stack operations
    void pushScene(std::unique_ptr<Scene> scene);
    void popScene();
    void replaceScene(std::unique_ptr<Scene> scene);
    void requestPopScene();
    void requestReturnToModeMenu();
    void requestReturnToMainMenu();
    void requestReturnToMapEditor();

    // Main loop
    void processEvents(const sf::Event& event);
    void updateSimulation(const float &fixedDt);
    void updateVisuals(float deltaTime);
    void render(sf::RenderTarget& target);

    // Queries
    Scene *getCurrentScene() const;
    std::string getSceneName() const;
    bool isEmpty() const;

    // Scene factory
    void setSceneFactory(SceneFactory *factory) { _factory = factory; }
    void pushSceneByName(const std::string &sceneName);

    // Window access (for exit, etc.)
    void setRenderWindow(sf::RenderWindow* window) { _window = window; }
    sf::RenderWindow* getRenderWindow() const { return _window; }

private:
    std::stack<std::unique_ptr<Scene>> _sceneStack;
    std::queue<std::function<void()>> _deferredActions;
    SceneFactory *_factory;
    sf::RenderWindow* _window = nullptr;

    void requestDeferredAction(std::function<void()> action);
    void processDeferredActions();
    void renderGameWorld(sf::RenderTarget& target);

    GameWorld _gameWorld{30, 17};
    sf::View _worldView;
};

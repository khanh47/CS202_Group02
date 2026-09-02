#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "Game/World/FixedStepAccumulator.h"
#include "Scene/SceneManager.h"
#include "Scene/SceneFactory.h"

class App {
private:
    std::unique_ptr<SceneManager> manager;
    std::unique_ptr<SceneFactory> factory;
    sf::RenderWindow window;
    sf::Clock dtClock;
    FixedStepAccumulator stepAccumulator{1.0 / 60.0};
    void render();
    void updateSimulation();
    void updateVisuals(float deltaTime);
    void processEvents();

public:
    App();
    void run();
};

#include "App.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "Game/GameSettings.h"
#include "Game/Snapshot/SaveLoadGame.h"

App::App() : window(sf::VideoMode({1920, 1080}), "SUPER MARIO") {
#ifdef _WIN32
    ShowWindow((HWND)window.getNativeHandle(), SW_MAXIMIZE); //to maximize window
#endif
    factory = std::make_unique<SceneFactory>();
    manager = std::make_unique<SceneManager>(factory.get());
    manager->setRenderWindow(&window);
    window.setFramerateLimit(60);
    manager->pushSceneByName("MAIN_MENU");
}

void App::run() {
    dtClock.restart();
    while (window.isOpen()) {
        const float deltaTime = stepAccumulator.addFrame(
            dtClock.restart().asSeconds()
        );
        processEvents();
        updateSimulation();
        updateVisuals(deltaTime);
        render();
    }
    GameSettings::getInstance().save();
}

void App::processEvents() {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            if (SaveLoadGame::getInstance().hasUnsavedSession()
                && manager->getSceneName() != "ExitConfirmScene") {
                manager->pushSceneByName("EXIT_CONFIRM");
                continue;
            }
            window.close();
            continue;
        }
        manager->processEvents(*event);
    }
}

void App::updateSimulation() {
    stepAccumulator.consume([this](float fixedDt) {
        manager->updateSimulation(fixedDt);
    });
}

void App::updateVisuals(float deltaTime) {
    manager->updateVisuals(deltaTime);
}

void App::render() {
    window.clear(sf::Color(240, 240, 240));
    manager->render(window);
    window.display();
}

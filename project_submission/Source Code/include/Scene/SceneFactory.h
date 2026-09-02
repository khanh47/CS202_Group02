#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class Scene;

class SceneFactory {
private:
    std::unordered_map<std::string, std::function<std::unique_ptr<Scene>()>> _factories;

public:
    SceneFactory();

    void registerScene(const std::string& stateName, std::function<std::unique_ptr<Scene>()> factory);
    std::unique_ptr<Scene> createScene(const std::string& stateName);
};

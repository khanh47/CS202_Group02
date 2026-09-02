#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <string>

class SceneManager;

class Scene {
public:
    Scene();
    explicit Scene(const std::string& name);
    virtual ~Scene() = default;

    // Lifecycle
    virtual void init() {}
    virtual void onEnter();
    virtual void onExit() { _isActive = false; }
    virtual void cleanup() {}

    // Per-frame
    virtual void handleInput(const sf::Event& event) {}
    virtual void updateSimulation(const float& fixedDt) {}
    virtual void updateVisuals(float deltaTime) {}
    virtual void render(sf::RenderTarget& target) { renderBackground(target); }

    // Queries
    virtual std::string getName() const { return _name; }
    virtual bool isActive() const { return _isActive; }

    // Scene manager access (for scene transitions)
    void setSceneManager(SceneManager* manager) { _sceneManager = manager; }
    SceneManager* getSceneManager() const { return _sceneManager; }

protected:
    void setBackground(const std::string& textureAlias);
    void renderBackground(sf::RenderTarget& target);
    void playTitleScreenMusic();
    void stopTitleScreenMusic();

    bool _isActive = false;
    SceneManager* _sceneManager = nullptr;
    std::string _name;

private:
    std::optional<sf::Sprite> _backgroundSprite;
};

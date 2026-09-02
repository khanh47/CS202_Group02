#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Text.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "Game/Behaviours/Behaviour.h"
#include "Physics/PhysicsBody.h"
#include "Physics/PhysicsWorld.h"
#include "box2d/id.h"

class GameObject {
public:
    GameObject();
    virtual ~GameObject();

    virtual void updateSimulation(const float &fixedDt);
    virtual void finalizeSimulation(const float &fixedDt);
    virtual void updateVisuals(float deltaTime);
    virtual void render(sf::RenderTarget &target);

    virtual void spawn(const PhysicsWorld &physicsWorld, sf::Vector2f spawnPixels, sf::Vector2f hitboxPixels);
    virtual void destroy();
    
    bool isPendingDestroy() { return _pendingDestroy; }
    sf::Vector2f getPosition() const;
    virtual sf::Vector2f getVelocity() const;
    void setPosition(sf::Vector2f positionPixels);
    void setVelocity(sf::Vector2f velocityPixels);

    void setSaveId(std::string saveId) { _saveId = std::move(saveId); }
    const std::string& getSaveId() const noexcept { return _saveId; }

    sf::Vector2f getHitboxPixels() const { return _hitboxPixels; }
    std::shared_ptr<PhysicsBody> getPhysicsBody() const { return _body; }

    virtual void onContact(GameObject& other, const b2ContactData& contactData, b2ShapeId ownShape) {}
    virtual void finalizeGroundContacts() {}

    template<typename T, typename... Args>
    T* addBehaviour(Args&&... args) {
        static_assert(std::is_base_of_v<Behaviour, T>, "Behaviour must derive from Behaviour");

        auto behaviour = std::make_unique<T>(std::forward<Args>(args)...);
        T* behaviourPtr = behaviour.get();
        behaviourPtr->attach(*this);
        _behaviours.emplace_back(std::move(behaviour));
        return behaviourPtr;
    }

    template<typename T>
    T* getBehaviour() const {
        static_assert(std::is_base_of_v<Behaviour, T>, "Behaviour must derive from Behaviour");

        for (const auto& behaviour : _behaviours) {
            if (auto* typed = dynamic_cast<T*>(behaviour.get())) {
                return typed;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool removeBehaviour() {
        static_assert(std::is_base_of_v<Behaviour, T>, "Behaviour must derive from Behaviour");

        for (auto it = _behaviours.begin(); it != _behaviours.end(); ++it) {
            if (dynamic_cast<T*>(it->get()) != nullptr) {
                (*it)->detach();
                _behaviours.erase(it);
                return true;
            }
        }
        return false;
    }

protected:
    virtual void onCreateBodyDef(b2BodyDef& def);
    virtual void onCreateShapeDef(b2ShapeDef& def);
    virtual void onUpdateVisuals(float deltaTime);
    virtual void onRenderVisual(sf::RenderTarget& target, const sf::Vector2f& position, float angleDegrees);
    virtual void onRenderDebugHitbox(sf::RenderTarget& target) const;
    virtual void onHitboxRecreated();
    virtual b2Polygon makeHitbox(sf::Vector2f hitboxPixels) const;

    
    bool hasValidBody() const;
    sf::Vector2f getBodyPositionPixels() const;
    float getBodyAngleDegrees() const;
    void updateHitboxSize(sf::Vector2f newHitboxPixels);

    sf::Vector2f _hitboxPixels{0.f, 0.f};
    sf::Vector2f _baseHitboxPixels{0.f, 0.f};

    std::shared_ptr<PhysicsBody> _body = nullptr;
    bool _pendingDestroy = false;
    std::string _saveId;

    std::vector<std::unique_ptr<Behaviour>> _behaviours;

private:
    void createBody(const PhysicsWorld &physicsWorld, sf::Vector2f spawnPixels);
    void createHitbox(sf::Vector2f hitboxPixels);
    void drawFallbackRect(sf::RenderTarget& target) const; // debugging
};

#pragma once

#include <SFML/Graphics.hpp>
#include <box2d/box2d.h>

class GameObject;

class Behaviour {
public:
    virtual ~Behaviour() = default;

    GameObject* getOwner() const { return _owner; }
    bool hasOwner() const { return _owner != nullptr; }

    void attach(GameObject& owner) {
        _owner = &owner;
        onAttach();
    }

    void detach() {
        onDetach();
        _owner = nullptr;
    }

protected:
    virtual void onAttach() {}
    virtual void onDetach() {}
    
    GameObject& owner() const { return *_owner; }

private:
    GameObject* _owner = nullptr;
};

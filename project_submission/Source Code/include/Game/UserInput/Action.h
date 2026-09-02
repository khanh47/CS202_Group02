#include <SFML/Graphics.hpp>

#pragma once

using namespace std;

enum class ActionType {
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    Accelerate,
    Decelerate,
    Attack,
    Interact,
    ToggleFlyMode
};

class Action {
private: 

public: 
    Action() = default;
    virtual ~Action() = default;

    void PerformAction(const ActionType& action);

    virtual void MoveLeft() = 0;
    virtual void MoveRight() = 0;
    virtual void MoveUp() = 0;
    virtual void MoveDown() = 0;
    virtual void Accelerate() = 0;
    virtual void Decelerate() = 0;
    virtual void Shoot() = 0;
};
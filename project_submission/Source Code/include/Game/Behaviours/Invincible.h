#pragma once

#include "Game/Behaviours/Behaviour.h"
#include "Physics/CollisionFilter.h"
#include "Physics/PhysicsBody.h"
#include "box2d/id.h"
#include "box2d/types.h"

class Invincible: virtual public Behaviour {
public:
    Invincible();
    Invincible(float time): _time(time) {};
    ~Invincible() = default;

    void refreshCollisionMask();
    void updateSimulation(const float &fixedDt);

    float getTime() {
        return _time;
    }

    void onAttach() override;
    void onDetach() override;

private:
    float _time = 0.0f;
    b2Filter _savedFilter = {};
    bool _hasSavedFilter = false;
};

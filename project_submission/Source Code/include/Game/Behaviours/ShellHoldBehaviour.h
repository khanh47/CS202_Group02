#pragma once

#include "Game/Behaviours/Behaviour.h"

class KoopaShell;

/**
 * @brief Lets the owner player pick up a Koopa shell while holding the
 * Interact action, keep it in front of them, and throw it horizontally on
 * release. Removable via removeBehaviour<ShellHoldBehaviour>().
 */
class ShellHoldBehaviour : public Behaviour {
public:
    ShellHoldBehaviour() = default;
    ~ShellHoldBehaviour() override = default;

    void setInteractHeld(bool held);
    bool isInteractHeld() const { return _interactHeld; }
    bool isHoldingShell() const { return _heldShell != nullptr; }

    void updateSimulation(const float& fixedDt);
    void updateVisuals(float deltaTime);
    void releaseShell(bool throwAway);

protected:
    void onDetach() override;

private:
    void tryPickUpShell();
    void holdShell(KoopaShell* shell);

    bool _interactHeld = false;
    KoopaShell* _heldShell = nullptr;
};

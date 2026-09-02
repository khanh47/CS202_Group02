#include "Game/Behaviours/ShellHoldBehaviour.h"

#include <cmath>
#include <memory>

#include "Game/Behaviours/Animatable.h"
#include "Game/Objects/GameObject.h"
#include "Game/Objects/Player/Player.h"
#include "Game/Objects/Projectile/KoopaShell.h"
#include "Game/World/GameWorld.h"

namespace {
bool aabbOverlap(
    const sf::Vector2f& aPos,
    const sf::Vector2f& aSize,
    const sf::Vector2f& bPos,
    const sf::Vector2f& bSize
) {
    return std::abs(aPos.x - bPos.x) < (aSize.x + bSize.x) * 0.5f
        && std::abs(aPos.y - bPos.y) < (aSize.y + bSize.y) * 0.5f;
}
}

void ShellHoldBehaviour::updateSimulation(const float& fixedDt) {
    (void)fixedDt;

    auto* owner = getOwner();
    if (!owner || !owner->getPhysicsBody() || !owner->getPhysicsBody()->isValid()) {
        return;
    }

    if (_heldShell) {
        return;
    }

    if (_interactHeld) {
        tryPickUpShell();
    }
}

void ShellHoldBehaviour::updateVisuals(float deltaTime) {
    (void)deltaTime;

    if (!_heldShell) {
        return;
    }

    auto* owner = getOwner();
    if (!owner || !owner->getPhysicsBody() || !owner->getPhysicsBody()->isValid()) {
        releaseShell(false);
        return;
    }

    if (_heldShell->isPendingDestroy()
        || _heldShell->isDying()
        || !_heldShell->getPhysicsBody()
        || !_heldShell->getPhysicsBody()->isValid()) {
        releaseShell(false);
        return;
    }

    // Pin the shell to the player's post-physics head position so it never
    // lags behind their movement. Held slightly lower than the head top for
    // better visuals (shell floats in front of the player's body).
    const float offsetY =
        owner->getHitboxPixels().y * 0.5f
        + _heldShell->getHitboxPixels().y * 0.5f;
    const float lowerOffset = owner->getHitboxPixels().y * 0.15f;
    _heldShell->setPosition({
        owner->getPosition().x,
        owner->getPosition().y - offsetY + lowerOffset
    });

    // Mirror the player's facing so the held shell flips with them.
    if (auto* player = dynamic_cast<Player*>(owner)) {
        _heldShell->setFacingRight(!player->isFacingLeft());
    }
}

void ShellHoldBehaviour::tryPickUpShell() {
    auto* owner = getOwner();
    auto* player = dynamic_cast<Player*>(owner);
    if (!player) {
        return;
    }

    GameWorld* world = player->getGameWorld();
    if (!world) {
        return;
    }

    const sf::Vector2f playerPos = owner->getPosition();
    const sf::Vector2f playerSize = owner->getHitboxPixels();

    KoopaShell* best = nullptr;
    float bestDistance = 0.0f;
    for (const std::shared_ptr<GameObject>& object : world->objects()) {
        if (!object) {
            continue;
        }

        auto* shell = dynamic_cast<KoopaShell*>(object.get());
        if (!shell
            || shell->isHeld()
            || shell->isSliding()
            || shell->isDying()
            || shell->isPendingDestroy()
            || !shell->getPhysicsBody()
            || !shell->getPhysicsBody()->isValid()) {
            continue;
        }

        const sf::Vector2f shellPos = shell->getPosition();
        if (!aabbOverlap(playerPos, playerSize, shellPos, shell->getHitboxPixels())) {
            continue;
        }

        const float dx = shellPos.x - playerPos.x;
        const float dy = shellPos.y - playerPos.y;
        const float distance = dx * dx + dy * dy;
        if (!best || distance < bestDistance) {
            best = shell;
            bestDistance = distance;
        }
    }

    if (best) {
        holdShell(best);
    }
}

void ShellHoldBehaviour::holdShell(KoopaShell* shell) {
    _heldShell = shell;
    _heldShell->setHeld(true);
    _heldShell->resetReviveTimer();
    _heldShell->stop();
}

void ShellHoldBehaviour::releaseShell(bool throwAway) {
    if (!_heldShell) {
        return;
    }

    KoopaShell* shell = _heldShell;
    _heldShell = nullptr;

    if (!shell->isPendingDestroy()
        && !shell->isDying()
        && shell->getPhysicsBody()
        && shell->getPhysicsBody()->isValid()) {
        shell->setHeld(false);
        if (throwAway) {
            auto* player = dynamic_cast<Player*>(getOwner());
            const bool facingRight = !(player && player->isFacingLeft());
            if (player) {
                // Offset the throw ahead of the player so the sliding shell
                // never spawns overlapping their body.
                const float facing = facingRight ? 1.0f : -1.0f;
                const float clearDistance =
                    player->getHitboxPixels().x * 0.5f
                    + shell->getHitboxPixels().x * 0.5f
                    + 5.0f;
                const sf::Vector2f pos = shell->getPosition();
                shell->setPosition({pos.x + facing * clearDistance, pos.y});
            }
            shell->kick(facingRight);
            if (player) {
                if (auto* animatable = player->getBehaviour<Animatable>()) {
                    animatable->playAnimation("throw", true);
                }
            }
        }
    }
}

void ShellHoldBehaviour::setInteractHeld(bool held) {
    if (_interactHeld == held) {
        return;
    }
    _interactHeld = held;
    if (!held) {
        releaseShell(true);
    }
}

void ShellHoldBehaviour::onDetach() {
    releaseShell(false);
}

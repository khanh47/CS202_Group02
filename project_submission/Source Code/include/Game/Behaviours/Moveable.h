#pragma once

#include "Game/Behaviours/Behaviour.h"
#include "Game/Behaviours/GroundTracker.h"

class Moveable : public Behaviour {
public:
    void startMoveLeft() { _movingLeft = true; _facingLeft = true; }
    void stopMoveLeft() { _movingLeft = false; }
    void startMoveRight() { _movingRight = true; _facingLeft = false; }
    void stopMoveRight() { _movingRight = false; } 
    void startJump() { if (_groundTracker.canJump()) _jumping = true; }
    void stopJump() { _jumping = false; }

    void beginGroundContact() {
        _groundTracker.beginSupport();
    }

    void beginGroundContact(b2ShapeId visitor) {
        _groundTracker.beginSupport(visitor);
    }

    void endGroundContact() {
        _groundTracker.endSupport();
    }

    void endGroundContact(b2ShapeId visitor) {
        _groundTracker.endSupport(visitor);
    }

    void resetGroundContacts() {
        _groundTracker.reset();
    }

    void finalizeGroundContacts() {
        _groundTracker.finalizeStep();
    }
    
    bool isMovingRight() const { return _movingRight; }
    bool isMovingLeft() const { return _movingLeft; }
    bool isJumping() const { return _jumping; }

    bool isFacingLeft() const { return _facingLeft; }
    void setFacingLeft(bool facingLeft) { _facingLeft = facingLeft; }
    bool isAirbone() const {
        return !_groundTracker.isGrounded();
    }

    bool hasGroundSupport() const noexcept {
        return _groundTracker.hasGroundSupport();
    }

    void consumeGroundForJump() {
        _groundTracker.consumeForJump();
    }

private:
    bool _facingLeft = false;
    bool _movingLeft = false;
    bool _movingRight = false;
    bool _jumping = false;
    GroundTracker _groundTracker;
};

#pragma once

#include <algorithm>
#include <vector>

#include <box2d/id.h>

class GroundTracker {
public:
    void beginSupport() {
        ++_anonymousSupportCount;
        confirmSupport();
    }

    void beginSupport(b2ShapeId visitor) {
        const auto existing = std::find_if(
            _supportVisitors.begin(),
            _supportVisitors.end(),
            [visitor](b2ShapeId current) {
                return B2_ID_EQUALS(current, visitor);
            }
        );
        if (existing == _supportVisitors.end()) {
            _supportVisitors.push_back(visitor);
            confirmSupport();
        }
    }

    void endSupport() {
        if (_anonymousSupportCount > 0) {
            --_anonymousSupportCount;
        }
    }

    void endSupport(b2ShapeId visitor) {
        const auto existing = std::find_if(
            _supportVisitors.begin(),
            _supportVisitors.end(),
            [visitor](b2ShapeId current) {
                return B2_ID_EQUALS(current, visitor);
            }
        );
        if (existing != _supportVisitors.end()) {
            _supportVisitors.erase(existing);
        }
    }

    void finalizeStep() {
        if (hasSupport()) {
            confirmSupport();
            return;
        }

        if (!_grounded) {
            _jumpAvailable = false;
            return;
        }

        ++_unsupportedStepCount;
        if (_unsupportedStepCount >= unsupportedStepsBeforeAirborne) {
            _grounded = false;
            _jumpAvailable = false;
        }
    }

    void consumeForJump() {
        // Box2D can retain the old support until the next step. Forget it now
        // so an applied jump cannot immediately re-ground itself.
        _anonymousSupportCount = 0;
        _supportVisitors.clear();
        _unsupportedStepCount = unsupportedStepsBeforeAirborne;
        _grounded = false;
        _jumpAvailable = false;
    }

    void reset() {
        _anonymousSupportCount = 0;
        _supportVisitors.clear();
        _unsupportedStepCount = 0;
        _grounded = false;
        _jumpAvailable = false;
    }

    bool isGrounded() const noexcept {
        return _grounded;
    }

    bool canJump() const noexcept {
        return _jumpAvailable;
    }

    bool hasGroundSupport() const noexcept {
        return hasSupport();
    }

private:
    static constexpr int unsupportedStepsBeforeAirborne = 5;

    bool hasSupport() const noexcept {
        return _anonymousSupportCount > 0 || !_supportVisitors.empty();
    }

    void confirmSupport() {
        _unsupportedStepCount = 0;
        _grounded = true;
        _jumpAvailable = true;
    }

    int _anonymousSupportCount = 0;
    int _unsupportedStepCount = 0;
    bool _grounded = false;
    bool _jumpAvailable = false;
    std::vector<b2ShapeId> _supportVisitors;
};

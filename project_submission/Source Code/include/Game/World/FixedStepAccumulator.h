#pragma once

#include <algorithm>
#include <functional>

class FixedStepAccumulator {
public:
    explicit FixedStepAccumulator(double fixedStepSeconds)
        : _fixedStepSeconds(fixedStepSeconds) {}

    float addFrame(float frameSeconds) {
        const float clamped = std::clamp(frameSeconds, 0.0f, 0.25f);
        _accumulatedSeconds += clamped;
        return clamped;
    }

    template <typename Update>
    int consume(Update&& update) {
        int steps = 0;
        while (_accumulatedSeconds >= _fixedStepSeconds) {
            std::invoke(update, static_cast<float>(_fixedStepSeconds));
            _accumulatedSeconds -= _fixedStepSeconds;
            ++steps;
        }
        return steps;
    }

    double remainder() const noexcept { return _accumulatedSeconds; }

private:
    double _fixedStepSeconds;
    double _accumulatedSeconds = 0.0;
};

#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <cstddef>
#include <vector>

#include "Animation/AnimationFrame.h"

class AnimationClip {
public:
    AnimationClip() = default;
    AnimationClip(std::vector<AnimationFrame> frames, bool looping);
    ~AnimationClip() = default;

    const AnimationFrame& getFrame(std::size_t id) const { return frames[id]; }
    std::size_t getFrameCount() const { return frames.size(); }
    bool isEmpty() const { return frames.empty(); }
    bool isLooping() const { return looping; }

private:
    std::vector<AnimationFrame> frames;
    bool looping = true;
};


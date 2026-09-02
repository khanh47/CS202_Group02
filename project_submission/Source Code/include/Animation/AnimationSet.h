#pragma once

#include "Animation/AnimationClip.h"

#include <unordered_map>
#include <string>

struct AnimationSet {
    std::unordered_map<std::string, AnimationClip> clips;
    std::string defaultClip = "idle";
};
#pragma once

#include <string>

#include "Animation/AnimationSet.h"

class AnimationLibrary {
public:
    static AnimationLibrary& getInstance();

    const AnimationSet& getAnimationSet(const std::string& name) const;
    const AnimationSet& getPlayerAnimationSet(const std::string& name) const;

private:
    AnimationLibrary();

    void registerAnimationSet(const std::string& name, AnimationSet animationSet);
    void preloadPlayerAnimationSets();
    void preloadEnemyAnimationSets();
    void preloadItemAnimationSets();

    std::unordered_map<std::string, AnimationSet> _animationSets;
};

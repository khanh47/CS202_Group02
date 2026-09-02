#pragma once

#include "Animation/AnimationClip.h"
#include "Animation/AnimationSet.h"

namespace Animation {
    void advanceBrickAnimationClock(float deltaTime);
    sf::IntRect getBrickAnimationFrameRect();

    AnimationClip createLinearClip(
        sf::Vector2i startPosition,
        sf::Vector2i frameSize,
        int frameCount,
        sf::Vector2i frameStride,
        float frameDuration,
        bool looping = true
    );

    AnimationSet makeDefaultPlayerAnimationSet();
    AnimationSet makeGoombaAnimationSet();
    AnimationSet makeKoopaAnimationSet();
    AnimationSet makePiranhaPlantAnimationSet();
    AnimationSet makeFireFlowerAnimationSet();
    AnimationSet makeFireballAnimationSet();
    AnimationSet makeTransformAnimationSet();
    AnimationSet makeSuperMushroomAnimationSet();
    AnimationSet makeOneUpMushroomAnimationSet();
    AnimationSet makeMegaMushroomAnimationSet();
    AnimationSet makeSuperStarAnimationSet();
    AnimationSet makeCoinAnimationSet();
    AnimationSet makeBrickAnimationSet();
    AnimationSet makeCoinBlockAnimationSet();
    AnimationSet makeLuckyBlockAnimationSet();
    AnimationSet makeFlagpoleAnimationSet();
    AnimationSet makeCheckpointFlagAnimationSet();
    AnimationSet makeMegaCoinAnimationSet();
}

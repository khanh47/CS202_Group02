#include "Animation/Animation.h"

#include <cstddef>

namespace {
// Two frames per second keeps the brick shimmer readable while preserving
// the shared phase used by static bricks and live coin blocks.
constexpr float BrickAnimationFrameDuration = 1.0f / 3.0f;
constexpr std::size_t BrickAnimationFrameCount = 4;
constexpr int BrickAnimationFrameStride = 72;
constexpr int BrickAnimationFrameSize = 64;

std::size_t brickAnimationFrame = 0;
float brickAnimationFrameElapsed = 0.0f;
}

void Animation::advanceBrickAnimationClock(float deltaTime) {
    if (deltaTime <= 0.0f) {
        return;
    }

    brickAnimationFrameElapsed += deltaTime;
    while (brickAnimationFrameElapsed >= BrickAnimationFrameDuration) {
        brickAnimationFrameElapsed -= BrickAnimationFrameDuration;
        brickAnimationFrame =
            (brickAnimationFrame + 1) % BrickAnimationFrameCount;
    }
}

sf::IntRect Animation::getBrickAnimationFrameRect() {
    return sf::IntRect(
        {
            static_cast<int>(brickAnimationFrame * BrickAnimationFrameStride),
            0
        },
        {BrickAnimationFrameSize, BrickAnimationFrameSize}
    );
}

AnimationClip Animation::createLinearClip(
    sf::Vector2i startPosition,
    sf::Vector2i frameSize,
    int frameCount,
    sf::Vector2i frameStride,
    float frameDuration,
    bool looping
) {
    std::vector<AnimationFrame> frames;
    frames.reserve(frameCount);

    for (int i = 0; i < frameCount; i++) {
        sf::IntRect pos = sf::IntRect(
            {startPosition.x + i * frameStride.x, startPosition.y + i * frameStride.y},
            {frameSize.x, frameSize.y}
        );

        frames.push_back(AnimationFrame(pos, frameDuration));
    }

    return AnimationClip(frames, looping);
}


AnimationSet Animation::makeDefaultPlayerAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "idle";

    animationSet.clips.emplace(
        "idle",
        Animation::createLinearClip(
            {48, 32},
            {32, 32},
            4,
            {32, 0},
            1.0f / 4.0f,
            true
        )
    );

    animationSet.clips.emplace(
        "walk",
        Animation::createLinearClip(
            {48, 80},
            {32, 32},
            6,
            {32, 0},
            1.0f / 6.0f,
            true
        )
    );

    animationSet.clips.emplace(
        "jump",
        Animation::createLinearClip(
            {48, 128},
            {32, 32},
            3,
            {32, 0},
            1.0f / 3.0f,
            true
        )
    );
//
    animationSet.clips.emplace(
        "bump",
        Animation::createLinearClip(
            {176, 128},
            {32, 32},
            2,
            {32, 0},
            1.0f / 6.0f,
            false
        )
    );

    // animationSet.clips.emplace(
    //     "shoot",
    //     Animation::createLinearClip(
    //         {288, 352},
    //         {32, 32},
    //         3,
    //         {32, 0},
    //         1.0f / 5.0f,
    //         false
    //     )
    // );

    animationSet.clips.emplace(
        "shoot",
        Animation::createLinearClip(
            {288, 128},
            {48, 32},
            3,
            {48, 0},
            1.0f / 10.0f,
            false
        )
    );

    animationSet.clips.emplace(
        "air_shot",
        Animation::createLinearClip(
            {288, 400},
            {32, 32},
            3,
            {32, 0},
            1.0f / 10.0f,
            false
        )
    );

    animationSet.clips.emplace(
        "hit",
        Animation::createLinearClip(
            {288, 352},
            {32, 32},
            3,
            {32, 0},
            1.0f / 3.0f,
            false
        )
    );

    animationSet.clips.emplace(
        "knockout",
        Animation::createLinearClip(
            {48, 512},
            {32, 32},
            9,
            {32, 0},
            1.0f / 7.0f,
            false
        )
    );

    animationSet.clips.emplace(
        "hold_stand",
        Animation::createLinearClip(
            {144, 176},
            {32, 32},
            1,
            {0, 0},
            1.0f,
            true 
        )
    );

    animationSet.clips.emplace(
        "hold_walk",
        Animation::createLinearClip(
            {48, 224},
            {32, 32},
            6,
            {32, 0},
            1.0f / 6.0f,
            true
        )
    );

    animationSet.clips.emplace(
        "throw",
        Animation::createLinearClip(
            {55, 272},
            {32, 32},
            2,
            {32, 0},
            0.5f / 2.0f,
            false
        )
    );

    animationSet.clips.emplace(
        "victory",
        Animation::createLinearClip(
            {48, 320},   
            {32, 32},   
            4,          
            {32, 0},     
            1 / 3.0f,       
            false 
        )
    );

    animationSet.clips.emplace(
        "lose",
        Animation::createLinearClip(
            {112, 368},   
            {32, 32},   
            2,          
            {32, 0},     
            1 / 3.0f,       
            false 
        )
    );

    return animationSet;
}

AnimationSet Animation::makeGoombaAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "walk";

    animationSet.clips.emplace(
        "walk",
        Animation::createLinearClip(
            {2, 42},
            {16, 19},
            4,       
            {17, 0}, 
            1.0f / 4.0f,
            true        
        )
    );

    animationSet.clips.emplace(
        "dead",
        Animation::createLinearClip(
            {2, 302},
            {17, 18},
            1,       
            {0, 0}, 
            1.0f,
            false 
        )
    );

    animationSet.clips.emplace(
        "stomped",
        Animation::createLinearClip(
            {0, 730},   
            {32, 16},   
            1,          
            {0, 0},     
            1.0f,       
            false       
        )
    );

    return animationSet;
}

AnimationSet Animation::makePiranhaPlantAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "bite";

    animationSet.clips.emplace(
        "bite",
        Animation::createLinearClip(
            {7, 12},
            {26, 35},
            3,       
            {33, 0}, 
            1.0f / 3.0f,
            true        
        )
    );

    return animationSet;
}

AnimationSet Animation::makeKoopaAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "walk";

    animationSet.clips.emplace(
        "walk",
        Animation::createLinearClip(
            {6, 32},
            {28, 32},
            8,        
            {28, 0},
            1.0f / 8.0f,
            true         
        )
    );

    animationSet.clips.emplace(
        "dead",
        Animation::createLinearClip(
            {12, 228},
            {19, 15},
            1,          
            {0, 0},      
            1.0f,
            false 
        )
    );

    animationSet.clips.emplace(
        "slide",
        Animation::createLinearClip(
            {10, 159},
            {19, 15},
            8,          
            {24, 0},
            1.0f / 8.0f,        
            true 
        )
    );

    animationSet.clips.emplace(
        "shake",
        Animation::createLinearClip(
            {100, 90},
            {19, 16},
            6,          
            {24, 0},
            0.5f / 6.0f,        
            true 
        )
    );

    animationSet.clips.emplace(
        "revive",
        Animation::createLinearClip(
            {6, 211},
            {28, 32},
            8,          
            {28, 0},
            2.0f / 8.0f,        
            false 
        )
    );

    return animationSet;
}

AnimationSet Animation::makeFireFlowerAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "idle";

    animationSet.clips.emplace(
        "idle",
        Animation::createLinearClip(
            {0, 109},
            {18, 17},
            4,
            {18, 0},
            1.0f / 6.0f,
            true
        )
    );

    return animationSet;
}

AnimationSet Animation::makeFireballAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "spin";

    const float frameDuration = 1.0f / 8.0f;
    std::vector<AnimationFrame> frames = {
        AnimationFrame(sf::IntRect({4, 148}, {8, 10}), frameDuration),
        AnimationFrame(sf::IntRect({23, 148}, {8, 10}), frameDuration),
        AnimationFrame(sf::IntRect({42, 148}, {8, 10}), frameDuration),
        AnimationFrame(sf::IntRect({59, 148}, {8, 10}), frameDuration)
    };

    animationSet.clips.emplace("spin", AnimationClip(frames, true));

    return animationSet;
}

AnimationSet Animation::makeTransformAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "transform";

    const float dt = 0.08f;
    // Rapidly alternate between transformation frames (Normal -> Intermediate -> Fire)
    std::vector<AnimationFrame> frames = {
        AnimationFrame(sf::IntRect({0, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({32, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({64, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({32, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({0, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({32, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({64, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({32, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({0, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({32, 0}, {32, 32}), dt),
        AnimationFrame(sf::IntRect({64, 0}, {32, 32}), dt)
    };

    animationSet.clips.emplace("transform", AnimationClip(frames, false));
    return animationSet;
}

AnimationSet Animation::makeSuperMushroomAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "idle";

    animationSet.clips.emplace(
        "idle",
        Animation::createLinearClip(
            {0, 90},
            {18, 18},
            2,
            {18, 0},
            1.0f,
            true
        )
    );

    return animationSet;
}

AnimationSet Animation::makeOneUpMushroomAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "idle";

    animationSet.clips.emplace(
        "idle",
        Animation::createLinearClip(
            {36, 90},
            {18, 18},
            2,
            {18, 0},
            1.0f,
            true
        )
    );

    return animationSet;
}

AnimationSet Animation::makeMegaMushroomAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "idle";

    // The Mega Mushroom asset is a single, large frame rather than a strip.
    animationSet.clips.emplace(
        "idle",
        Animation::createLinearClip(
            {0, 0},
            {1402, 1122},
            1,
            {0, 0},
            1.0f,
            true
        )
    );

    return animationSet;
}

AnimationSet Animation::makeSuperStarAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "idle";

    animationSet.clips.emplace(
        "idle",
        Animation::createLinearClip(
            {0, 126},
            {18, 18},
            4,
            {18, 0},
            1.0f / 6.0f,
            true
        )
    );

    return animationSet;
}

AnimationSet Animation::makeCoinAnimationSet() {
    AnimationSet set;
    set.defaultClip = "idle";
    
    set.clips.emplace(
        "idle", 
        Animation::createLinearClip (
            {0, 0},
            {64, 64},
            4, 
            {72, 0},
            1.0f / 6.0f,
            true
        )
    );

    return set;
}

AnimationSet Animation::makeBrickAnimationSet() {
    AnimationSet set;
    set.defaultClip = "shining";

    set.clips.emplace(
        "shining",
        Animation::createLinearClip(
            {0, 0},
            {64, 64},
            4,
            {72, 0},
            BrickAnimationFrameDuration,
            true
        )
    );

    set.clips.emplace(
        "empty",
        Animation::createLinearClip(
            {288, 0},
            {64, 64},
            1,
            {0, 0},
            1.0f,
            false
        )
    );

    return set;
}

AnimationSet Animation::makeCoinBlockAnimationSet() {
    return makeBrickAnimationSet();
}

AnimationSet Animation::makeLuckyBlockAnimationSet() {
    AnimationSet set;
    set.defaultClip = "shining";
    
    set.clips.emplace(
        "shining", 
        Animation::createLinearClip (
            {0, 0},
            {64, 64},
            4, 
            {72, 0},
            1.0f / 4.0f,
            true
        )
    );

    set.clips.emplace(
        "empty",
        Animation::createLinearClip(
            {288, 0},
            {64, 64},
            1,
            {0, 0},
            1.0f,
            false
        )
    );

    return set;
}

AnimationSet Animation::makeFlagpoleAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "idle";

    animationSet.clips.emplace(
        "idle",
        Animation::createLinearClip(
            {217, 0},
            {29, 168},
            3,
            {29, 0},
            1.0f / 3,
            true
        )
    );

    return animationSet;
}

AnimationSet Animation::makeCheckpointFlagAnimationSet() {
    AnimationSet animationSet;
    animationSet.defaultClip = "waving";

    animationSet.clips.emplace(
        "captured",
        Animation::createLinearClip(
            {33, 165},
            {32, 32},
            4,
            {33, 0},
            1.0f / 4,
            true
        )
    );

    animationSet.clips.emplace(
        "waving",
        Animation::createLinearClip(
            {0, 132},
            {32, 32},
            4,
            {33, 0},
            1.0f / 4,
            true
        )
    );

    return animationSet;
}

AnimationSet Animation::makeMegaCoinAnimationSet() {
    AnimationSet set;
    set.defaultClip = "spin";
    
    set.clips.emplace(
        "spin",
        Animation::createLinearClip(
            {0, 0},
            {36, 36},
            4,
            {36, 0},
            1.0f / 6.0f,
            true
        )
    );

    return set;
}

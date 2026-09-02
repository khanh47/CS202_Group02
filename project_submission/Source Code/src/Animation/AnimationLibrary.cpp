#include "Animation/AnimationLibrary.h"
#include "Animation/AnimationSet.h"
#include "Animation/Animation.h"

#include <stdexcept>
#include <utility>

AnimationLibrary& AnimationLibrary::getInstance() {
    static AnimationLibrary instance;
    return instance;
}

AnimationLibrary::AnimationLibrary() {
    preloadPlayerAnimationSets();
    preloadEnemyAnimationSets();
    preloadItemAnimationSets();
}

void AnimationLibrary::registerAnimationSet(const std::string& name, AnimationSet animationSet) {
    _animationSets.emplace(name, std::move(animationSet));
}

void AnimationLibrary::preloadPlayerAnimationSets() {
    const AnimationSet playerSet = Animation::makeDefaultPlayerAnimationSet();
    registerAnimationSet("mario", playerSet);
    registerAnimationSet("fire_mario", playerSet);
    registerAnimationSet("fire_luigi", playerSet);
    registerAnimationSet("luigi", playerSet);

    const AnimationSet transformSet = Animation::makeTransformAnimationSet();
    registerAnimationSet("transform", transformSet);
}

void AnimationLibrary::preloadEnemyAnimationSets() {
    const AnimationSet goombaSet = Animation::makeGoombaAnimationSet();
    registerAnimationSet("goomba", goombaSet);

    const AnimationSet koopaSet = Animation::makeKoopaAnimationSet();
    registerAnimationSet("koopa", koopaSet);

    const AnimationSet piranhaPlantSet = Animation::makePiranhaPlantAnimationSet();
    registerAnimationSet("piranha_plant", piranhaPlantSet);
}

void AnimationLibrary::preloadItemAnimationSets() {
    const AnimationSet fireFlowerSet = Animation::makeFireFlowerAnimationSet();
    registerAnimationSet("fire_flower", fireFlowerSet);

    const AnimationSet fireballSet = Animation::makeFireballAnimationSet();
    registerAnimationSet("fireball", fireballSet);

    const AnimationSet superMushroomSet = Animation::makeSuperMushroomAnimationSet();
    registerAnimationSet("super_mushroom", superMushroomSet);

    const AnimationSet oneUpMushroomSet = Animation::makeOneUpMushroomAnimationSet();
    registerAnimationSet("one_up_mushroom", oneUpMushroomSet);

    const AnimationSet megaMushroomSet = Animation::makeMegaMushroomAnimationSet();
    registerAnimationSet("mega_mushroom", megaMushroomSet);

    const AnimationSet superStarSet = Animation::makeSuperStarAnimationSet();
    registerAnimationSet("super_star", superStarSet);

    const AnimationSet coinSet = Animation::makeCoinAnimationSet();
    registerAnimationSet("coin", coinSet);

    const AnimationSet brickSet = Animation::makeBrickAnimationSet();
    registerAnimationSet("brick", brickSet);

    const AnimationSet coinBlockSet = Animation::makeCoinBlockAnimationSet();
    registerAnimationSet("coin_block", coinBlockSet);

    const AnimationSet luckyBlockSet = Animation::makeLuckyBlockAnimationSet();
    registerAnimationSet("lucky_block", luckyBlockSet);

    const AnimationSet flagpoleSet = Animation::makeFlagpoleAnimationSet();
    registerAnimationSet("flagpole", flagpoleSet);

    const AnimationSet checkpointFlagSet = Animation::makeCheckpointFlagAnimationSet();
    registerAnimationSet("checkpoint_flag", checkpointFlagSet);

    const AnimationSet megaCoinSet = Animation::makeMegaCoinAnimationSet();
    registerAnimationSet("mega_coin", megaCoinSet);
}

const AnimationSet& AnimationLibrary::getAnimationSet(const std::string& name) const {
    const auto it = _animationSets.find(name);
    if (it == _animationSets.end()) {
        throw std::runtime_error("Unknown animation set: " + name);
    }

    return it->second;
}

const AnimationSet& AnimationLibrary::getPlayerAnimationSet(const std::string& name) const {
    return getAnimationSet(name);
}

#include "Animation/AnimationClip.h"
#include "Animation/AnimationFrame.h"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <utility>
#include <vector>

AnimationClip::AnimationClip(std::vector<AnimationFrame> frames,bool looping):
frames(std::move(frames)), looping(looping) {

}
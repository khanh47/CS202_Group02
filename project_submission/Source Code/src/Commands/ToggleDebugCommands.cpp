#include "Commands/ToggleDebugCommands.h"
#include "Game/GameSettings.h"

void ToggleGridCommand::execute() {
    auto& settings = GameSettings::getInstance();
    settings.debugDrawGrid = !settings.debugDrawGrid;
    settings.save();
}

void ToggleCoordinatesCommand::execute() {
    auto& settings = GameSettings::getInstance();
    settings.debugDrawCoordinates = !settings.debugDrawCoordinates;
    settings.save();
}

void ToggleHitboxCommand::execute() {
    auto& settings = GameSettings::getInstance();
    settings.debugDrawHitbox = !settings.debugDrawHitbox;
    settings.save();
}

void ToggleFreeCameraCommand::execute() {
    auto& settings = GameSettings::getInstance();
    settings.freeCameraMove = !settings.freeCameraMove;
    settings.save();
}

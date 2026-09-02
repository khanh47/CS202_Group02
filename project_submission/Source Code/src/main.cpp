#include "App.h"
#include "Audio/MusicManager.h"
#include "Audio/SoundManager.h"
#include "Game/GameSettings.h"

int main() {
    GameSettings& settings = GameSettings::getInstance();
    settings.load();
    Audio::MusicManager::getInstance().setEnabled(settings.musicEnabled);
    Audio::MusicManager::getInstance().setVolume(settings.musicVolume);
    Audio::SoundManager::getInstance().setEnabled(settings.soundEnabled);
    Audio::SoundManager::getInstance().setGlobalVolume(settings.soundVolume);

    App myApp;
    myApp.run();
    return 0;
}
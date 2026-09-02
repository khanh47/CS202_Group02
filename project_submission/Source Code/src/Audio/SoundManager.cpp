#include "Audio/SoundManager.h"
#include "ResourceManager.h"
#include "Game/GameSettings.h"
#include <algorithm>

namespace Audio {

SoundManager &SoundManager::getInstance() {
    static SoundManager instance;
    return instance;
}

SoundManager::SoundManager() {
    const GameSettings& settings = GameSettings::getInstance();
    _globalVolume = settings.soundVolume;
    _enabled = settings.soundEnabled;
    _pool.reserve(_maxPoolSize);
}
SoundManager::~SoundManager() = default;

void SoundManager::preload(const std::string &alias, const std::string &filepath) {
    // Delegate loading to ResourceManager; mapping stored there
    try {
        ResourceManager::getInstance().preLoadSound(filepath, alias);
    } catch (...) {
    }
}

void SoundManager::playEffect(const std::string &alias, float volumeMultiplier) {
    if (!_enabled) return;
    try {
        auto &buf = ResourceManager::getInstance().getSoundBuffer(alias);

        // Find available sound in pool
        for (auto &sound : _pool) {
            if (sound && sound->getStatus() == sf::SoundSource::Status::Stopped) {
                sound->setBuffer(buf);
                sound->setVolume(std::clamp(_globalVolume * volumeMultiplier, 0.f, 100.f));
                sound->play();
                return;
            }
        }

        if (_pool.size() < _maxPoolSize) {
            auto sound = std::make_unique<sf::Sound>(buf);
            sound->setVolume(std::clamp(_globalVolume * volumeMultiplier, 0.f, 100.f));
            sound->play();
            _pool.push_back(std::move(sound));
        }
    } catch (...) {
    }
}

void SoundManager::setGlobalVolume(float v) {
    _globalVolume = std::clamp(v, 0.f, 100.f);
}

float SoundManager::getGlobalVolume() const { return _globalVolume; }

void SoundManager::setEnabled(bool e) {
    _enabled = e;
    if (!e) stopAll();
}

bool SoundManager::isEnabled() const { return _enabled; }

void SoundManager::stopAll() {
    for (auto &sound : _pool) {
        if (sound && sound->getStatus() != sf::SoundSource::Status::Stopped) {
            sound->stop();
        }
    }
}

void SoundManager::update() {
    // Optionally shrink pool or reuse; currently no-op
}

} // namespace Audio

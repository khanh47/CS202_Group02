#include "Audio/MusicManager.h"
#include "ResourceManager.h"
#include "Game/GameSettings.h"

namespace Audio {

MusicManager &MusicManager::getInstance() {
    static MusicManager instance;
    return instance;
}

MusicManager::MusicManager() {
    _volume = GameSettings::getInstance().musicVolume;
    _enabled = GameSettings::getInstance().musicEnabled;
}

MusicManager::~MusicManager() = default;

void MusicManager::play(const std::string &alias, bool loop) {
    if (!_enabled) {
        _lastAlias = alias;
        return;
    }
    try {
        auto &music = ResourceManager::getInstance().getMusic(alias);
        if (_currentAlias == alias) {
            // already playing
            music.setLooping(loop);
            music.setVolume(_volume);
            return;
        }
        // stop previous
        if (!_currentAlias.empty()) {
            try { ResourceManager::getInstance().getMusic(_currentAlias).stop(); } catch(...){}
        }
        music.setLooping(loop);
        music.setVolume(_volume);
        music.play();
        _currentAlias = alias;
    } catch (...) {
    }
}

void MusicManager::stop() {
    if (_currentAlias.empty()) return;
    try {
        ResourceManager::getInstance().getMusic(_currentAlias).stop();
    } catch (...) {
    }
    _currentAlias.clear();
}

void MusicManager::setVolume(float v) {
    _volume = v;
    if (!_currentAlias.empty()) {
        try {
            ResourceManager::getInstance().getMusic(_currentAlias).setVolume(_volume);
        } catch (...) {}
    }
}

float MusicManager::getVolume() const { return _volume; }

void MusicManager::setEnabled(bool e) {
    if (_enabled == e) return;
    _enabled = e;
    if (!e) {
        _lastAlias = _currentAlias;
        stop();
    } else if (!_lastAlias.empty()) {
        play(_lastAlias, true);
    }
}
bool MusicManager::isEnabled() const { return _enabled; }

void MusicManager::crossfadeTo(const std::string &alias, float /*seconds*/) {
    // simple immediate switch for now
    play(alias, true);
}

} // namespace Audio

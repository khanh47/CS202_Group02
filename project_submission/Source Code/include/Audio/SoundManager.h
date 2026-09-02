#pragma once

#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <memory>

namespace Audio {

class SoundManager {
public:
    static SoundManager &getInstance();

    // Preload a sound buffer alias via ResourceManager (delegated)
    void preload(const std::string &alias, const std::string &filepath);

    // Play an effect by alias. volumeMultiplier multiplies global SFX volume.
    void playEffect(const std::string &alias, float volumeMultiplier = 1.0f);

    // Global SFX volume (0-100)
    void setGlobalVolume(float v);
    float getGlobalVolume() const;

    // Master switch for sound effects
    void setEnabled(bool e);
    bool isEnabled() const;

    void stopAll();

    // Call periodically to reclaim finished sounds (optional)
    void update();

private:
    SoundManager();
    ~SoundManager();

    // Keep each sound object at a stable address while it is playing.
    // Growing a vector<sf::Sound> can copy active sounds and interrupt them.
    std::vector<std::unique_ptr<sf::Sound>> _pool;
    float _globalVolume = 80.f;
    bool _enabled = true;
    size_t _maxPoolSize = 32;
};

} // namespace Audio

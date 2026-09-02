#pragma once

#include <string>

namespace Audio {

class MusicManager {
public:
    static MusicManager &getInstance();

    void play(const std::string &alias, bool loop = true);
    void stop();
    void setVolume(float v);
    float getVolume() const;
    void setEnabled(bool e);
    bool isEnabled() const;

    // Simple immediate switch; crossfade can be implemented later
    void crossfadeTo(const std::string &alias, float seconds);

private:
    MusicManager();
    ~MusicManager();

    std::string _currentAlias;
    std::string _lastAlias;
    float _volume = 80.f;
    bool _enabled = true;
};

} // namespace Audio

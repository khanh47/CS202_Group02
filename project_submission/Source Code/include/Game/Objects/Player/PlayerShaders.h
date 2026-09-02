#pragma once

#include <SFML/Graphics/Shader.hpp>

/**
 * @brief Singleton managing GLSL shaders for player visual effects.
 *
 * Visual effects are provided for:
 *   - Blink: rapid white ↔ raw color flashing during power-up transformation.
 *   - Rainbow: continuous hue rotation while StarMan invincibility is active.
 *   - Mega glow: pulsing golden brightness while Mega is active.
 *
 * Shaders are loaded once from embedded GLSL source strings. Call update()
 * every frame to advance the time-based uniforms before drawing.
 */
class PlayerShaders {
public:
    static PlayerShaders& getInstance();

    PlayerShaders(const PlayerShaders&) = delete;
    PlayerShaders& operator=(const PlayerShaders&) = delete;

    /** Advance the internal clock used by shader uniforms. */
    void update(float deltaTime);

    /** Shader that flashes between empty and raw sprite colors (~8 Hz). */
    sf::Shader* getGhostShader();

    /** Shader that flashes between white and raw sprite colors (~8 Hz). */
    sf::Shader* getBlinkShader();

    /** Shader that continuously rotates the hue of every pixel. */
    sf::Shader* getRainbowShader();

    /** Shader that adds a pulsing golden glow to the Mega player. */
    sf::Shader* getMegaGlowShader();

private:
    PlayerShaders();

    bool _shadersAvailable = false;
    bool _megaGlowAvailable = false;
    sf::Shader _ghostShader;
    sf::Shader _blinkShader;
    sf::Shader _rainbowShader;
    sf::Shader _megaGlowShader;
    float _time = 0.0f;
};

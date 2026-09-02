#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

class ResourceManager {
public:
	static ResourceManager &getInstance();
	// Resources retrieval
	sf::Texture &getTexture(const std::string &alias);
	sf::Font &getFont(const std::string &alias);
	sf::Music &getMusic(const std::string &alias);
    void preLoadSound(const std::string &filePath, const std::string &alias);
    sf::SoundBuffer &getSoundBuffer(const std::string &alias);

private:
	ResourceManager();
	~ResourceManager();

	ResourceManager(const ResourceManager &) = delete;
	ResourceManager &operator=(const ResourceManager &) = delete;

	void _preLoadTexture(const std::string &filePath, const std::string &alias);
	void _preLoadFont(const std::string &filePath, const std::string &alias);
	void _preLoadMusic(const std::string &filePath, const std::string &alias);
	void _preLoadSound(const std::string &filePath, const std::string &alias);

	void _unloadTexture(const std::string &alias);
	void _unloadFont(const std::string &alias);
	void _unloadMusic(const std::string &alias);

	std::unordered_map<std::string, sf::Texture> _textures;
	std::unordered_map<std::string, sf::Font> _fonts;
	std::unordered_map<std::string, std::unique_ptr<sf::Music>> _musics;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> _soundBuffers;

	std::map<std::string, std::string> _MappingAliasToFilename;
};
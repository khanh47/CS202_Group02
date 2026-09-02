#include "ResourceManager.h"
#include <iostream>
#include <stdexcept>

ResourceManager &ResourceManager::getInstance() {
	static ResourceManager instance; // Created only once
	return instance; // Always returns same instance
}

void ResourceManager::_preLoadTexture(const std::string &filename, const std::string &alias) {
	_MappingAliasToFilename[alias] = filename;
	if (_textures.find(filename) != _textures.end()) {
		// std::cerr << "Texture already loaded: " << filename << std::endl;
		return;
	}

	sf::Texture texture;
	if (!texture.loadFromFile(filename)) {
		throw std::runtime_error("Failed to load texture: " + filename);
	}
	texture.setSmooth(false);

	_textures[filename] = texture;
}

void ResourceManager::_preLoadFont(const std::string &filename, const std::string &alias) {
	if (_fonts.find(filename) != _fonts.end())
		return;

	sf::Font font;
	if (!font.openFromFile(filename))
		throw std::runtime_error("Failed to load font: " + filename);

	_fonts[filename] = font;
	_MappingAliasToFilename[alias] = filename;
}

void ResourceManager::_preLoadMusic(const std::string &filename, const std::string &alias) {
	if (_musics.find(filename) != _musics.end())
		return;

	auto music = std::make_unique<sf::Music>();
	if (!music->openFromFile(filename))
		throw std::runtime_error("Failed to load music: " + filename);

	_musics.emplace(filename, std::move(music));
	_MappingAliasToFilename[alias] = filename;
}

void ResourceManager::_preLoadSound(const std::string &filename, const std::string &alias) {
	if (_soundBuffers.find(filename) != _soundBuffers.end())
		return;

	auto buffer = std::make_unique<sf::SoundBuffer>();
	if (!buffer->loadFromFile(filename))
		throw std::runtime_error("Failed to load sound: " + filename);

	_soundBuffers.emplace(filename, std::move(buffer));
	_MappingAliasToFilename[alias] = filename;
}

void ResourceManager::preLoadSound(const std::string &filePath, const std::string &alias) {
	_preLoadSound(filePath, alias);
}

void ResourceManager::_unloadTexture(const std::string &alias) {
	auto it = _MappingAliasToFilename.find(alias);
	if (it != _MappingAliasToFilename.end()) {
		const std::string &filename = it->second;
		auto texIt = _textures.find(filename);
		if (texIt != _textures.end()) {
			_textures.erase(texIt);
		}
		_MappingAliasToFilename.erase(it);
	}
}

void ResourceManager::_unloadFont(const std::string &alias) {
	auto it = _MappingAliasToFilename.find(alias);
	if (it != _MappingAliasToFilename.end()) {
		const std::string &filename = it->second;
		auto fontIt = _fonts.find(filename);
		if (fontIt != _fonts.end()) {
			_fonts.erase(fontIt);
		}
		_MappingAliasToFilename.erase(it);
	}
}

void ResourceManager::_unloadMusic(const std::string &alias) {
	auto it = _MappingAliasToFilename.find(alias);
	if (it != _MappingAliasToFilename.end()) {
		const std::string &filename = it->second;
		auto musicIt = _musics.find(filename);
		if (musicIt != _musics.end()) {
			_musics.erase(musicIt);
		}
		_MappingAliasToFilename.erase(it);
	}
}

// flyweight pattern implementation
sf::Texture &ResourceManager::getTexture(const std::string &alias) {
	auto it = _MappingAliasToFilename.find(alias);
	if (it != _MappingAliasToFilename.end()) {
		const std::string &filename = it->second;
		auto texIt = _textures.find(filename);
		if (texIt != _textures.end()) {
			return texIt->second;
		} else {
			throw std::runtime_error("Texture not loaded: " + filename);
		}
	} else {
		throw std::runtime_error("Texture alias not found: " + alias);
	}
}

sf::Font &ResourceManager::getFont(const std::string &alias) {
	auto it = _MappingAliasToFilename.find(alias);
	if (it != _MappingAliasToFilename.end()) {
		const std::string &filename = it->second;
		auto fontIt = _fonts.find(filename);
		if (fontIt != _fonts.end()) {
			return fontIt->second;
		} else {
			throw std::runtime_error("Font not found: " + filename);
		}
	} else {
		throw std::runtime_error("Font alias not found: " + alias);
	}
}

sf::Music &ResourceManager::getMusic(const std::string &alias) {
	auto it = _MappingAliasToFilename.find(alias);
	if (it != _MappingAliasToFilename.end()) {
		const std::string &filename = it->second;
		auto musicIt = _musics.find(filename);
		if (musicIt != _musics.end()) {
			return *musicIt->second;
		}
		throw std::runtime_error("Music not found: " + filename);
	}

	throw std::runtime_error("Music alias not found: " + alias);
}

sf::SoundBuffer &ResourceManager::getSoundBuffer(const std::string &alias) {
	auto it = _MappingAliasToFilename.find(alias);
	if (it != _MappingAliasToFilename.end()) {
		const std::string &filename = it->second;
		auto bufIt = _soundBuffers.find(filename);
		if (bufIt != _soundBuffers.end()) {
			return *bufIt->second;
		}
		throw std::runtime_error("Sound buffer not loaded: " + filename);
	}

	throw std::runtime_error("Sound alias not found: " + alias);
}

ResourceManager::~ResourceManager() {
	_textures.clear();
	_fonts.clear();
	_musics.clear();
	_MappingAliasToFilename.clear();
}

ResourceManager::ResourceManager() {
	_preLoadFont("assets/fonts/SuperMario256.ttf", "SuperMario");
	_preLoadFont("assets/fonts/moon_get-Heavy.ttf", "moon_get");

	_preLoadMusic("assets/soundtrack/music/title_screen.mp3", "title_screen");
	_preLoadMusic("assets/soundtrack/music/ground_theme.mp3", "ground_theme");
	_preLoadMusic("assets/soundtrack/music/underground_theme.mp3", "underground_theme");
	_preLoadMusic("assets/soundtrack/music/starman_theme.mp3", "starman_theme");
	_preLoadMusic("assets/soundtrack/music/course_clear.mp3", "course_clear");
	_preLoadMusic("assets/soundtrack/music/game_over.mp3", "game_over_music");
	
	_preLoadSound("assets/soundtrack/sfx/pipe.mp3", "pipe");
	_preLoadSound("assets/soundtrack/sfx/power_up.mp3", "power_up");
	_preLoadSound("assets/soundtrack/sfx/power_down.mp3", "power_down");
	_preLoadSound("assets/soundtrack/sfx/mega_up.mp3", "mega_up");
	_preLoadSound("assets/soundtrack/sfx/one_up.mp3", "one_up");
	_preLoadSound("assets/soundtrack/sfx/sprout.mp3", "sprout");
	_preLoadSound("assets/soundtrack/sfx/fireball.mp3", "fireball");
	_preLoadSound("assets/soundtrack/sfx/footstep.mp3", "footstep");
	_preLoadSound("assets/soundtrack/sfx/jump.mp3", "jump");
	_preLoadSound("assets/soundtrack/sfx/dead.mp3", "dead");
	_preLoadSound("assets/soundtrack/sfx/coin.mp3", "coin");
	_preLoadSound("assets/soundtrack/sfx/break.mp3", "break");
	_preLoadSound("assets/soundtrack/sfx/kill.mp3", "kill");
	_preLoadSound("assets/soundtrack/sfx/select_button.mp3", "select_button");
	
	_preLoadTexture("assets/sprites/Tiles/mario_and_items.png", "mario_and_items");
	_preLoadTexture("assets/sprites/Tilesets/mutiple_tilesets.png", "mutiple_tilesets");
	_preLoadTexture("assets/sprites/Tilesets/general_tiles.png", "general_tiles");

	_preLoadTexture("assets/spritesheets/mario_spritesheet.png", "mario_spritesheet");
	_preLoadTexture("assets/spritesheets/fire_mario_spritesheet.png", "fire_mario_spritesheet");
	_preLoadTexture("assets/spritesheets/luigi_spritesheet.png", "luigi_spritesheet");
	_preLoadTexture("assets/spritesheets/fire_luigi_spritesheet.png", "fire_luigi_spritesheet");

	_preLoadTexture("assets/backgrounds/far_sky.png", "far_sky");
	_preLoadTexture("assets/backgrounds/close_bush.png", "close_bush");
	_preLoadTexture("assets/backgrounds/far_underground.png", "far_underground");
	_preLoadTexture("assets/backgrounds/close_underground.png", "close_underground");
	_preLoadTexture("assets/guis/game_over.png", "game_over");
	_preLoadTexture(
		"assets/guis/Square_premade_buttons_16x16px.png",
		"square_premade_buttons"
	);
	_preLoadTexture("assets/spritesheets/goomba_spritesheet.png", "goomba_spritesheet");
	_preLoadTexture("assets/spritesheets/koopa_spritesheet.png", "koopa_spritesheet");
	_preLoadTexture("assets/spritesheets/piranha_plant_spritesheet.png", "piranha_plant_spritesheet");
	_preLoadTexture("assets/spritesheets/transparent_coin_strip.png", "coin_spritesheet");
	_preLoadTexture("assets/spritesheets/transparent_brick_spritesheet.png", "brick_spritesheet");
	_preLoadTexture("assets/spritesheets/transparent_underground_brick_spritesheet.png", "underground_brick_spritesheet");
	_preLoadTexture("assets/spritesheets/transparent_lucky_block_spritesheet.png", "lucky_block_spritesheet");
	_preLoadTexture("assets/spritesheets/transparent_underground_lucky_block_spritesheet.png", "underground_lucky_block_spritesheet");
	_preLoadTexture("assets/spritesheets/transparent_mega_coin_strip.png", "mega_coin_spritesheet");
	_preLoadTexture("assets/spritesheets/mega_mushroom_spritesheet.png", "mega_mushroom_spritesheet");

	_preLoadTexture("assets/spritesheets/goal_flag_spritesheet.png", "goal_flag_spritesheet");
	_preLoadTexture("assets/spritesheets/checkpoint_flag_spritesheet.png", "checkpoint_flag_spritesheet");
	_preLoadTexture("assets/spritesheets/pipes_spritesheet.png", "pipes_spritesheet");

	// ------------------------------------------------------------------
	// Autotile tilesets
	// ------------------------------------------------------------------
	// Each entry below is the PLACEHOLDER using the existing brick texture.
	// To use a real tileset, replace the file path with your PNG and keep
	// the alias.  The alias must match the "texture" field in
	// assets/datas/autotile_defs.json.
	//
	// Real tileset layout expected: 4 x 4 grid, each cell 64 x 64 px,
	// masks 0-15 arranged left-to-right, top-to-bottom.
	//
	// Example (after adding your PNGs):
	//   _preLoadTexture("assets/sprites/Tiles/grassland_autotile.png",  "at_grassland");
	//   _preLoadTexture("assets/sprites/Tiles/castle_autotile.png",     "at_castle");
	//   _preLoadTexture("assets/sprites/Tiles/underground_autotile.png","at_underground");
	_preLoadTexture("assets/sprites/Tilesets/transparent_grassland_autotile.png", "at_grassland");
	_preLoadTexture("assets/sprites/Tilesets/transparent_castle_autotile.png", "at_castle");
	_preLoadTexture("assets/sprites/Tilesets/transparent_underground_autotile.png", "at_underground");
}

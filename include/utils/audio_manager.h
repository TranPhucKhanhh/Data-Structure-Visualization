#pragma once

#include <SFML/Audio.hpp>
#include <ui/common.h>

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>

class AudioManager {
public:
	bool initialize(const std::filesystem::path& assetDirectory);
	void applySettings(const AudioSettings& settings);

	void setMusicEnabled(bool enabled);
	void setMusicVolume(float volume);
	void setSfxEnabled(bool enabled);
	void setSfxVolume(float volume);

	void playClick();

	bool hasMusic() const;
	bool hasClickSfx() const;
	bool isInitialized() const;
	const std::string& getLastError() const;

private:
	void tryPlayMusic();

	bool initialized_ = false;
	bool musicEnabled_ = true;
	bool sfxEnabled_ = true;
	float musicVolume_ = 40.0f;
	float sfxVolume_ = 70.0f;

	bool musicLoaded_ = false;
	bool clickLoaded_ = false;
	std::string lastError_;

	sf::Music backgroundMusic_;
	sf::SoundBuffer clickBuffer_;
	std::optional<sf::Sound> clickSound_;

	std::chrono::steady_clock::time_point lastClickTime_ = std::chrono::steady_clock::now();
};

extern AudioManager audioManager;

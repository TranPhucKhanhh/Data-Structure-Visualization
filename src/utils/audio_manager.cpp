#include "utils/audio_manager.h"

#include <algorithm>
#include <iostream>

namespace {
	constexpr float kMinVolume = 0.0f;
	constexpr float kMaxVolume = 100.0f;
	constexpr auto kClickCooldown = std::chrono::milliseconds(40);
}

AudioManager audioManager;

bool AudioManager::initialize(const std::filesystem::path& assetDirectory)
{
	initialized_ = true;
	lastError_.clear();

	std::filesystem::path bgmPath = assetDirectory / "bgm.mp3";
	musicLoaded_ = backgroundMusic_.openFromFile(bgmPath.string());
	if (!musicLoaded_) {
		bgmPath = assetDirectory / "bgm.ogg";
		musicLoaded_ = backgroundMusic_.openFromFile(bgmPath.string());
	}
	if (musicLoaded_) {
		backgroundMusic_.setLooping(true);
		backgroundMusic_.setVolume(musicVolume_);
	}
	else {
		std::cerr << "Warning: Could not load background music: " << bgmPath << std::endl;
	}

	std::filesystem::path clickPath = assetDirectory / "click.wav";
	clickLoaded_ = clickBuffer_.loadFromFile(clickPath.string());
	if (!clickLoaded_) {
		clickPath = assetDirectory / "click.mp3";
		clickLoaded_ = clickBuffer_.loadFromFile(clickPath.string());
	}
	if (clickLoaded_) {
		clickSound_.emplace(clickBuffer_);
		clickSound_->setVolume(sfxVolume_);
	}
	else {
		std::cerr << "Warning: Could not load click sound: " << clickPath << std::endl;
	}

	if (!musicLoaded_ && !clickLoaded_) {
		lastError_ = "No audio files loaded.";
	}

	tryPlayMusic();
	return musicLoaded_ || clickLoaded_;
}

void AudioManager::applySettings(const AudioSettings& settings)
{
	setMusicEnabled(settings.musicEnabled);
	setMusicVolume(settings.musicVolume);
	setSfxEnabled(settings.sfxEnabled);
	setSfxVolume(settings.sfxVolume);
}

void AudioManager::setMusicEnabled(bool enabled)
{
	musicEnabled_ = enabled;
	if (!musicLoaded_) {
		return;
	}

	if (!musicEnabled_) {
		backgroundMusic_.stop();
		return;
	}

	tryPlayMusic();
}

void AudioManager::setMusicVolume(float volume)
{
	musicVolume_ = std::clamp(volume, kMinVolume, kMaxVolume);
	if (musicLoaded_) {
		backgroundMusic_.setVolume(musicVolume_);
	}
}

void AudioManager::setSfxEnabled(bool enabled)
{
	sfxEnabled_ = enabled;
}

void AudioManager::setSfxVolume(float volume)
{
	sfxVolume_ = std::clamp(volume, kMinVolume, kMaxVolume);
	if (clickLoaded_ && clickSound_.has_value()) {
		clickSound_->setVolume(sfxVolume_);
	}
}

void AudioManager::playClick()
{
	if (!initialized_ || !sfxEnabled_ || !clickLoaded_) {
		return;
	}

	const auto now = std::chrono::steady_clock::now();
	if (now - lastClickTime_ < kClickCooldown) {
		return;
	}
	lastClickTime_ = now;

	if (!clickSound_.has_value()) {
		return;
	}

	clickSound_->stop();
	clickSound_->play();
}

bool AudioManager::hasMusic() const
{
	return musicLoaded_;
}

bool AudioManager::hasClickSfx() const
{
	return clickLoaded_;
}

bool AudioManager::isInitialized() const
{
	return initialized_;
}

const std::string& AudioManager::getLastError() const
{
	return lastError_;
}

void AudioManager::tryPlayMusic()
{
	if (!musicLoaded_ || !musicEnabled_) {
		return;
	}

	if (backgroundMusic_.getStatus() != sf::SoundSource::Status::Playing) {
		backgroundMusic_.play();
	}
}

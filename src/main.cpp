#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <ui/menu.h>
#include <ui/trie.h>
#include <ui/heap.h>
#include <ui/shortestpath.h>
#include <ui/singlylinkedlist.h>
#include <ui/common.h>
#include <utils/audio_manager.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <iostream>

namespace {
	constexpr const char* kWindowTitle = "Data Visualization :))";
	constexpr const char* kGraphicsConfigFileName = "graphics_settings.cfg";

	std::filesystem::path getGraphicsConfigPath()
	{
		return std::filesystem::current_path() / kGraphicsConfigFileName;
	}

	bool parseBoolValue(const std::string& value)
	{
		return value == "1" || value == "true" || value == "True" || value == "TRUE";
	}

	void normalizeGraphicsSettings(GraphicsSettings& settings, int monitorWidth, int monitorHeight)
	{
		settings.resolutionWidth = std::clamp(settings.resolutionWidth, 800, std::max(800, monitorWidth));
		settings.resolutionHeight = std::clamp(settings.resolutionHeight, 600, std::max(600, monitorHeight));
		settings.antialiasingLevel = std::clamp(settings.antialiasingLevel, 0, 16);
		settings.antialiasingEnabled = settings.antialiasingLevel > 0;
		settings.fpsLimit = std::clamp(settings.fpsLimit, 24, 240);
	}

	bool loadSettingsFromConfig(GraphicsSettings& graphics, AudioSettings& audio)
	{
		const std::filesystem::path configPath = getGraphicsConfigPath();
		std::ifstream in(configPath);
		if (!in.is_open()) {
			return false;
		}

		GraphicsSettings loadedGraphics = graphics;
		AudioSettings loadedAudio = audio;
		std::string line;
		while (std::getline(in, line)) {
			const std::size_t eq = line.find('=');
			if (eq == std::string::npos || eq == 0 || eq + 1 >= line.size()) {
				continue;
			}

			const std::string key = line.substr(0, eq);
			const std::string value = line.substr(eq + 1);
			if (key == "windowMode") {
				loadedGraphics.windowMode = (value == "fullscreen") ? WindowMode::Fullscreen : WindowMode::Windowed;
			}
			else if (key == "fxaaEnabled") {
				loadedGraphics.fxaaEnabled = parseBoolValue(value);
			}
			else if (key == "antialiasingEnabled") {
				loadedGraphics.antialiasingEnabled = parseBoolValue(value);
			}
			else if (key == "vsyncEnabled") {
				loadedGraphics.vsyncEnabled = parseBoolValue(value);
			}
			else if (key == "musicEnabled") {
				loadedAudio.musicEnabled = parseBoolValue(value);
			}
			else if (key == "sfxEnabled") {
				loadedAudio.sfxEnabled = parseBoolValue(value);
			}
			else {
				try {
					if (key == "resolutionWidth") {
						loadedGraphics.resolutionWidth = std::stoi(value);
					}
					else if (key == "resolutionHeight") {
						loadedGraphics.resolutionHeight = std::stoi(value);
					}
					else if (key == "antialiasingLevel") {
						loadedGraphics.antialiasingLevel = std::stoi(value);
					}
					else if (key == "fpsLimit") {
						loadedGraphics.fpsLimit = std::stoi(value);
					}
					else if (key == "musicVolume") {
						loadedAudio.musicVolume = std::stof(value);
					}
					else if (key == "sfxVolume") {
						loadedAudio.sfxVolume = std::stof(value);
					}
				}
				catch (const std::exception&) {
					// Ignore malformed numeric values and keep previous defaults.
				}
			}
		}

		graphics = loadedGraphics;
		audio = loadedAudio;
		return true;
	}

	void saveSettingsToConfig(const GraphicsSettings& graphics, const AudioSettings& audio)
	{
		const std::filesystem::path configPath = getGraphicsConfigPath();
		std::ofstream out(configPath, std::ios::trunc);
		if (!out.is_open()) {
			return;
		}

		out << "windowMode=" << (graphics.windowMode == WindowMode::Fullscreen ? "fullscreen" : "windowed") << '\n';
		out << "resolutionWidth=" << graphics.resolutionWidth << '\n';
		out << "resolutionHeight=" << graphics.resolutionHeight << '\n';
		out << "fxaaEnabled=" << (graphics.fxaaEnabled ? 1 : 0) << '\n';
		out << "antialiasingEnabled=" << (graphics.antialiasingEnabled ? 1 : 0) << '\n';
		out << "antialiasingLevel=" << graphics.antialiasingLevel << '\n';
		out << "vsyncEnabled=" << (graphics.vsyncEnabled ? 1 : 0) << '\n';
		out << "fpsLimit=" << graphics.fpsLimit << '\n';
		out << "musicEnabled=" << (audio.musicEnabled ? 1 : 0) << '\n';
		out << "musicVolume=" << audio.musicVolume << '\n';
		out << "sfxEnabled=" << (audio.sfxEnabled ? 1 : 0) << '\n';
		out << "sfxVolume=" << audio.sfxVolume << '\n';
	}

	std::filesystem::path resolveFontPath()
	{
		const std::string font_name = "/Roboto_Condensed-Regular.ttf";
		const auto path = std::filesystem::path(std::string(ASSET_FONT + font_name));
		if (std::filesystem::exists(path)) {
			return path;	
		}
	
		std::cerr << "Warning: Font file not found at " << path << std::endl;
		return {};

	}

	std::filesystem::path resolveAudioPath()
	{
		const auto path = std::filesystem::path(std::string(ASSET_AUDIO));
		if (std::filesystem::exists(path)) {
			return path;
		}

		std::cerr << "Warning: Audio asset directory not found at " << path << std::endl;
		return {};
	}

	void loadMenuFonts()
	{
		menuTitleFont = nullptr;
		menuSubtitleFont = nullptr;
		menuCardTitleFont = nullptr;
		menuCardDescFont = nullptr;

		ImGuiIO& io = ImGui::GetIO();
		const std::filesystem::path fontPath = resolveFontPath();
		if (fontPath.empty()) {
			return;
		}

		const std::string fontPathStr = fontPath.string();
		const char* fontPathCStr = fontPathStr.c_str();
		menuTitleFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 56.0f);
		menuSubtitleFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 28.0f);
		menuCardTitleFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 24.0f);
		menuCardDescFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 20.0f);
		io.FontDefault = menuCardDescFont ? menuCardDescFont : io.FontDefault;
		(void) ImGui::SFML::UpdateFontTexture();
	}

	void drawActiveScreen(sf::RenderWindow& window)
	{
		// Draw the current UI state depend on the UI view state
		if (uiConfig.state == UIState::Menu) {
			menu_ui.draw();
		}
		else if (uiConfig.state == UIState::Trie) {
			trie_ui.draw();
			trie_ui.drawSfml(window);
		}
		else if (uiConfig.state == UIState::Heap) {
			heap_ui.draw();
			heap_ui.drawSfml(window);
		}
		else if (uiConfig.state == UIState::ShortestPath) {
			shortest_path_ui.draw();
			shortest_path_ui.drawSfml(window);
		}
		else if (uiConfig.state == UIState::SinglyLinkedList) {
			singly_linked_list_ui.draw();
			singly_linked_list_ui.drawSfml(window);
		}
	}

	sf::VideoMode pickVideoMode(const GraphicsSettings& settings)
	{
		if (settings.windowMode == WindowMode::Fullscreen) {
			const std::vector<sf::VideoMode> modes = sf::VideoMode::getFullscreenModes();
			for (const sf::VideoMode& mode : modes) {
				if (mode.size.x == static_cast<unsigned int>(settings.resolutionWidth) &&
					mode.size.y == static_cast<unsigned int>(settings.resolutionHeight)) {
					return mode;
				}
			}
			return sf::VideoMode::getDesktopMode();
		}

		return sf::VideoMode({
			static_cast<unsigned int>(std::max(800, settings.resolutionWidth)),
			static_cast<unsigned int>(std::max(600, settings.resolutionHeight))
		});
	}

	sf::RenderWindow createWindowForSettings(const GraphicsSettings& settings)
	{
		sf::ContextSettings contextSettings;
		contextSettings.antiAliasingLevel = settings.antialiasingEnabled
			? static_cast<unsigned int>(std::clamp(settings.antialiasingLevel, 0, 16))
			: 0U;

		const sf::VideoMode mode = pickVideoMode(settings);
		const sf::State state = (settings.windowMode == WindowMode::Fullscreen)
			? sf::State::Fullscreen
			: sf::State::Windowed;
		const auto windowStyle = (settings.windowMode == WindowMode::Fullscreen)
			? sf::Style::Default
			: (sf::Style::Titlebar | sf::Style::Close);

		sf::RenderWindow window(
			mode,
			kWindowTitle,
			windowStyle,
			state,
			contextSettings
		);

		window.setVerticalSyncEnabled(settings.vsyncEnabled);
		window.setFramerateLimit(static_cast<unsigned int>(std::max(24, settings.fpsLimit)));
		return window;
	}

	bool applyGraphicsSettings(sf::RenderWindow& window)
	{
		ImGui::SFML::Shutdown();
		window.close();
		window = createWindowForSettings(uiConfig.graphicsSettings);
		if (!window.isOpen()) {
			return false;
		}
		if (!ImGui::SFML::Init(window)) {
			return false;
		}
		loadMenuFonts();
		saveSettingsToConfig(uiConfig.graphicsSettings, uiConfig.audioSettings);
		return true;
	}
}

int main()
{
	const sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
	uiConfig.monitorWidth = static_cast<int>(desktopMode.size.x);
	uiConfig.monitorHeight = static_cast<int>(desktopMode.size.y);

	uiConfig.graphicsSettings.windowMode = WindowMode::Fullscreen;
	uiConfig.graphicsSettings.resolutionWidth = 1920;
	uiConfig.graphicsSettings.resolutionHeight = 1080;
	uiConfig.graphicsSettings.antialiasingEnabled = true;
	uiConfig.graphicsSettings.antialiasingLevel = 8;
	uiConfig.graphicsSettings.vsyncEnabled = true;
	uiConfig.graphicsSettings.fpsLimit = 60;
	uiConfig.graphicsSettings.fxaaEnabled = false;
	uiConfig.audioSettings.musicEnabled = true;
	uiConfig.audioSettings.musicVolume = 40.0f;
	uiConfig.audioSettings.sfxEnabled = true;
	uiConfig.audioSettings.sfxVolume = 70.0f;
	(void) loadSettingsFromConfig(uiConfig.graphicsSettings, uiConfig.audioSettings);
	normalizeGraphicsSettings(uiConfig.graphicsSettings, uiConfig.monitorWidth, uiConfig.monitorHeight);
	uiConfig.audioSettings.musicVolume = std::clamp(uiConfig.audioSettings.musicVolume, 0.0f, 100.0f);
	uiConfig.audioSettings.sfxVolume = std::clamp(uiConfig.audioSettings.sfxVolume, 0.0f, 100.0f);

	sf::RenderWindow window = createWindowForSettings(uiConfig.graphicsSettings);
	uiConfig.requestAppQuit = false;
	uiConfig.requestGraphicsApply = false;

	if (!ImGui::SFML::Init(window)) {
		return -1;
	}
	loadMenuFonts();

	const std::filesystem::path audioPath = resolveAudioPath();
	if (!audioPath.empty()) {
		(void) audioManager.initialize(audioPath);
	}
	audioManager.applySettings(uiConfig.audioSettings);
	saveSettingsToConfig(uiConfig.graphicsSettings, uiConfig.audioSettings);

	sf::Clock deltaClock;

	while (window.isOpen())
	{
		if (uiConfig.requestGraphicsApply) {
			normalizeGraphicsSettings(uiConfig.graphicsSettings, uiConfig.monitorWidth, uiConfig.monitorHeight);
			if (!applyGraphicsSettings(window)) {
				return -1;
			}
			uiConfig.requestGraphicsApply = false;
		}

		while (const std::optional event = window.pollEvent())
		{
			ImGui::SFML::ProcessEvent(window, *event);

			if (event->is<sf::Event::Closed>())
				window.close();			
			
		}

		ImGui::SFML::Update(window, deltaClock.restart());

		window.clear();
		drawActiveScreen(window);
		if (uiConfig.requestAppQuit) {
			window.close();
		}
		ImGui::SFML::Render(window);

		window.display();
	}
}

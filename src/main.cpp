#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <ui/menu.h>
#include <ui/trie.h>
#include <ui/heap.h>
#include <ui/shortestpath.h>
#include <ui/singlylinkedlist.h>
#include <ui/common.h>

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

	bool loadGraphicsSettingsFromConfig(GraphicsSettings& settings)
	{
		const std::filesystem::path configPath = getGraphicsConfigPath();
		std::ifstream in(configPath);
		if (!in.is_open()) {
			return false;
		}

		GraphicsSettings loaded = settings;
		std::string line;
		while (std::getline(in, line)) {
			const std::size_t eq = line.find('=');
			if (eq == std::string::npos || eq == 0 || eq + 1 >= line.size()) {
				continue;
			}

			const std::string key = line.substr(0, eq);
			const std::string value = line.substr(eq + 1);
			if (key == "windowMode") {
				loaded.windowMode = (value == "fullscreen") ? WindowMode::Fullscreen : WindowMode::Windowed;
			}
			else if (key == "fxaaEnabled") {
				loaded.fxaaEnabled = parseBoolValue(value);
			}
			else if (key == "antialiasingEnabled") {
				loaded.antialiasingEnabled = parseBoolValue(value);
			}
			else if (key == "vsyncEnabled") {
				loaded.vsyncEnabled = parseBoolValue(value);
			}
			else {
				try {
					if (key == "resolutionWidth") {
						loaded.resolutionWidth = std::stoi(value);
					}
					else if (key == "resolutionHeight") {
						loaded.resolutionHeight = std::stoi(value);
					}
					else if (key == "antialiasingLevel") {
						loaded.antialiasingLevel = std::stoi(value);
					}
					else if (key == "fpsLimit") {
						loaded.fpsLimit = std::stoi(value);
					}
				}
				catch (const std::exception&) {
					// Ignore malformed numeric values and keep previous defaults.
				}
			}
		}

		settings = loaded;
		return true;
	}

	void saveGraphicsSettingsToConfig(const GraphicsSettings& settings)
	{
		const std::filesystem::path configPath = getGraphicsConfigPath();
		std::ofstream out(configPath, std::ios::trunc);
		if (!out.is_open()) {
			return;
		}

		out << "windowMode=" << (settings.windowMode == WindowMode::Fullscreen ? "fullscreen" : "windowed") << '\n';
		out << "resolutionWidth=" << settings.resolutionWidth << '\n';
		out << "resolutionHeight=" << settings.resolutionHeight << '\n';
		out << "fxaaEnabled=" << (settings.fxaaEnabled ? 1 : 0) << '\n';
		out << "antialiasingEnabled=" << (settings.antialiasingEnabled ? 1 : 0) << '\n';
		out << "antialiasingLevel=" << settings.antialiasingLevel << '\n';
		out << "vsyncEnabled=" << (settings.vsyncEnabled ? 1 : 0) << '\n';
		out << "fpsLimit=" << settings.fpsLimit << '\n';
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
		saveGraphicsSettingsToConfig(uiConfig.graphicsSettings);
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
	(void) loadGraphicsSettingsFromConfig(uiConfig.graphicsSettings);
	normalizeGraphicsSettings(uiConfig.graphicsSettings, uiConfig.monitorWidth, uiConfig.monitorHeight);

	sf::RenderWindow window = createWindowForSettings(uiConfig.graphicsSettings);
	uiConfig.requestAppQuit = false;
	uiConfig.requestGraphicsApply = false;

	if (!ImGui::SFML::Init(window)) {
		return -1;
	}
	loadMenuFonts();

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

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
#include <string>

#include <iostream>

namespace {
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
}

int main()
{
	sf::ContextSettings contextSettings;
	contextSettings.antiAliasingLevel = 8;

	// Prefer 1920x1080 fullscreen to keep the authored UI proportions stable.
	sf::VideoMode fullscreenMode = sf::VideoMode::getDesktopMode();
	for (const sf::VideoMode& mode : sf::VideoMode::getFullscreenModes()) {
		if (mode.size.x == 1920 && mode.size.y == 1080) {
			fullscreenMode = mode;
			break;
		}
	}

	sf::RenderWindow window(
		fullscreenMode,
		"Data Visualization :))",
		sf::Style::Default,
		sf::State::Fullscreen,
		contextSettings
	);

	uiConfig.requestAppQuit = false;

	if (!ImGui::SFML::Init(window)) {
		return -1;
	}

	// Load custom font for ImGui menu text
	ImGuiIO& io = ImGui::GetIO();
	const std::filesystem::path fontPath = resolveFontPath();
	if (!fontPath.empty()) {
		const std::string fontPathStr = fontPath.string();
		const char* fontPathCStr = fontPathStr.c_str();
		menuTitleFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 56.0f);
		menuSubtitleFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 28.0f);
		menuCardTitleFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 24.0f);
		menuCardDescFont = io.Fonts->AddFontFromFileTTF(fontPathCStr, 20.0f);

		io.FontDefault = menuCardDescFont ? menuCardDescFont : io.FontDefault;
		(void) ImGui::SFML::UpdateFontTexture();
	}

	sf::Clock deltaClock;

	while (window.isOpen())
	{
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

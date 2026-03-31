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

namespace {
	std::filesystem::path resolveFontPath()
	{
		const std::array<std::filesystem::path, 5> candidates = {
			std::filesystem::path("fonts/Roboto_Condensed-Regular.ttf"),
			std::filesystem::path("../fonts/Roboto_Condensed-Regular.ttf"),
			std::filesystem::path("../../fonts/Roboto_Condensed-Regular.ttf"),
			std::filesystem::path("../../../fonts/Roboto_Condensed-Regular.ttf"),
			std::filesystem::path("d:/Final Project CS163/Data-Structure-Visualization/fonts/Roboto_Condensed-Regular.ttf")
		};

		for (const auto& candidate : candidates) {
			if (std::filesystem::exists(candidate)) {
				return candidate;
			}
		}

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
	sf::RenderWindow window(
		sf::VideoMode({ 1920, 1080 }),
		"Data Visualization :))",
		sf::Style::Default,
		sf::State::Windowed,
		contextSettings
	);

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

		if (menuCardDescFont != nullptr) {
			io.FontDefault = menuCardDescFont;
			const bool fontTextureUpdated = ImGui::SFML::UpdateFontTexture();
			(void)fontTextureUpdated;
		}
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
		ImGui::SFML::Render(window);

		window.display();
	}
}

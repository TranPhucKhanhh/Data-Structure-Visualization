#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <ui/menu.h>
#include <ui/trie.h>
#include <ui/heap.h>
#include <ui/shortestpath.h>
#include <ui/singlylinkedlist.h>
#include <ui/common.h>

namespace {
	void drawActiveScreen(sf::RenderWindow& window)
	{
		while ( const std::optional event = window.pollEvent() )
		{
			if ( event->is<sf::Event::Closed>() )
				window.close();

			
		}

		window.clear();

		// Draw the current UI state depend on the UI view state
		if (uiConfig.state == UIState::Menu) {
			menu_ui.draw();
		}
		else if (uiConfig.state == UIState::Trie) {
			trie_ui.draw();
		}
		else if (uiConfig.state == UIState::Heap) {
			heap_ui.draw();
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
	sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Data Visualization :))");

	if (!ImGui::SFML::Init(window)) {
		return -1;
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

#include <SFML/Graphics.hpp>
#include <ui/menu.h>
#include <ui/trie.h>
#include <ui/heap.h>
#include <ui/shortestpath.h>
#include <ui/singlylinkedlist.h>
#include <ui/common.h>

int main()
{
	sf::RenderWindow window( sf::VideoMode( { 1920, 1080} ), "Data Visualization :))" );

	while ( window.isOpen() )
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
		}

		window.display();
	}
}

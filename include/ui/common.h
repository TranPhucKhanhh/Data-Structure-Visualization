#pragma once

enum class UIState{
	Menu,
	Trie,
	Heap,
	SinglyLinkedList,
	ShortestPath,
};

struct UIConfig {
	UIState state = UIState::Menu;
};

inline UIConfig uiConfig;
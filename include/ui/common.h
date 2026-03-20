#pragma once

enum class UIState{
	Menu,
	Trie,
	Heap,
	SinglyLinkedList,
	ShortestPath,
};

struct VisualStyle {
	float nodeRadius = 28.0f;
	float edgeThickness = 3.0f;
	float fontScale = 1.0f;
};

struct UIConfig {
	UIState state = UIState::Menu;
	VisualStyle style;
};

inline UIConfig uiConfig;
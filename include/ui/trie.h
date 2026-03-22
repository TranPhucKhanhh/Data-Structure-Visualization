#pragma once

#include <imgui.h>
#include <ui/common.h>

class TrieUI {
public:
	TrieUI();

	void draw();
private:
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
};

inline TrieUI trie_ui;
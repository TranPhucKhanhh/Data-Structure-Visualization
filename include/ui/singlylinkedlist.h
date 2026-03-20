#pragma once

#include <array>
#include <logic/singlylinkedlist.h>

class SinglyLinkedListUI {
public:
	SinglyLinkedListUI();

	void draw();
private:
	std::array<char, 256> txtPath_{};
	std::array<char, 256> jsonPath_{};
	int randomCount_ = 8;
	int randomMin_ = 0;
	int randomMax_ = 99;

	int addIndex_ = 0;
	int addValue_ = 0;
	int deleteIndex_ = 0;
	int updateIndex_ = 0;
	int updateValue_ = 0;
	int searchValue_ = 0;
	bool autoplay_ = false;
	PlaybackMode playbackMode_ = PlaybackMode::StepByStep;
	float playbackSpeed_ = 1.0f;
	float scrollOffset_ = 0.0f;
};

inline SinglyLinkedListUI singly_linked_list_ui;	
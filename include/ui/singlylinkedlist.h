#pragma once

#include <array>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <logic/singlylinkedlist.h>

enum class PlaybackMode {
	StepByStep,
	RunAtOnce
};

class SinglyLinkedListUI {
public:
	SinglyLinkedListUI();

	void drawSfml(sf::RenderWindow& window);
	void draw();
private:
	std::array<char, 256> txtPath_{};
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
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
	bool showCodeOverlay_ = true;
	bool isCanvasDragging_ = false;
	sf::Vector2i lastDragMousePos_{0, 0};
};

inline SinglyLinkedListUI singly_linked_list_ui;	
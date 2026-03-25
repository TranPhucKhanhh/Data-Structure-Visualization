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
	std::array<char, 512> userDefinedList_{};
	bool userDefinedListExpanded_ = false;

	int addIndex_ = 0;
	int addValue_ = 0;
	int deleteIndex_ = 0;
	int updateIndex_ = 0;
	int updateValue_ = 0;
	int searchValue_ = 0;
	bool autoplay_ = true;
	PlaybackMode playbackMode_ = PlaybackMode::RunAtOnce;
	float playbackSpeed_ = 1.0f;
	float scrollOffset_ = 0.0f;
	float canvasOffsetY_ = 0.0f;
	float zoomScale_ = 1.0f;
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
	int visualStylePreset_ = 0;
	sf::Color canvasBgColor_{255, 255, 255, 255};
	sf::Color nodeBaseColor_{230, 230, 230, 255};
	sf::Color activeNodeColor_{245, 158, 11, 255};
	sf::Color secondaryNodeColor_{156, 163, 175, 255};
	sf::Color deleteNodeColor_{239, 68, 68, 255};
	sf::Color edgeColor_{70, 70, 70, 255};
	sf::Color valueTextColor_{42, 42, 42, 255};
	sf::Color indexTextColor_{68, 68, 68, 255};
	sf::Color highlightRingColor_{255, 214, 102, 230};
	bool showCodeOverlay_ = true;
	bool operationPanelCollapsed_ = false;
	int operationMenuIndex_ = 0;
	bool commentPanelCollapsed_ = false;
	bool codePanelCollapsed_ = false;
	float operationPanelOpenT_ = 1.0f;
	float commentPanelOpenT_ = 1.0f;
	float codePanelOpenT_ = 1.0f;
	bool isCanvasDragging_ = false;
	bool isEditingUserDefinedInput_ = false;
	sf::Vector2i lastDragMousePos_{0, 0};
};

inline SinglyLinkedListUI singly_linked_list_ui;	
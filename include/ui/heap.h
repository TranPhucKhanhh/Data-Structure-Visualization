#pragma once

#include <array>
#include <string>
#include <vector>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <imgui.h>

#include <logic/heap.h>
#include <ui/common.h>

enum class HeapPlaybackMode {
	StepByStep,
	RunAtOnce
};

class HeapUI {
public:
	HeapUI();

	void draw();
	void drawSfml(sf::RenderWindow& window);

private:
	std::array<char, 256> txtPath_{};
	std::array<char, 512> createValues_{};
	int randomCount_ = 10;
	int insertValue_ = 0;
	int searchValue_ = 0;
	int updateOldValue_ = 0;
	int updateNewValue_ = 0;

	std::vector<HeapInstruction> currentSteps_;
	std::vector<int> timelineBaseHeap_;
	std::vector<int> timelineHeap_;
	int currentStepIndex_ = 0;
	int displayCursorIndex_ = 0;
	int activeIndex_ = -1;
	int secondaryIndex_ = -1;
	int lastOperationMenuIndex_ = -1;
	float autoplayAccumulator_ = 0.0f;
	std::string operationResult_ = "Ready";

	bool autoplay_ = true;
	HeapPlaybackMode playbackMode_ = HeapPlaybackMode::RunAtOnce;
	float playbackSpeed_ = 1.0f;

	bool stepTransitioning_ = false;
	int transitionFromStep_ = 0;
	int transitionToStep_ = 0;
	float stepTransitionProgress_ = 1.0f;
	float stepTransitionDuration_ = 0.28f;

	float scrollOffsetX_ = 0.0f;
	float scrollOffsetY_ = 0.0f;
	float zoomScale_ = 1.0f;
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
	bool showCodeOverlay_ = true;

	bool operationPanelCollapsed_ = false;
	bool commentPanelCollapsed_ = false;
	bool codePanelCollapsed_ = false;
	float operationPanelOpenT_ = 1.0f;
	float commentPanelOpenT_ = 1.0f;
	float codePanelOpenT_ = 1.0f;
	int operationMenuIndex_ = 0;

	bool isCanvasDragging_ = false;
	sf::Vector2i lastDragMousePos_{0, 0};

	sf::Color canvasBgColor_{255, 255, 255, 255};
	sf::Color nodeBaseColor_{230, 230, 230, 255};
	sf::Color activeNodeColor_{245, 158, 11, 255};
	sf::Color secondaryNodeColor_{156, 163, 175, 255};
	sf::Color deleteNodeColor_{239, 68, 68, 255};
	sf::Color edgeColor_{70, 70, 70, 255};
	sf::Color valueTextColor_{42, 42, 42, 255};
	sf::Color indexTextColor_{68, 68, 68, 255};
	sf::Color highlightRingColor_{255, 214, 102, 230};

	void rebuildViewFromStep(int stepIndex);
	void startTimeline(std::vector<HeapInstruction>&& steps, const std::vector<int>& baseData, int sourceMenuIndex, const std::string& fallbackMessage);
	void startStepTransition(int targetStep);
};

inline HeapUI heap_ui;

#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <string>
#include <vector>

#include <imgui.h>

#include <logic/shortestpath.h>
#include <ui/common.h>

enum class ShortestPathPlaybackMode {
	StepByStep,
	RunAtOnce
};

class ShortestPathUI {
public:
	ShortestPathUI();

	void draw();
	void drawSfml(sf::RenderWindow& window);
private:
    std::array<char, 1024> txtPath_{};
	std::array<char, 1024> edgeInput_{};
	std::vector<std::array<int, 3>> graphEdges_;
	std::vector<ShortestPathInstruction> currentSteps_;
	std::vector<int> pathNodes_;
	int currentStepIndex_ = 0;
	int lastOperationMenuIndex_ = -1;
	float autoplayAccumulator_ = 0.0f;
	std::string statusMessage_ = "Ready";
	std::string resultMessage_ = "Load a graph and run Dijkstra to see the result.";
	int startNode_ = 0;
	int endNode_ = 3;
	int vertexCount_ = 0;
	int edgeCount_ = 0;
	bool graphLoaded_ = false;
	bool pathFound_ = false;
	int operationMenuIndex_ = 0;
	bool autoplay_ = true;
	ShortestPathPlaybackMode playbackMode_ = ShortestPathPlaybackMode::RunAtOnce;
	float playbackSpeed_ = 1.0f;

	bool stepTransitioning_ = false;
	int transitionFromStep_ = 0;
	int transitionToStep_ = 0;
	float stepTransitionProgress_ = 1.0f;
	float stepTransitionDuration_ = 0.32f;

	bool initializeAnimating_ = false;
	float initializeAnimationProgress_ = 1.0f;
	float initializeAnimationDuration_ = 0.75f;

	bool operationPanelCollapsed_ = false;
	bool commentPanelCollapsed_ = false;
	bool codePanelCollapsed_ = false;
	float operationPanelOpenT_ = 1.0f;
	float commentPanelOpenT_ = 1.0f;
	float codePanelOpenT_ = 1.0f;
	float scrollOffsetX_ = 0.0f;
	float scrollOffsetY_ = 0.0f;
	float zoomScale_ = 1.0f;
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
	std::vector<sf::Vector2f> randomNodeOffsets_;
	bool randomLayoutDirty_ = true;
	sf::Vector2u randomLayoutCanvasSize_{0u, 0u};
	std::vector<sf::Vector2f> forceNodePositions_;
	std::vector<sf::Vector2f> forceNodeVelocities_;
	bool forceLayoutDirty_ = true;
	sf::Vector2u forceLayoutCanvasSize_{0u, 0u};
	float forceRepelK_ = 2400000.0f;
	float forceSpringKMin_ = 0.45f;
	float forceSpringKMax_ = 1.35f;
	float forceCenterK_ = 2.0f;
	float forceDamping_ = 3.0f;
	float forceMaxSpeed_ = 700.0f;
	float forcePadding_ = 36.0f;

	sf::Color canvasBgColor_{255, 255, 255, 255};
	sf::Color nodeBaseColor_{230, 230, 230, 255};
	sf::Color activeNodeColor_{245, 158, 11, 255};
	sf::Color settledNodeColor_{156, 163, 175, 255};
	sf::Color pathNodeColor_{96, 165, 250, 255};
	sf::Color edgeColor_{70, 70, 70, 255};
	sf::Color relaxEdgeColor_{245, 158, 11, 255};
	sf::Color pathEdgeColor_{34, 197, 94, 255};
	sf::Color valueTextColor_{42, 42, 42, 255};
	sf::Color highlightRingColor_{255, 214, 102, 230};

	void startTimeline(std::vector<ShortestPathInstruction>&& steps, int sourceMenuIndex, const std::string& fallbackMessage);
	void startStepTransition(int targetStep);
	void startInitializeAnimation();
	void ensureRandomNodeLayout(const sf::Vector2u& canvasSize);
	void ensureForceLayoutState(const sf::Vector2u& canvasSize);

	//Helper function to sync graphEdges_ from Logic to UI, to replace parseEdges function
	void syncVisualEdges();
};

inline ShortestPathUI shortest_path_ui;

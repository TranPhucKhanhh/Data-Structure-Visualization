#pragma once

#include <array>
#include <string>
#include <vector>

#include <imgui.h>

#include <logic/shortestpath.h>
#include <ui/common.h>

class ShortestPathUI {
public:
	ShortestPathUI();

	void draw();
private:
	std::array<char, 1024> edgeInput_{};
	std::vector<std::array<int, 3>> graphEdges_;
	std::vector<int> pathNodes_;
	std::string statusMessage_ = "Ready";
	std::string resultMessage_ = "Load a graph and run Dijkstra to see the result.";
	int startNode_ = 0;
	int endNode_ = 3;
	int vertexCount_ = 0;
	int edgeCount_ = 0;
	bool graphLoaded_ = false;
	bool pathFound_ = false;
	int operationMenuIndex_ = 0;
	bool operationPanelCollapsed_ = false;
	bool commentPanelCollapsed_ = false;
	bool codePanelCollapsed_ = false;
	float operationPanelOpenT_ = 1.0f;
	float commentPanelOpenT_ = 1.0f;
	float codePanelOpenT_ = 1.0f;
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
};

inline ShortestPathUI shortest_path_ui;	
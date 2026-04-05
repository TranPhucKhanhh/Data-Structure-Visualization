#include <ui/shortestpath.h>

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <limits>
#include <sstream>
#include <string>

namespace {
	float lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	const char* kCreateCode[] = {
		"1  FUNCTION loadGraph(edges):",
		"2      parse each edge as (u, v, w)",
		"3      store edges in an adjacency list",
		"4      update vertex count"
	};

	const char* kRunCode[] = {
		"1  FUNCTION dijkstra(start, end):",
		"2      initialize distances and priority queue",
		"3      visit the cheapest node",
		"4      relax each outgoing edge",
		"5      stop when end is reached"
	};

	const char* kSettingsCode[] = {
		"1  FUNCTION updateView():",
		"2      adjust node radius and edge thickness",
		"3      keep the layout readable"
	};

	void pickCodeBlock(int menuIndex, const char**& codeArray, int& lineCount, const char*& title) {
		switch (menuIndex) {
		case 0:
			codeArray = kCreateCode;
			lineCount = 4;
			title = "GRAPH";
			break;
		case 1:
			codeArray = kRunCode;
			lineCount = 5;
			title = "DIJKSTRA";
			break;
		case 2:
			codeArray = kSettingsCode;
			lineCount = 3;
			title = "SETTINGS";
			break;
		default:
			codeArray = nullptr;
			lineCount = 0;
			title = "SHORTEST PATH";
			break;
		}
	}

	std::string instructionToComment(int menuIndex, const std::string& fallbackMessage) {
		switch (menuIndex) {
		case 0:
			return "Input weighted edges and prepare the graph.";
		case 1:
			return fallbackMessage.empty() ? "Run Dijkstra to compute the shortest route." : fallbackMessage;
		case 2:
			return "Adjust view settings for readability.";
		default:
			return fallbackMessage.empty() ? "Ready" : fallbackMessage;
		}
	}

	std::string normalizeEdgeInput(std::string raw) {
		for (char& c : raw) {
			switch (c) {
			case ',':
			case ';':
			case '|':
			case '(':
			case ')':
			case '[':
			case ']':
			case '{':
			case '}':
				c = ' ';
				break;
			default:
				break;
			}
		}
		return raw;
	}

	std::vector<std::array<int, 3>> parseEdges(const char* rawText, int& vertexCount) {
		std::vector<std::array<int, 3>> edges;
		vertexCount = 0;
		if (rawText == nullptr) {
			return edges;
		}

		std::stringstream ss(normalizeEdgeInput(rawText));
		int u = 0;
		int v = 0;
		int w = 0;
		while (ss >> u >> v >> w) {
			edges.push_back({ u, v, w });
			vertexCount = std::max(vertexCount, std::max(u, v) + 1);
		}

		return edges;
	}

	int lookupEdgeWeight(const std::vector<std::array<int, 3>>& edges, int a, int b) {
		int bestWeight = std::numeric_limits<int>::max();
		bool found = false;
		for (const auto& edge : edges) {
			const int u = edge[0];
			const int v = edge[1];
			const int w = edge[2];
			if ((u == a && v == b) || (u == b && v == a)) {
				bestWeight = std::min(bestWeight, w);
				found = true;
			}
		}
		return found ? bestWeight : -1;
	}

	std::string formatPath(const std::vector<int>& nodes) {
		if (nodes.empty()) {
			return "-";
		}

		std::string out;
		for (std::size_t i = 0; i < nodes.size(); ++i) {
			if (i != 0) {
				out += " -> ";
			}
			out += std::to_string(nodes[i]);
		}
		return out;
	}

	std::vector<int> extractPathFromSteps(const std::vector<ShortestPathInstruction>& steps) {
		std::vector<int> path;
		for (const auto& step : steps) {
			if (step.op_type == ShortestPathOp::FOUND_PATH && step.node_u >= 0) {
				path.push_back(step.node_u);
			}
		}
		std::reverse(path.begin(), path.end());
		return path;
	}
}

ShortestPathUI::ShortestPathUI() {
	const char* sampleGraph =
		"0 1 4\n"
		"0 2 1\n"
		"2 1 2\n"
		"1 3 1\n"
		"2 3 5\n"
		"3 4 3";
	std::snprintf(edgeInput_.data(), edgeInput_.size(), "%s", sampleGraph);
	graphEdges_ = parseEdges(edgeInput_.data(), vertexCount_);
	edgeCount_ = static_cast<int>(graphEdges_.size());
	graphLoaded_ = edgeCount_ > 0;
	startNode_ = 0;
	endNode_ = std::max(0, vertexCount_ - 1);
	statusMessage_ = "Sample graph is ready.";
	resultMessage_ = "Press Run Dijkstra to preview the shortest path.";
}

void ShortestPathUI::draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vpPos = viewport->Pos;
	const ImVec2 vpSize = viewport->Size;
	const float foldLerp = 1.0f - std::exp(-14.0f * ImGui::GetIO().DeltaTime);
	operationPanelOpenT_ = std::clamp(lerp(operationPanelOpenT_, operationPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
	commentPanelOpenT_ = std::clamp(lerp(commentPanelOpenT_, commentPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
	codePanelOpenT_ = std::clamp(lerp(codePanelOpenT_, codePanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);

	ImGui::SetNextWindowPos(vpPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(vpSize, ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.98f, 0.99f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (!ImGui::Begin("Shortest Path Visualizer##SP", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse)) {
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.06f, 0.98f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.24f, 0.33f, 0.65f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.34f, 0.46f, 0.80f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.84f, 0.88f, 0.94f, 1.0f));
	ImGui::SetNextWindowPos(vpPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(vpSize.x, 44.0f), ImGuiCond_Always);
	if (ImGui::Begin("##ShortestPathTopBar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(8.0f);
		if (ImGui::Button("MAIN MENU", ImVec2(112.0f, 26.0f))) {
			uiConfig.state = UIState::Menu;
		}
		ImGui::SameLine();
		ImGui::TextUnformatted("Shortest Path");
		ImGui::SameLine();
		ImGui::TextDisabled("Basic graph setup and result preview");
	}
	ImGui::End();
	ImGui::PopStyleColor(5);
	ImGui::PopStyleVar();

	const float drawerBottomY = vpPos.y + vpSize.y - 280.0f;
	ImGui::SetNextWindowPos(ImVec2(vpPos.x, drawerBottomY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(48.0f, 188.0f), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
	if (ImGui::Begin("Operation Toggle##SP", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings)) {
		ImGui::SetCursorPosY(84.0f);
		if (ImGui::Button(operationPanelCollapsed_ ? ">" : "<", ImVec2(34.0f, 32.0f))) {
			operationPanelCollapsed_ = !operationPanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	const float operationPanelWidth = 176.0f * operationPanelOpenT_;
	if (operationPanelWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(vpPos.x + 48.0f, drawerBottomY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(operationPanelWidth, 188.0f), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
		if (ImGui::Begin("Operations##SP", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar)) {
			if (menuCardTitleFont != nullptr) {
				ImGui::PushFont(menuCardTitleFont);
			}
			if (operationPanelOpenT_ > 0.6f) {
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.14f, 0.48f, 0.22f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.58f, 0.30f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.12f, 0.42f, 0.20f, 0.98f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 9.0f));
				const char* operationNames[] = { "Graph", "Run", "Settings" };
				const float menuRowWidth = ImGui::GetContentRegionAvail().x;
				for (int i = 0; i < 3; ++i) {
					if (ImGui::Selectable(operationNames[i], operationMenuIndex_ == i, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(menuRowWidth, 0.0f))) {
						operationMenuIndex_ = i;
					}
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor(3);
			}
			if (menuCardTitleFont != nullptr) {
				ImGui::PopFont();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();

		const float inputPanelWidth = 660.0f * operationPanelOpenT_;
		const float inputPanelHeight = 188.0f;
		const float inputPanelX = vpPos.x + 48.0f + operationPanelWidth + 2.0f;
		ImGui::SetNextWindowPos(ImVec2(inputPanelX, drawerBottomY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(inputPanelWidth, inputPanelHeight), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
		if (ImGui::Begin("Operation Inputs##SP", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar)) {
			if (menuCardDescFont != nullptr) {
				ImGui::PushFont(menuCardDescFont);
			}

			if (operationMenuIndex_ == 0) {
				ImGui::TextUnformatted("Graph Input");
				ImGui::TextWrapped("Enter each edge as three numbers: u v w. Example: 0 1 4");
				ImGui::InputTextMultiline("##SPEdgeInput", edgeInput_.data(), edgeInput_.size(), ImVec2(-1.0f, 118.0f));
				ImGui::InputInt("Start node", &startNode_);
				ImGui::SameLine();
				ImGui::InputInt("End node", &endNode_);
			}
			else if (operationMenuIndex_ == 1) {
				ImGui::TextUnformatted("Build / Run");
				ImGui::TextWrapped("Use the current input to build the graph or run Dijkstra right away.");
				if (ImGui::Button("Use sample graph", ImVec2(170.0f, 0.0f))) {
					const char* sampleGraph =
						"0 1 4\n"
						"0 2 1\n"
						"2 1 2\n"
						"1 3 1\n"
						"2 3 5\n"
						"3 4 3";
					std::snprintf(edgeInput_.data(), edgeInput_.size(), "%s", sampleGraph);
					graphEdges_ = parseEdges(edgeInput_.data(), vertexCount_);
					edgeCount_ = static_cast<int>(graphEdges_.size());
					graphLoaded_ = edgeCount_ > 0;
					startNode_ = 0;
					endNode_ = std::max(0, vertexCount_ - 1);
					pathNodes_.clear();
					pathFound_ = false;
					statusMessage_ = "Sample graph loaded.";
					resultMessage_ = "Press Run Dijkstra to preview the shortest path.";
				}
				ImGui::SameLine();
				if (ImGui::Button("Build graph", ImVec2(120.0f, 0.0f))) {
					graphEdges_ = parseEdges(edgeInput_.data(), vertexCount_);
					edgeCount_ = static_cast<int>(graphEdges_.size());
					graphLoaded_ = edgeCount_ > 0;
					pathNodes_.clear();
					pathFound_ = false;
					if (graphLoaded_) {
						statusMessage_ = "Graph loaded successfully.";
						resultMessage_ = "Graph is ready. Switch to Run Dijkstra to see the result.";
						startNode_ = std::clamp(startNode_, 0, std::max(0, vertexCount_ - 1));
						endNode_ = std::clamp(endNode_, 0, std::max(0, vertexCount_ - 1));
					}
					else {
						statusMessage_ = "No valid edges were found in the input.";
						resultMessage_ = "Check the edge format and try again.";
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Run Dijkstra", ImVec2(120.0f, 0.0f))) {
					graphEdges_ = parseEdges(edgeInput_.data(), vertexCount_);
					edgeCount_ = static_cast<int>(graphEdges_.size());
					graphLoaded_ = edgeCount_ > 0;
					pathNodes_.clear();
					pathFound_ = false;

					if (!graphLoaded_) {
						statusMessage_ = "No valid graph to run on.";
						resultMessage_ = "Build a graph first.";
					}
					else if (startNode_ < 0 || endNode_ < 0 || startNode_ >= vertexCount_ || endNode_ >= vertexCount_) {
						statusMessage_ = "Start or end node is outside the graph.";
						resultMessage_ = "Adjust the node indices and try again.";
					}
					else {
						shortestPath.clear();
						for (const auto& edge : graphEdges_) {
							shortestPath.addEdge(edge[0], edge[1], edge[2]);
						}

						const std::vector<ShortestPathInstruction> steps = shortestPath.dijkstraStep(startNode_, endNode_);
						pathNodes_ = extractPathFromSteps(steps);
						pathFound_ = !pathNodes_.empty();

						if (pathFound_) {
							int totalWeight = 0;
							bool pathComplete = true;
							for (std::size_t i = 1; i < pathNodes_.size(); ++i) {
								const int weight = lookupEdgeWeight(graphEdges_, pathNodes_[i - 1], pathNodes_[i]);
								if (weight < 0) {
									pathComplete = false;
									break;
								}
								totalWeight += weight;
							}

							if (pathComplete) {
								statusMessage_ = "Dijkstra finished successfully.";
								resultMessage_ = std::string("Shortest path: ") + formatPath(pathNodes_) + " | Total weight: " + std::to_string(totalWeight);
							}
							else {
								statusMessage_ = "Dijkstra finished, but the path preview could not be resolved completely.";
								resultMessage_ = std::string("Path: ") + formatPath(pathNodes_);
							}
						}
						else {
							statusMessage_ = "No path was found between the selected nodes.";
							resultMessage_ = "The graph may be disconnected or the nodes are unreachable.";
						}
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear graph", ImVec2(120.0f, 0.0f))) {
					edgeInput_[0] = '\0';
					graphEdges_.clear();
					pathNodes_.clear();
					vertexCount_ = 0;
					edgeCount_ = 0;
					graphLoaded_ = false;
					pathFound_ = false;
					statusMessage_ = "Graph cleared.";
					resultMessage_ = "Enter a new graph and run Dijkstra again.";
					shortestPath.clear();
				}
			}
			else {
				ImGui::TextUnformatted("Settings");
				ImGui::TextWrapped("These controls only change the visual size of the panels.");
				ImGui::SliderFloat("Node radius##sp", &nodeRadius_, 16.0f, 48.0f, "%.1f");
				ImGui::SliderFloat("Edge thickness##sp", &edgeThickness_, 1.0f, 8.0f, "%.1f");
				ImGui::SliderFloat("Font scale##sp", &fontScale_, 0.75f, 1.8f, "%.2f");
			}

			if (menuCardDescFont != nullptr) {
				ImGui::PopFont();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	const float commentY = vpPos.y + vpSize.y - 430.0f;
	const float commentH = 108.0f;
	const float codeY = vpPos.y + vpSize.y - 285.0f;
	const float codeH = 155.0f;
	const float rightTabWidth = 26.0f;
	const float rightPanelWidth = 420.0f;
	const float rightTabX = vpPos.x + vpSize.x - rightTabWidth;

	const std::string currentComment = instructionToComment(operationMenuIndex_, statusMessage_);

	const float animatedCommentWidth = rightPanelWidth * commentPanelOpenT_;
	if (animatedCommentWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(rightTabX - animatedCommentWidth, commentY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(animatedCommentWidth, commentH), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.55f, 0.16f, 0.96f));
		if (ImGui::Begin("Traversal Comment##SPComment", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar)) {
			if (commentPanelOpenT_ > 0.55f) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.14f, 0.10f, 0.07f, 1.0f));
				ImGui::TextWrapped("%s", currentComment.c_str());
				ImGui::PopStyleColor();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	ImGui::SetNextWindowPos(ImVec2(rightTabX, commentY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(rightTabWidth, commentH), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.55f, 0.16f, 0.96f));
	if (ImGui::Begin("Traversal Comment Toggle##SPCommentToggle", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(commentH * 0.5f - 12.0f);
		if (ImGui::Button(commentPanelCollapsed_ ? "<" : ">", ImVec2(18.0f, 24.0f))) {
			commentPanelCollapsed_ = !commentPanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	const float animatedCodeWidth = rightPanelWidth * codePanelOpenT_;
	if (animatedCodeWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(rightTabX - animatedCodeWidth, codeY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(animatedCodeWidth, codeH), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.78f, 0.08f, 0.96f));
		if (ImGui::Begin("Source Code##SPCode", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar)) {
			if (codePanelOpenT_ > 0.55f) {
				const char** codeArray = nullptr;
				int lineCount = 0;
				const char* opTitle = "SHORTEST PATH";
				pickCodeBlock(operationMenuIndex_, codeArray, lineCount, opTitle);
				if (codeArray != nullptr && lineCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.10f, 0.05f, 1.0f));
					ImGui::Text("Operation: %s", opTitle);
					ImGui::Separator();
					for (int i = 0; i < lineCount; ++i) {
						ImGui::TextUnformatted(codeArray[i]);
					}
					ImGui::PopStyleColor();
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	ImGui::SetNextWindowPos(ImVec2(rightTabX, codeY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(rightTabWidth, codeH), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.78f, 0.08f, 0.96f));
	if (ImGui::Begin("Source Code Toggle##SPCodeToggle", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(codeH * 0.5f - 12.0f);
		if (ImGui::Button(codePanelCollapsed_ ? "<" : ">", ImVec2(18.0f, 24.0f))) {
			codePanelCollapsed_ = !codePanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::SetNextWindowPos(ImVec2(vpPos.x, vpPos.y + vpSize.y - 58.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(vpSize.x, 48.0f), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.03f, 0.03f, 0.98f));
	if (ImGui::Begin("Playback##SPBottom", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::TextDisabled("Basic interface only. Use the panels above to input graph data and run Dijkstra.");
	}
	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}
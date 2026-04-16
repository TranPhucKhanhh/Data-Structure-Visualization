#include <ui/shortestpath.h>

#include <imgui.h>

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "utils/SimpleFileDialog.h"

namespace {
//	struct SPVisualState {
//		std::vector<int> distances;
//		std::vector<int> parent;
//		std::vector<bool> settled;
//		std::vector<bool> inPath;
//		int activeNode = -1;
//		int relaxU = -1;
//		int relaxV = -1;
//		bool noPath = false;
//	};

	float lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	float easeInOut(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	sf::Color blendColor(const sf::Color& a, const sf::Color& b, float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		auto blend = [&](std::uint8_t c0, std::uint8_t c1) -> std::uint8_t {
			return static_cast<std::uint8_t>(std::clamp(lerp(static_cast<float>(c0), static_cast<float>(c1), t), 0.0f, 255.0f));
		};
		return sf::Color(blend(a.r, b.r), blend(a.g, b.g), blend(a.b, b.b), blend(a.a, b.a));
	}

	const sf::Font* getShortestPathFont() {
		static sf::Font font;
		static bool attempted = false;
		static bool loaded = false;

		if (!attempted) {
			attempted = true;
			const std::string fontName = "/DroidSans.ttf";
			const auto path = std::filesystem::path(std::string(ASSET_FONT + fontName));
			if (std::filesystem::exists(path)) {
				if (font.openFromFile(path)) {
					loaded = true;
				}
			}
		}

		return loaded ? &font : nullptr;
	}

	const char* kGraphCode[] = {
		"1  FUNCTION loadGraph(edges):",
		"2      parse each edge as (u, v, w)",
		"3      store edges in adjacency list",
		"4      validate start/end node range"
	};

	const char* kRunCode[] = {
		"1  FUNCTION dijkstra(start, end):",
		"2      initialize dist[] = INF, parent[] = -1",
		"3      pop the node with smallest tentative distance",
		"4      relax each outgoing edge",
		"5      update distance and parent when better",
		"6      reconstruct path from end"
	};

	const char* kSettingsCode[] = {
		"1  FUNCTION updateView():",
		"2      adjust node radius / edge thickness",
		"3      pan with drag and zoom with mouse wheel"
	};

	void pickCodeBlock(int menuIndex, const char**& codeArray, int& lineCount, const char*& title) {
		switch (menuIndex) {
		case 0:
			codeArray = kGraphCode;
			lineCount = 4;
			title = "GRAPH";
			break;
		case 1:
			codeArray = kRunCode;
			lineCount = 6;
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

	int mapInstructionToCodeLine(const ShortestPathInstruction* instruction, int menuIndex, bool finished) {
		if (menuIndex != 1) {
			return 1;
		}

		if (finished) {
			return 6;
		}

		if (instruction == nullptr) {
			return 2;
		}

		switch (instruction->op_type) {
		case ShortestPathOp::HIGHLIGHT_NODE:
		case ShortestPathOp::MARK_PERMANENT:
			return 3;
		case ShortestPathOp::RELAX_EDGE:
			return 4;
		case ShortestPathOp::UPDATE_DISTANCE:
			return 5;
		case ShortestPathOp::FOUND_PATH:
		case ShortestPathOp::NOT_FOUND:
			return 6;
		default:
			return 2;
		}
	}

	std::string instructionToComment(const ShortestPathInstruction* instruction, const std::string& fallbackMessage) {
		if (instruction == nullptr) {
			return fallbackMessage.empty() ? "Ready" : fallbackMessage;
		}

		switch (instruction->op_type) {
		case ShortestPathOp::HIGHLIGHT_NODE:
			return "Processing node " + std::to_string(instruction->node_u);
		case ShortestPathOp::RELAX_EDGE:
			return "Relax edge " + std::to_string(instruction->node_u) + " -> " + std::to_string(instruction->node_v) +
				" (w=" + std::to_string(instruction->weight) + ")";
		case ShortestPathOp::UPDATE_DISTANCE:
			return "Update dist[" + std::to_string(instruction->node_u) + "] = " + std::to_string(instruction->weight);
		case ShortestPathOp::MARK_PERMANENT:
			return "Mark node " + std::to_string(instruction->node_u) + " as permanent";
		case ShortestPathOp::FOUND_PATH:
			return "Path reconstruction visits node " + std::to_string(instruction->node_u);
		case ShortestPathOp::NOT_FOUND:
			return "No path exists between the selected nodes";
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

//	std::vector<std::array<int, 3>> buildRandomPositiveGraph(int& vertexCount) {
//		std::random_device rd;
//		std::mt19937 rng(rd());
//		std::uniform_int_distribution<int> vertexDist(5, 10);
//		std::uniform_int_distribution<int> weightDist(1, 20);
//
//		vertexCount = vertexDist(rng);
//		const int maxEdges = vertexCount * (vertexCount - 1) / 2;
//		const int minEdges = vertexCount - 1;
//		const int desiredUpperBound = std::min(maxEdges, vertexCount + vertexCount / 2 + 2);
//		std::uniform_int_distribution<int> edgeDist(minEdges, std::max(minEdges, desiredUpperBound));
//		const int targetEdges = edgeDist(rng);
//
//		std::vector<std::array<int, 3>> edges;
//		edges.reserve(static_cast<std::size_t>(targetEdges));
//		std::set<std::pair<int, int>> used;
//
//		std::vector<int> order(static_cast<std::size_t>(vertexCount));
//		for (int i = 0; i < vertexCount; ++i) {
//			order[static_cast<std::size_t>(i)] = i;
//		}
//		std::shuffle(order.begin(), order.end(), rng);
//
//		for (int i = 1; i < vertexCount; ++i) {
//			const int u = order[static_cast<std::size_t>(i)];
//			std::uniform_int_distribution<int> parentDist(0, i - 1);
//			const int v = order[static_cast<std::size_t>(parentDist(rng))];
//			const int a = std::min(u, v);
//			const int b = std::max(u, v);
//			used.insert({ a, b });
//			edges.push_back({ u, v, weightDist(rng) });
//		}
//
//		std::uniform_int_distribution<int> nodeDist(0, vertexCount - 1);
//		while (static_cast<int>(edges.size()) < targetEdges) {
//			const int u = nodeDist(rng);
//			const int v = nodeDist(rng);
//			if (u == v) {
//				continue;
//			}
//			const int a = std::min(u, v);
//			const int b = std::max(u, v);
//			if (used.find({ a, b }) != used.end()) {
//				continue;
//			}
//			used.insert({ a, b });
//			edges.push_back({ u, v, weightDist(rng) });
//		}
//
//		return edges;
//	}

	std::string edgesToInputText(const std::vector<std::array<int, 3>>& edges) {
		std::ostringstream oss;
		for (std::size_t i = 0; i < edges.size(); ++i) {
			const auto& edge = edges[i];
			oss << edge[0] << ' ' << edge[1] << ' ' << edge[2];
			if (i + 1 < edges.size()) {
				oss << '\n';
			}
		}
		return oss.str();
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

	SPVisualState buildVisualState(const std::vector<ShortestPathInstruction>& steps, int appliedCount, int vertexCount) {
		SPVisualState state;
		state.distances.assign(static_cast<std::size_t>(std::max(0, vertexCount)), std::numeric_limits<int>::max());
		state.parent.assign(static_cast<std::size_t>(std::max(0, vertexCount)), -1);
		state.settled.assign(static_cast<std::size_t>(std::max(0, vertexCount)), false);
		state.inPath.assign(static_cast<std::size_t>(std::max(0, vertexCount)), false);

		const int count = std::clamp(appliedCount, 0, static_cast<int>(steps.size()));
		for (int i = 0; i < count; ++i) {
			const ShortestPathInstruction& step = steps[static_cast<std::size_t>(i)];
			switch (step.op_type) {
			case ShortestPathOp::HIGHLIGHT_NODE:
				state.activeNode = step.node_u;
				state.relaxU = -1;
				state.relaxV = -1;
				break;
			case ShortestPathOp::RELAX_EDGE:
				state.relaxU = step.node_u;
				state.relaxV = step.node_v;
				state.activeNode = step.node_u;
				break;
			case ShortestPathOp::UPDATE_DISTANCE:
				if (step.node_u >= 0 && step.node_u < static_cast<int>(state.distances.size())) {
					state.distances[static_cast<std::size_t>(step.node_u)] = step.weight;
					if (state.relaxU >= 0 && state.relaxU < static_cast<int>(state.parent.size())) {
						state.parent[static_cast<std::size_t>(step.node_u)] = state.relaxU;
					}
					state.activeNode = step.node_u;
				}
				break;
			case ShortestPathOp::MARK_PERMANENT:
				if (step.node_u >= 0 && step.node_u < static_cast<int>(state.settled.size())) {
					state.settled[static_cast<std::size_t>(step.node_u)] = true;
					state.activeNode = step.node_u;
				}
				break;
			case ShortestPathOp::FOUND_PATH:
				if (step.node_u >= 0 && step.node_u < static_cast<int>(state.inPath.size())) {
					state.inPath[static_cast<std::size_t>(step.node_u)] = true;
					if (step.node_v >= 0) {
						state.parent[static_cast<std::size_t>(step.node_u)] = step.node_v;
						state.relaxU = step.node_v;
						state.relaxV = step.node_u;
					}
					state.activeNode = step.node_u;
				}
				break;
			case ShortestPathOp::NOT_FOUND:
				state.noPath = true;
				break;
			default:
				break;
			}
		}

		return state;
	}

	void drawThickLine(sf::RenderWindow& window, const sf::Vector2f& a, const sf::Vector2f& b, float thickness, const sf::Color& color) {
		sf::Vector2f delta = b - a;
		const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
		if (length <= 0.001f) {
			return;
		}

		sf::RectangleShape line(sf::Vector2f(length, std::max(1.0f, thickness)));
		line.setFillColor(color);
		line.setOrigin(sf::Vector2f(0.0f, line.getSize().y * 0.5f));
		line.setPosition(a);
		line.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
		window.draw(line);
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

void ShortestPathUI::startTimeline(std::vector<ShortestPathInstruction>&& steps, int sourceMenuIndex, const std::string& fallbackMessage) {
	currentSteps_ = std::move(steps);
	currentStepIndex_ = 0;
	transitionFromStep_ = 0;
	transitionToStep_ = 0;
	stepTransitioning_ = false;
	stepTransitionProgress_ = 1.0f;
	autoplayAccumulator_ = 0.0f;
	autoplay_ = true;
	playbackMode_ = ShortestPathPlaybackMode::RunAtOnce;
	lastOperationMenuIndex_ = sourceMenuIndex;
	if (!fallbackMessage.empty()) {
		statusMessage_ = fallbackMessage;
	}
}

void ShortestPathUI::startStepTransition(int targetStep) {
	const int clampedTarget = std::clamp(targetStep, 0, static_cast<int>(currentSteps_.size()));
	if (clampedTarget == currentStepIndex_) {
		stepTransitioning_ = false;
		stepTransitionProgress_ = 1.0f;
		transitionFromStep_ = currentStepIndex_;
		transitionToStep_ = currentStepIndex_;
		return;
	}

	transitionFromStep_ = currentStepIndex_;
	transitionToStep_ = clampedTarget;
	stepTransitionProgress_ = 0.0f;
	stepTransitioning_ = true;
}

void ShortestPathUI::startInitializeAnimation() {
	initializeAnimating_ = true;
	initializeAnimationProgress_ = 0.0f;
}

void ShortestPathUI::draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vpPos = viewport->Pos;
	const ImVec2 vpSize = viewport->Size;
	const float dt = ImGui::GetIO().DeltaTime;
	const float foldLerp = 1.0f - std::exp(-14.0f * dt);
	operationPanelOpenT_ = std::clamp(lerp(operationPanelOpenT_, operationPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
	commentPanelOpenT_ = std::clamp(lerp(commentPanelOpenT_, commentPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
	codePanelOpenT_ = std::clamp(lerp(codePanelOpenT_, codePanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);

	ImGui::SetNextWindowPos(vpPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(vpSize, ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	if (!ImGui::Begin("Shortest Path Visualizer##SP", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse |
		ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoInputs |
		ImGuiWindowFlags_NoBringToFrontOnFocus)) {
		ImGui::End();
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.06f, 0.98f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
	ImGui::SetNextWindowPos(vpPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(vpSize.x, 44.0f), ImGuiCond_Always);
	if (ImGui::Begin("##ShortestPathTopBar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.24f, 0.33f, 0.65f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.34f, 0.46f, 0.80f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.84f, 0.88f, 0.94f, 1.0f));

		if (ImGui::Button("MAIN MENU", ImVec2(112.0f, 26.0f))) {
			uiConfig.state = UIState::Menu;
		}

		auto navButton = [&](const char* label, UIState target, bool active) {
			ImGui::SameLine();
			if (active) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.30f, 0.42f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
			}
			if (ImGui::Button(label, ImVec2(132.0f, 26.0f))) {
				uiConfig.state = target;
			}
			if (active) {
				ImGui::PopStyleColor(2);
			}
		};

		navButton("LINKED LIST", UIState::SinglyLinkedList, uiConfig.state == UIState::SinglyLinkedList);
		navButton("TRIE", UIState::Trie, uiConfig.state == UIState::Trie);
		navButton("HEAP", UIState::Heap, uiConfig.state == UIState::Heap);
		navButton("SHORTEST PATH", UIState::ShortestPath, uiConfig.state == UIState::ShortestPath);

		ImGui::PopStyleColor(4);
	}
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	auto resetTimeline = [&]() {
		currentSteps_.clear();
		currentStepIndex_ = 0;
		stepTransitioning_ = false;
		stepTransitionProgress_ = 1.0f;
		autoplay_ = false;
		autoplayAccumulator_ = 0.0f;
		lastOperationMenuIndex_ = -1;
	};

	const float drawerBottomY = vpPos.y + vpSize.y - 300.0f;
	ImGui::SetNextWindowPos(ImVec2(vpPos.x, drawerBottomY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(52.0f, 200.0f), ImGuiCond_Always);
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

	const float operationPanelWidth = 190.0f * operationPanelOpenT_;
	if (operationPanelWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(vpPos.x + 52.0f, drawerBottomY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(operationPanelWidth, 200.0f), ImGuiCond_Always);
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
				const char* operationNames[] = { "Graph", "Run", "Customize" };
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

		const float inputPanelWidth = 700.0f * operationPanelOpenT_;
		const float inputPanelHeight = 200.0f;
		const float inputPanelX = vpPos.x + 52.0f + operationPanelWidth + 2.0f;
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
				ImGui::InputTextMultiline("##SPEdgeInput", edgeInput_.data(), edgeInput_.size(), ImVec2(-1.0f, 108.0f));
				if (ImGui::Button("Random (5-10 nodes)", ImVec2(190.0f, 0.0f))) {
					graphEdges_ = shortestPath.generateRandomGraph(vertexCount_);
					edgeCount_ = static_cast<int>(graphEdges_.size());
					const std::string generatedText = edgesToInputText(graphEdges_);
					std::snprintf(edgeInput_.data(), edgeInput_.size(), "%s", generatedText.c_str());
					graphLoaded_ = edgeCount_ > 0;
					startNode_ = 0;
					endNode_ = std::max(0, vertexCount_ - 1);
					pathNodes_.clear();
					pathFound_ = false;
					statusMessage_ = "Random positive-weight graph generated.";
					resultMessage_ = "Go to Run and press Run Dijkstra to test this graph.";
					startInitializeAnimation();
					resetTimeline();
				}


                //Update the File Browser button for Shortest Path
				if (ImGui::Button("Browse File", ImVec2(120.0f, 0.0f)))
                {
                    std::string selected_path = cr::utils::SimpleFileDialog::dialog();
                    if (!selected_path.empty())
                    {
                        std::snprintf(txtPath_.data(), txtPath_.size(), "%s", selected_path.c_str());
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Load from File", ImVec2(120.0f, 0.0f)))
                {
                    std::cerr << "0\n";
                    statusMessage_ = shortestPath.initFromFile(txtPath_.data());
                    std::cerr << "1\n";
                    if (statusMessage_.find("Success") != std::string::npos)
                    {
                        vertexCount_ = shortestPath.num_vertices;
                        graphLoaded_ = true;

                        std::ifstream file_reader(txtPath_.data());
                        std::string content((std::istreambuf_iterator<char>(file_reader)), std::istreambuf_iterator<char>());
                        std::snprintf(edgeInput_.data(), edgeInput_.size(), "%s", content.c_str());

                        resultMessage_ = "Graph loaded from file. Ready to run Dijkstra.";

                        startNode_ = std::clamp(startNode_, 0, std::max(0, vertexCount_ - 1));
                        endNode_ = std::clamp(endNode_, 0, std::max(0, vertexCount_ - 1));

                        startInitializeAnimation();
                    }
                    else
                    {
                        graphLoaded_ = false;
                        resultMessage_ = "Error: Failed to load graph. Please check the file format.";
                        initializeAnimating_ = false;
                        initializeAnimationProgress_ = 1.0f;
                    }
                }

                ImGui::SameLine();
                ImGui::Text("Path: %s", txtPath_.data());

				ImGui::PushItemWidth(100.0f);
                ImGui::InputInt("Start node", &startNode_);
				ImGui::SameLine();
				ImGui::InputInt("End node", &endNode_);
                ImGui::PopItemWidth();
			}
			else if (operationMenuIndex_ == 1) {
				ImGui::TextUnformatted("Build / Run");
				ImGui::TextWrapped("Use the current input to build the graph or run Dijkstra with timeline animation.");
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
					startInitializeAnimation();
					resetTimeline();
				}
				ImGui::SameLine();
				if (ImGui::Button("Build graph", ImVec2(120.0f, 0.0f))) {
					graphEdges_ = parseEdges(edgeInput_.data(), vertexCount_);
					edgeCount_ = static_cast<int>(graphEdges_.size());
					graphLoaded_ = edgeCount_ > 0;
					pathNodes_.clear();
					pathFound_ = false;
					resetTimeline();
					if (graphLoaded_) {
						statusMessage_ = "Graph loaded successfully.";
						resultMessage_ = "Graph is ready. Press Run Dijkstra.";
						startNode_ = std::clamp(startNode_, 0, std::max(0, vertexCount_ - 1));
						endNode_ = std::clamp(endNode_, 0, std::max(0, vertexCount_ - 1));
						startInitializeAnimation();
					}
					else {
						statusMessage_ = "No valid edges were found in the input.";
						resultMessage_ = "Check the edge format and try again.";
						initializeAnimating_ = false;
						initializeAnimationProgress_ = 1.0f;
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
						resetTimeline();
						initializeAnimating_ = false;
						initializeAnimationProgress_ = 1.0f;
					}
					else if (startNode_ < 0 || endNode_ < 0 || startNode_ >= vertexCount_ || endNode_ >= vertexCount_) {
						statusMessage_ = "Start or end node is outside the graph.";
						resultMessage_ = "Adjust the node indices and try again.";
						resetTimeline();
						initializeAnimating_ = false;
						initializeAnimationProgress_ = 1.0f;
					}
					else {
						shortestPath.clear();
						for (const auto& edge : graphEdges_) {
							shortestPath.addEdge(edge[0], edge[1], edge[2]);
						}

						std::vector<ShortestPathInstruction> steps = shortestPath.dijkstraStep(startNode_, endNode_);
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
								statusMessage_ = "Dijkstra finished, but path preview is incomplete.";
								resultMessage_ = std::string("Path: ") + formatPath(pathNodes_);
							}
						}
						else {
							statusMessage_ = "No path was found between the selected nodes.";
							resultMessage_ = "The graph may be disconnected or unreachable.";
						}

						startTimeline(std::move(steps), 1, statusMessage_);
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
					initializeAnimating_ = false;
					initializeAnimationProgress_ = 1.0f;
					resetTimeline();
				}
			}
			else {
				ImGui::TextUnformatted("Customize");
				ImGui::TextWrapped("Use drag + mouse wheel on canvas. These controls adjust visual size.");
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

	const bool hasTimeline = !currentSteps_.empty();
	const int displayStep = stepTransitioning_ ? transitionToStep_ : currentStepIndex_;
	const ShortestPathInstruction* displayInstruction = (hasTimeline && displayStep > 0)
		? &currentSteps_[static_cast<std::size_t>(displayStep - 1)]
		: nullptr;
	const int panelMenuIndex = hasTimeline && lastOperationMenuIndex_ >= 0 ? lastOperationMenuIndex_ : operationMenuIndex_;
	const bool operationFinished = hasTimeline && displayStep >= static_cast<int>(currentSteps_.size());
	const int highlightedCodeLine = mapInstructionToCodeLine(displayInstruction, panelMenuIndex, operationFinished);

	const std::string currentComment = instructionToComment(displayInstruction, statusMessage_);

	const float commentY = vpPos.y + vpSize.y - 450.0f;
	const float commentH = 115.0f;
	const float codeY = vpPos.y + vpSize.y - 300.0f;
	const float codeH = 170.0f;
	const float rightTabWidth = 26.0f;
	const float rightPanelWidth = 480.0f;
	const float rightTabX = vpPos.x + vpSize.x - rightTabWidth;

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
				ImGui::Separator();
				ImGui::TextWrapped("%s", resultMessage_.c_str());
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
				pickCodeBlock(panelMenuIndex, codeArray, lineCount, opTitle);
				if (codeArray != nullptr && lineCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.10f, 0.05f, 1.0f));
					ImGui::Text("Operation: %s", opTitle);
					ImGui::Separator();
					for (int i = 0; i < lineCount; ++i) {
						if ((i + 1) == highlightedCodeLine) {
							ImGui::TextColored(ImVec4(0.16f, 0.32f, 0.10f, 1.0f), "> %s", codeArray[i]);
						}
						else {
							ImGui::TextUnformatted(codeArray[i]);
						}
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
		ImGui::PushItemWidth(140.0f);
		ImGui::SliderFloat("##SPBottomPlaybackSpeed", &playbackSpeed_, 0.25f, 5.0f, "");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text("%.2gx", playbackSpeed_);

		ImGui::SameLine(vpSize.x * 0.43f);
		if (ImGui::Button("|<")) {
			autoplay_ = false;
			currentStepIndex_ = 0;
			autoplayAccumulator_ = 0.0f;
			stepTransitioning_ = false;
			stepTransitionProgress_ = 1.0f;
		}
		ImGui::SameLine();
		if (ImGui::Button("<")) {
			autoplay_ = false;
			playbackMode_ = ShortestPathPlaybackMode::StepByStep;
			startStepTransition(currentStepIndex_ - 1);
		}
		ImGui::SameLine();
		if (ImGui::Button(autoplay_ ? "[]" : "|>")) {
			autoplay_ = !autoplay_;
			if (autoplay_) {
				playbackMode_ = ShortestPathPlaybackMode::RunAtOnce;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(">")) {
			autoplay_ = false;
			playbackMode_ = ShortestPathPlaybackMode::StepByStep;
			startStepTransition(currentStepIndex_ + 1);
		}
		ImGui::SameLine();
		if (ImGui::Button(">|")) {
			autoplay_ = false;
			currentStepIndex_ = static_cast<int>(currentSteps_.size());
			stepTransitioning_ = false;
			stepTransitionProgress_ = 1.0f;
		}

		if (!currentSteps_.empty()) {
			int frameIndex = displayStep;
			const int maxFrame = static_cast<int>(currentSteps_.size());
			ImGui::SameLine(vpSize.x * 0.58f);
			ImGui::PushItemWidth(vpSize.x * 0.36f);
			if (ImGui::SliderInt("##SPBottomTimeline", &frameIndex, 0, maxFrame, "")) {
				autoplay_ = false;
				startStepTransition(std::clamp(frameIndex, 0, maxFrame));
			}
			ImGui::PopItemWidth();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	if (stepTransitioning_) {
		const float safeDuration = std::max(0.08f, stepTransitionDuration_ / std::max(0.25f, playbackSpeed_));
		stepTransitionProgress_ += dt / safeDuration;
		if (stepTransitionProgress_ >= 1.0f) {
			stepTransitionProgress_ = 1.0f;
			stepTransitioning_ = false;
			currentStepIndex_ = transitionToStep_;
		}
	}

	if (!ImGui::GetIO().WantTextInput && hasTimeline && autoplay_ && playbackMode_ == ShortestPathPlaybackMode::RunAtOnce && currentStepIndex_ < static_cast<int>(currentSteps_.size()) && !stepTransitioning_) {
		autoplayAccumulator_ += dt * playbackSpeed_;
		constexpr float kStepInterval = 0.45f;
		while (autoplayAccumulator_ >= kStepInterval && currentStepIndex_ < static_cast<int>(currentSteps_.size()) && !stepTransitioning_) {
			startStepTransition(currentStepIndex_ + 1);
			autoplayAccumulator_ -= kStepInterval;
		}
		if (currentStepIndex_ >= static_cast<int>(currentSteps_.size())) {
			autoplay_ = false;
			autoplayAccumulator_ = 0.0f;
		}
	}

	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

void ShortestPathUI::drawSfml(sf::RenderWindow& window) {
	const sf::Vector2u size = window.getSize();
	const sf::Font* font = getShortestPathFont();
	const float dt = ImGui::GetIO().DeltaTime;
	if (initializeAnimating_) {
		const float safeDuration = std::max(0.08f, initializeAnimationDuration_);
		initializeAnimationProgress_ = std::min(1.0f, initializeAnimationProgress_ + dt / safeDuration);
		if (initializeAnimationProgress_ >= 1.0f) {
			initializeAnimating_ = false;
		}
	}
	const float initializeBaseT = initializeAnimating_ ? easeInOut(initializeAnimationProgress_) : 1.0f;

	sf::RectangleShape background(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
	background.setFillColor(canvasBgColor_);
	window.draw(background);

	if (vertexCount_ <= 0) {
		return;
	}

	const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
	const bool mouseInsideCanvas =
		mousePos.x >= 0 && mousePos.y >= 0 &&
		mousePos.x < static_cast<int>(size.x) && mousePos.y < static_cast<int>(size.y);
	const bool canDragCanvas = mouseInsideCanvas && !ImGui::GetIO().WantCaptureMouse;

	if (canDragCanvas) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (std::abs(wheel) > 0.001f) {
			const float zoomStep = 1.0f + wheel * 0.12f;
			zoomScale_ = std::clamp(zoomScale_ * zoomStep, 0.55f, 2.2f);
		}
	}

	const bool isDragPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if (canDragCanvas && isDragPressed) {
		if (!isCanvasDragging_) {
			isCanvasDragging_ = true;
			lastDragMousePos_ = mousePos;
		}
		else {
			const int deltaX = mousePos.x - lastDragMousePos_.x;
			const int deltaY = mousePos.y - lastDragMousePos_.y;
			scrollOffsetX_ += static_cast<float>(deltaX);
			scrollOffsetY_ += static_cast<float>(deltaY);
			lastDragMousePos_ = mousePos;
		}
	}
	else {
		isCanvasDragging_ = false;
	}

	scrollOffsetX_ = std::clamp(scrollOffsetX_, -10000.0f, 10000.0f);
	scrollOffsetY_ = std::clamp(scrollOffsetY_, -10000.0f, 10000.0f);

	const int fromStep = stepTransitioning_ ? transitionFromStep_ : currentStepIndex_;
	const int toStep = stepTransitioning_ ? transitionToStep_ : currentStepIndex_;
	const float transitionT = stepTransitioning_ ? easeInOut(stepTransitionProgress_) : 1.0f;

	const SPVisualState fromState = buildVisualState(currentSteps_, fromStep, vertexCount_);
	const SPVisualState toState = buildVisualState(currentSteps_, toStep, vertexCount_);

	auto nodeColor = [&](const SPVisualState& state, int node) {
		if (node >= 0 && node < static_cast<int>(state.inPath.size()) && state.inPath[static_cast<std::size_t>(node)]) {
			return pathNodeColor_;
		}
		if (node >= 0 && node < static_cast<int>(state.settled.size()) && state.settled[static_cast<std::size_t>(node)]) {
			return settledNodeColor_;
		}
		if (state.activeNode == node) {
			return activeNodeColor_;
		}
		return nodeBaseColor_;
	};

	const float radius = std::clamp(nodeRadius_ * zoomScale_, 12.0f, 84.0f);
	const sf::Vector2f center(static_cast<float>(size.x) * 0.5f + scrollOffsetX_, static_cast<float>(size.y) * 0.44f + scrollOffsetY_);
	const float ringRadius = std::max(80.0f, std::min(size.x, size.y) * 0.28f * zoomScale_);

	std::vector<sf::Vector2f> nodePos(static_cast<std::size_t>(vertexCount_));
	if (vertexCount_ == 1) {
		nodePos[0] = center;
	}
	else {
		for (int i = 0; i < vertexCount_; ++i) {
			const float t = static_cast<float>(i) / static_cast<float>(vertexCount_);
			const float angle = t * 2.0f * 3.14159265f - 3.14159265f * 0.5f;
			nodePos[static_cast<std::size_t>(i)] = sf::Vector2f(center.x + std::cos(angle) * ringRadius, center.y + std::sin(angle) * ringRadius);
		}
	}

	const sf::Vector2f collapsedCenter(static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) * 0.44f);
	const float staggerSpan = (vertexCount_ <= 1) ? 0.0f : std::min(0.45f, 0.06f * static_cast<float>(vertexCount_ - 1));
	auto nodeRevealT = [&](int nodeIndex) {
		if (!initializeAnimating_ || vertexCount_ <= 1) {
			return initializeBaseT;
		}
		const float ratio = static_cast<float>(nodeIndex) / static_cast<float>(vertexCount_ - 1);
		const float delay = staggerSpan * ratio;
		const float local = (initializeAnimationProgress_ - delay) / std::max(0.001f, 1.0f - delay);
		return easeInOut(std::clamp(local, 0.0f, 1.0f));
	};

	auto animatedPoint = [&](const sf::Vector2f& point, float t) {
		return sf::Vector2f(
			lerp(collapsedCenter.x, point.x, t),
			lerp(collapsedCenter.y, point.y, t)
		);
	};

	std::unordered_set<long long> uniqueEdges;
	for (const auto& edge : graphEdges_) {
		int u = edge[0];
		int v = edge[1];
		if (u < 0 || v < 0 || u >= vertexCount_ || v >= vertexCount_) {
			continue;
		}

		const int a = std::min(u, v);
		const int b = std::max(u, v);
		const long long key = (static_cast<long long>(a) << 32) | static_cast<unsigned int>(b);
		if (uniqueEdges.find(key) != uniqueEdges.end()) {
			continue;
		}
		uniqueEdges.insert(key);

		sf::Color lineColor = edgeColor_;
		const bool inPath = (toState.parent[static_cast<std::size_t>(u)] == v) || (toState.parent[static_cast<std::size_t>(v)] == u);
		if (inPath && toState.inPath[static_cast<std::size_t>(u)] && toState.inPath[static_cast<std::size_t>(v)]) {
			lineColor = pathEdgeColor_;
		}
		if ((toState.relaxU == u && toState.relaxV == v) || (toState.relaxU == v && toState.relaxV == u)) {
			lineColor = relaxEdgeColor_;
		}

		const float edgeRevealT = std::min(nodeRevealT(u), nodeRevealT(v));
		const sf::Vector2f animatedFrom = animatedPoint(nodePos[static_cast<std::size_t>(u)], edgeRevealT);
		const sf::Vector2f animatedTo = animatedPoint(nodePos[static_cast<std::size_t>(v)], edgeRevealT);

		if (initializeAnimating_ && edgeRevealT > 0.01f) {
			const float glowPulse = std::sin(edgeRevealT * 3.14159265f);
			const float glowAlphaF = 190.0f * glowPulse * (1.0f - initializeAnimationProgress_ * 0.45f);
			const sf::Color glowColor(255, 215, 120, static_cast<std::uint8_t>(std::clamp(glowAlphaF, 0.0f, 220.0f)));
			const float glowThickness = edgeThickness_ * zoomScale_ * (2.6f - 1.4f * edgeRevealT);
			drawThickLine(window, animatedFrom, animatedTo, glowThickness, glowColor);
		}

		drawThickLine(window, animatedFrom, animatedTo, edgeThickness_ * zoomScale_, sf::Color(lineColor.r, lineColor.g, lineColor.b, static_cast<std::uint8_t>(255.0f * edgeRevealT)));

		if (font != nullptr) {
			const int w = lookupEdgeWeight(graphEdges_, u, v);
			sf::Text wText(*font, std::to_string(w), static_cast<unsigned int>(16.0f * fontScale_));
			wText.setFillColor(sf::Color(35, 35, 35, static_cast<std::uint8_t>(230.0f * edgeRevealT)));
			const sf::Vector2f from = animatedFrom;
			const sf::Vector2f to = animatedTo;
			const sf::Vector2f mid = (from + to) * 0.5f;
			const sf::Vector2f delta = to - from;
			const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
			const sf::Vector2f normal = (length > 0.001f)
				? sf::Vector2f(-delta.y / length, delta.x / length)
				: sf::Vector2f(0.0f, -1.0f);
			const sf::Vector2f labelPos = mid + normal * (24.0f * zoomScale_);
			const sf::FloatRect wb = wText.getLocalBounds();
			wText.setPosition(sf::Vector2f(labelPos.x - (wb.position.x + wb.size.x * 0.5f), labelPos.y - (wb.position.y + wb.size.y * 0.5f)));
			window.draw(wText);
		}
	}

	for (int i = 0; i < vertexCount_; ++i) {
		const sf::Color fromFill = nodeColor(fromState, i);
		const sf::Color toFill = nodeColor(toState, i);
		const sf::Color fill = blendColor(fromFill, toFill, transitionT);
		const float revealT = nodeRevealT(i);
		const sf::Vector2f animatedPos = animatedPoint(nodePos[static_cast<std::size_t>(i)], revealT);
		const float popPulse = std::sin(revealT * 3.14159265f);
		float nodeScale = 0.45f + 0.55f * revealT + 0.22f * popPulse * (1.0f - revealT * 0.35f);
		nodeScale = std::clamp(nodeScale, 0.35f, 1.24f);
		const float animatedRadius = radius * nodeScale;

		sf::CircleShape node(animatedRadius);
		node.setOrigin(sf::Vector2f(animatedRadius, animatedRadius));
		node.setPosition(animatedPos);
		node.setFillColor(sf::Color(fill.r, fill.g, fill.b, static_cast<std::uint8_t>(255.0f * revealT)));
		node.setOutlineThickness(edgeThickness_ * zoomScale_);
		node.setOutlineColor(sf::Color(edgeColor_.r, edgeColor_.g, edgeColor_.b, static_cast<std::uint8_t>(255.0f * revealT)));
		window.draw(node);

		if (i == startNode_ || i == endNode_) {
			sf::CircleShape ring(animatedRadius + 8.0f);
			ring.setOrigin(sf::Vector2f(animatedRadius + 8.0f, animatedRadius + 8.0f));
			ring.setPosition(animatedPos);
			ring.setFillColor(sf::Color::Transparent);
			ring.setOutlineThickness(3.0f);
			ring.setOutlineColor(i == startNode_ ? sf::Color(59, 130, 246, static_cast<std::uint8_t>(220.0f * revealT)) : sf::Color(234, 88, 12, static_cast<std::uint8_t>(220.0f * revealT)));
			window.draw(ring);
		}

		if (i == toState.activeNode) {
			sf::CircleShape activeRing(animatedRadius + 12.0f);
			activeRing.setOrigin(sf::Vector2f(animatedRadius + 12.0f, animatedRadius + 12.0f));
			activeRing.setPosition(animatedPos);
			activeRing.setFillColor(sf::Color::Transparent);
			activeRing.setOutlineThickness(2.0f);
			activeRing.setOutlineColor(sf::Color(highlightRingColor_.r, highlightRingColor_.g, highlightRingColor_.b, static_cast<std::uint8_t>(highlightRingColor_.a * revealT)));
			window.draw(activeRing);
		}

		if (font != nullptr) {
			sf::Text indexText(*font, std::to_string(i), static_cast<unsigned int>(20.0f * fontScale_));
			indexText.setFillColor(sf::Color(valueTextColor_.r, valueTextColor_.g, valueTextColor_.b, static_cast<std::uint8_t>(255.0f * revealT)));
			const sf::FloatRect ib = indexText.getLocalBounds();
			indexText.setPosition(sf::Vector2f(
				animatedPos.x - (ib.position.x + ib.size.x * 0.5f),
				animatedPos.y - (ib.position.y + ib.size.y * 0.5f)
			));
			window.draw(indexText);

			const int dist = toState.distances[static_cast<std::size_t>(i)];
			const std::string distLabel = dist == std::numeric_limits<int>::max() ? "INF" : std::to_string(dist);
			sf::Text distText(*font, distLabel, static_cast<unsigned int>(15.0f * fontScale_));
			distText.setFillColor(sf::Color(20, 20, 20, static_cast<std::uint8_t>(220.0f * revealT)));
			const sf::FloatRect db = distText.getLocalBounds();
			distText.setPosition(sf::Vector2f(
				animatedPos.x - (db.position.x + db.size.x * 0.5f),
				animatedPos.y + animatedRadius + 8.0f
			));
			window.draw(distText);
		}
	}
}

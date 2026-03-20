#include <ui/shortestpath.h>
#include <ui/common.h>

#include <imgui.h>

ShortestPathUI::ShortestPathUI() {
	// This is where you can initialize any resources or variables needed
}

void ShortestPathUI::draw() {
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(760.0f, 450.0f), ImGuiCond_Once);

	if (!ImGui::Begin("Shortest Path Visualizer")) {
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("Graph module foundation is ready.");
	ImGui::Separator();
	ImGui::TextUnformatted("Planned algorithms: BFS, Dijkstra, Bellman-Ford, Floyd-Warshall.");

	ImGui::Spacing();
	ImGui::TextWrapped("Next implementation step: add graph initialization and algorithm execution controls, then visualize explored edges and final path.");

	if (ImGui::Button("Back to menu")) {
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}
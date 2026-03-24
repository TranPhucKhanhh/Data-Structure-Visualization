#include <ui/shortestpath.h>
#include <ui/common.h>

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

	if (ImGui::Button("DATA STRUCTURES", ImVec2(170.0f, 0.0f))) {
		uiConfig.state = UIState::Menu;
	}
	ImGui::Separator();

	ImGui::TextUnformatted("Graph module foundation is ready.");
	ImGui::Separator();
	ImGui::TextUnformatted("Planned algorithms: BFS, Dijkstra, Bellman-Ford, Floyd-Warshall.");
	ImGui::SeparatorText("Style");
	ImGui::SliderFloat("Node radius##sp", &nodeRadius_, 12.0f, 60.0f, "%.1f");
	ImGui::SliderFloat("Edge thickness##sp", &edgeThickness_, 1.0f, 8.0f, "%.1f");
	ImGui::SliderFloat("Font scale##sp", &fontScale_, 0.75f, 2.0f, "%.2f");

	ImGui::Spacing();
	ImGui::TextWrapped("Next implementation step: add graph initialization and algorithm execution controls, then visualize explored edges and final path.");

	if (ImGui::Button("Back to menu")) {
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}
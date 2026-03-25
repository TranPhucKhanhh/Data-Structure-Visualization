#include <ui/heap.h>

HeapUI::HeapUI() {
	// This is where you can initialize any resources or variables needed
}

void HeapUI::draw() {
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(720.0f, 420.0f), ImGuiCond_Once);

	if (!ImGui::Begin("Heap Visualizer")) {
		ImGui::End();
		return;
	}

	if (ImGui::Button("DATA STRUCTURES", ImVec2(170.0f, 0.0f))) {
		uiConfig.state = UIState::Menu;
	}
	ImGui::Separator();

	ImGui::TextUnformatted("Heap module foundation is ready.");
	ImGui::Separator();
	ImGui::SeparatorText("Style");
	ImGui::SliderFloat("Node radius##heap", &nodeRadius_, 12.0f, 60.0f, "%.1f");
	ImGui::SliderFloat("Edge thickness##heap", &edgeThickness_, 1.0f, 8.0f, "%.1f");
	ImGui::SliderFloat("Font scale##heap", &fontScale_, 0.75f, 2.0f, "%.2f");

	ImGui::Spacing();
	ImGui::TextWrapped("Next implementation step: hook initialize/add/delete/update/search actions and render heap array/tree views.");

	if (ImGui::Button("Back to menu")) {
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}
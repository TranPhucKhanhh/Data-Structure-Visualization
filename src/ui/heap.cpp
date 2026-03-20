#include <ui/heap.h>
#include <ui/common.h>

#include <imgui.h>

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

	ImGui::TextUnformatted("Heap module foundation is ready.");
	ImGui::Separator();
	ImGui::Text("Node radius: %.1f", uiConfig.style.nodeRadius);
	ImGui::Text("Edge thickness: %.1f", uiConfig.style.edgeThickness);

	ImGui::Spacing();
	ImGui::TextWrapped("Next implementation step: hook initialize/add/delete/update/search actions and render heap array/tree views.");

	if (ImGui::Button("Back to menu")) {
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}
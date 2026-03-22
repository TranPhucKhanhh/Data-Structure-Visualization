#include <ui/menu.h>
#include <ui/common.h>

#include <imgui.h>

MenuUI::MenuUI() {
	// This is where you can initialize any resources or variables needed
}

void MenuUI::draw() {
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(560.0f, 560.0f), ImGuiCond_Once);

	if (!ImGui::Begin("Data Structure Visualizer - Menu")) {
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("Select a module");
	ImGui::Spacing();

	if (ImGui::Button("Singly Linked List", ImVec2(240.0f, 0.0f))) {
		uiConfig.state = UIState::SinglyLinkedList;
	}
	if (ImGui::Button("Trie", ImVec2(240.0f, 0.0f))) {
		uiConfig.state = UIState::Trie;
	}
	if (ImGui::Button("Heap", ImVec2(240.0f, 0.0f))) {
		uiConfig.state = UIState::Heap;
	}
	if (ImGui::Button("Shortest Path Graph", ImVec2(240.0f, 0.0f))) {
		uiConfig.state = UIState::ShortestPath;
	}

	ImGui::TextWrapped("Choose a module to begin.");
	ImGui::End();
}
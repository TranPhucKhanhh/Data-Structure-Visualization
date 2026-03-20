#include <ui/trie.h>
#include <ui/common.h>

#include <imgui.h>

TrieUI::TrieUI() {
	// This is where you can initialize any resources or variables needed
}

void TrieUI::draw() {
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(720.0f, 420.0f), ImGuiCond_Once);

	if (!ImGui::Begin("Trie Visualizer")) {
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("Trie module foundation is ready.");
	ImGui::Separator();
	ImGui::Text("Node radius: %.1f", uiConfig.style.nodeRadius);
	ImGui::Text("Edge thickness: %.1f", uiConfig.style.edgeThickness);

	ImGui::Spacing();
	ImGui::TextWrapped("Next implementation step: hook initialize/add/delete/update/search actions and render trie nodes on a canvas.");

	if (ImGui::Button("Back to menu")) {
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}
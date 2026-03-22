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
	ImGui::SeparatorText("Style");
	ImGui::SliderFloat("Node radius##trie", &nodeRadius_, 12.0f, 60.0f, "%.1f");
	ImGui::SliderFloat("Edge thickness##trie", &edgeThickness_, 1.0f, 8.0f, "%.1f");
	ImGui::SliderFloat("Font scale##trie", &fontScale_, 0.75f, 2.0f, "%.2f");

	ImGui::Spacing();
	ImGui::TextWrapped("Next implementation step: hook initialize/add/delete/update/search actions and render trie nodes on a canvas.");

	if (ImGui::Button("Back to menu")) {
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}
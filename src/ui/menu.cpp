#include "ui/menu.h"
#include "ui/common.h"

MenuUI::MenuUI() {
	// Khởi tạo các tài nguyên nếu cần thiết (hiện tại để trống)
}

void MenuUI::draw() {
	// Cài đặt vị trí và kích thước mặc định cho cửa sổ Menu
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(560.0f, 560.0f), ImGuiCond_Once);

	// Bắt đầu vẽ cửa sổ ImGui
	if (!ImGui::Begin("Data Structure Visualizer - Menu")) {
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("Select a module");
	ImGui::Spacing();

	// Vẽ các nút bấm và chuyển đổi trạng thái (uiConfig.state) tương ứng
	if (ImGui::Button("Singly Linked List", ImVec2(240.0f, 0.0f))) {
		uiConfig.state = UIState::SinglyLinkedList;
	}
	if (ImGui::Button("Trie", ImVec2(240.0f, 0.0f))) {
		uiConfig.state = UIState::Trie; // Chuyển sang màn hình Trie
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
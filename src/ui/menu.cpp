#include "ui/menu.h"
#include "ui/common.h"

#include <algorithm>

MenuUI::MenuUI() {
	// Khởi tạo các tài nguyên nếu cần thiết (hiện tại để trống)
}

void MenuUI::draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vpPos = viewport->Pos;
	const ImVec2 vpSize = viewport->Size;

	ImGui::SetNextWindowPos(vpPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(vpSize, ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	if (!ImGui::Begin("Main Menu Canvas##DSMenu", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse)) {
		ImGui::End();
		ImGui::PopStyleColor();
		return;
	}

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetWindowPos();
	const float titleY = origin.y + 26.0f;
	const char* title = "Data Structure Visualization";
	const ImVec2 titleSize = ImGui::CalcTextSize(title);
	drawList->AddText(
		ImVec2(origin.x + (vpSize.x - titleSize.x) * 0.5f, titleY),
		IM_COL32(20, 26, 33, 255),
		title
	);

	const char* subtitle = "Select a visualizer module";
	const ImVec2 subtitleSize = ImGui::CalcTextSize(subtitle);
	drawList->AddText(
		ImVec2(origin.x + (vpSize.x - subtitleSize.x) * 0.5f, titleY + 32.0f),
		IM_COL32(107, 114, 128, 255),
		subtitle
	);

	const float topPadding = 92.0f;
	const float gap = 24.0f;
	const float areaWidth = std::min(vpSize.x - 80.0f, 1220.0f);
	const float cardWidth = (areaWidth - gap) * 0.5f;
	const float cardHeight = std::min((vpSize.y - topPadding - 64.0f - gap) * 0.5f, 330.0f);
	const float startX = origin.x + (vpSize.x - areaWidth) * 0.5f;
	const float startY = origin.y + topPadding;

	auto drawPreview = [&](int type, const ImVec2& min, const ImVec2& max, ImU32 fg) {
		const float w = max.x - min.x;
		const float h = max.y - min.y;
		if (type == 0) {
			const float y = min.y + h * 0.58f;
			const float step = w * 0.2f;
			for (int i = 0; i < 4; ++i) {
				const float x = min.x + w * 0.18f + step * static_cast<float>(i);
				drawList->AddRect(ImVec2(x - 30.0f, y - 20.0f), ImVec2(x + 30.0f, y + 20.0f), fg, 0.0f, 0, 2.0f);
				if (i < 3) {
					drawList->AddLine(ImVec2(x + 30.0f, y), ImVec2(x + step - 30.0f, y), fg, 2.0f);
					drawList->AddTriangleFilled(
						ImVec2(x + step - 34.0f, y - 5.0f),
						ImVec2(x + step - 34.0f, y + 5.0f),
						ImVec2(x + step - 26.0f, y),
						fg
					);
				}
			}
		}
		else if (type == 1) {
			const ImVec2 root(min.x + w * 0.50f, min.y + h * 0.24f);
			const ImVec2 l1(min.x + w * 0.34f, min.y + h * 0.48f);
			const ImVec2 r1(min.x + w * 0.66f, min.y + h * 0.48f);
			const ImVec2 l2a(min.x + w * 0.24f, min.y + h * 0.74f);
			const ImVec2 l2b(min.x + w * 0.44f, min.y + h * 0.74f);
			const ImVec2 r2a(min.x + w * 0.56f, min.y + h * 0.74f);
			const ImVec2 r2b(min.x + w * 0.76f, min.y + h * 0.74f);
			drawList->AddLine(root, l1, fg, 2.0f);
			drawList->AddLine(root, r1, fg, 2.0f);
			drawList->AddLine(l1, l2a, fg, 2.0f);
			drawList->AddLine(l1, l2b, fg, 2.0f);
			drawList->AddLine(r1, r2a, fg, 2.0f);
			drawList->AddLine(r1, r2b, fg, 2.0f);
			for (const ImVec2 p : { root, l1, r1, l2a, l2b, r2a, r2b }) {
				drawList->AddCircleFilled(p, 10.0f, fg);
			}
		}
		else if (type == 2) {
			const ImVec2 root(min.x + w * 0.50f, min.y + h * 0.22f);
			const ImVec2 l1(min.x + w * 0.35f, min.y + h * 0.47f);
			const ImVec2 r1(min.x + w * 0.65f, min.y + h * 0.47f);
			const ImVec2 l2a(min.x + w * 0.26f, min.y + h * 0.76f);
			const ImVec2 l2b(min.x + w * 0.44f, min.y + h * 0.76f);
			const ImVec2 r2a(min.x + w * 0.56f, min.y + h * 0.76f);
			const ImVec2 r2b(min.x + w * 0.74f, min.y + h * 0.76f);
			drawList->AddLine(root, l1, fg, 2.0f);
			drawList->AddLine(root, r1, fg, 2.0f);
			drawList->AddLine(l1, l2a, fg, 2.0f);
			drawList->AddLine(l1, l2b, fg, 2.0f);
			drawList->AddLine(r1, r2a, fg, 2.0f);
			drawList->AddLine(r1, r2b, fg, 2.0f);
			for (const ImVec2 p : { root, l1, r1, l2a, l2b, r2a, r2b }) {
				drawList->AddCircleFilled(p, 12.0f, fg);
			}
		}
		else {
			const ImVec2 a(min.x + w * 0.18f, min.y + h * 0.68f);
			const ImVec2 b(min.x + w * 0.36f, min.y + h * 0.30f);
			const ImVec2 c(min.x + w * 0.58f, min.y + h * 0.56f);
			const ImVec2 d(min.x + w * 0.78f, min.y + h * 0.28f);
			const ImVec2 e(min.x + w * 0.72f, min.y + h * 0.78f);
			drawList->AddLine(a, b, fg, 2.0f);
			drawList->AddLine(a, c, fg, 2.0f);
			drawList->AddLine(b, c, fg, 2.0f);
			drawList->AddLine(b, d, fg, 2.0f);
			drawList->AddLine(c, d, fg, 2.0f);
			drawList->AddLine(c, e, fg, 2.0f);
			drawList->AddLine(d, e, fg, 2.0f);
			drawList->AddLine(a, b, IM_COL32(255, 210, 90, 255), 3.5f);
			drawList->AddLine(b, d, IM_COL32(255, 210, 90, 255), 3.5f);
			drawList->AddLine(d, e, IM_COL32(255, 210, 90, 255), 3.5f);
			for (const ImVec2 p : { a, b, c, d, e }) {
				drawList->AddCircleFilled(p, 10.0f, fg);
			}
		}
	};

	auto drawCard = [&](const char* id, const char* label, const char* desc, UIState state, ImU32 previewBg, int iconType, const ImVec2& pos) {
		ImGui::SetCursorScreenPos(pos);
		ImGui::InvisibleButton(id, ImVec2(cardWidth, cardHeight));
		const bool hovered = ImGui::IsItemHovered();
		if (ImGui::IsItemClicked()) {
			uiConfig.state = state;
		}

		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		const ImU32 cardBg = hovered ? IM_COL32(244, 249, 255, 255) : IM_COL32(239, 247, 252, 255);
		const ImU32 border = hovered ? IM_COL32(52, 120, 246, 255) : IM_COL32(172, 206, 223, 255);

		drawList->AddRectFilled(min, max, cardBg, 8.0f);
		drawList->AddRect(min, max, border, 8.0f, 0, hovered ? 3.0f : 2.0f);

		const ImVec2 previewMin(min.x + 14.0f, min.y + 14.0f);
		const ImVec2 previewMax(max.x - 14.0f, min.y + cardHeight * 0.68f);
		drawList->AddRectFilled(previewMin, previewMax, previewBg, 4.0f);
		drawPreview(iconType, previewMin, previewMax, IM_COL32(244, 248, 255, 245));

		drawList->AddText(ImVec2(min.x + 18.0f, previewMax.y + 14.0f), IM_COL32(19, 32, 43, 255), label);
		drawList->AddText(ImVec2(min.x + 18.0f, previewMax.y + 36.0f), IM_COL32(84, 97, 110, 255), desc);
	};

	drawCard("card_ll", "Linked List", "Node and pointer operations", UIState::SinglyLinkedList, IM_COL32(76, 97, 185, 255), 0, ImVec2(startX, startY));
	drawCard("card_trie", "Trie", "Prefix tree visualization", UIState::Trie, IM_COL32(61, 176, 199, 255), 1, ImVec2(startX + cardWidth + gap, startY));
	drawCard("card_heap", "Heap", "Binary heap relationships", UIState::Heap, IM_COL32(245, 193, 36, 255), 2, ImVec2(startX, startY + cardHeight + gap));
	drawCard("card_sp", "Shortest Path on Graph", "Weighted graph path exploration", UIState::ShortestPath, IM_COL32(220, 83, 68, 255), 3, ImVec2(startX + cardWidth + gap, startY + cardHeight + gap));

	ImGui::End();
	ImGui::PopStyleColor();
}
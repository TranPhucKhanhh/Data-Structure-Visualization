#include "ui/menu.h"
#include "ui/common.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace {
	ImFont* getFallbackFont() {
		ImGuiIO& io = ImGui::GetIO();
		if (io.FontDefault != nullptr) {
			return io.FontDefault;
		}
		if (!io.Fonts->Fonts.empty()) {
			return io.Fonts->Fonts[0];
		}
		return nullptr;
	}
}

MenuUI::MenuUI() {
	// Khởi tạo các tài nguyên nếu cần thiết (hiện tại để trống)
}

void MenuUI::draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vpPos = viewport->Pos;
	const ImVec2 vpSize = viewport->Size;
	const float uiScale = std::max(0.70f, std::min(vpSize.x / 1920.0f, vpSize.y / 1080.0f));

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

	ImGui::SetWindowFontScale(1.0f);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImFont* fallbackFont = getFallbackFont();
	ImFont* titleFont = menuTitleFont != nullptr ? menuTitleFont : fallbackFont;
	ImFont* subtitleFont = menuSubtitleFont != nullptr ? menuSubtitleFont : fallbackFont;
	ImFont* cardTitleFont = menuCardTitleFont != nullptr ? menuCardTitleFont : fallbackFont;
	ImFont* cardDescFont = menuCardDescFont != nullptr ? menuCardDescFont : fallbackFont;
	const float titleFontSize = (titleFont != nullptr ? titleFont->FontSize : 56.0f) * uiScale;
	const float subtitleFontSize = (subtitleFont != nullptr ? subtitleFont->FontSize : 28.0f) * uiScale;
	const float cardTitleFontSize = (cardTitleFont != nullptr ? cardTitleFont->FontSize : 24.0f) * uiScale;
	const float cardDescFontSize = (cardDescFont != nullptr ? cardDescFont->FontSize : 20.0f) * uiScale;
	const ImVec2 origin = ImGui::GetWindowPos();
	const float titleY = origin.y + 24.0f * uiScale;
	const char* title = "Data Structure Visualization";
	const ImVec2 titleSize = titleFont
		? titleFont->CalcTextSizeA(titleFontSize, FLT_MAX, 0.0f, title)
		: ImGui::CalcTextSize(title);
	if (titleFont) {
		drawList->AddText(titleFont, titleFontSize,
			ImVec2(origin.x + (vpSize.x - titleSize.x) * 0.5f, titleY),
			IM_COL32(20, 26, 33, 255),
			title
		);
	}
	else {
		drawList->AddText(
			ImVec2(origin.x + (vpSize.x - titleSize.x) * 0.5f, titleY),
			IM_COL32(20, 26, 33, 255),
			title
		);
	}

	const char* subtitle = "Select a visualizer module";
	const ImVec2 subtitleSize = subtitleFont
		? subtitleFont->CalcTextSizeA(subtitleFontSize, FLT_MAX, 0.0f, subtitle)
		: ImGui::CalcTextSize(subtitle);
	const float subtitleY = titleY + titleSize.y + 8.0f * uiScale;
	if (subtitleFont) {
		drawList->AddText(subtitleFont, subtitleFontSize,
			ImVec2(origin.x + (vpSize.x - subtitleSize.x) * 0.5f, subtitleY),
			IM_COL32(107, 114, 128, 255),
			subtitle
		);
	}
	else {
		drawList->AddText(
			ImVec2(origin.x + (vpSize.x - subtitleSize.x) * 0.5f, subtitleY),
			IM_COL32(107, 114, 128, 255),
			subtitle
		);
	}

	const float topPadding = subtitleY + subtitleSize.y + 22.0f * uiScale - origin.y;
	const float gap = 24.0f * uiScale;
	const float areaWidth = std::min(vpSize.x - 80.0f * uiScale, 1220.0f * uiScale);
	const float cardWidth = (areaWidth - gap) * 0.5f;
	const float bottomReserved = 112.0f * uiScale;
	const float cardHeight = std::min((vpSize.y - topPadding - bottomReserved - gap) * 0.5f, 330.0f * uiScale);
	const float startX = origin.x + (vpSize.x - areaWidth) * 0.5f;
	const float startY = origin.y + topPadding;

	auto drawPreview = [&](int type, const ImVec2& min, const ImVec2& max, ImU32 fg, bool hovered, float previewTime) {
		const float w = max.x - min.x;
		const float h = max.y - min.y;
		if (type == 0) {
			const float y = min.y + h * 0.58f;
			const float step = w * 0.2f;
			const float phase = std::fmod(previewTime, 1.8f) / 1.8f;
			const int active = hovered ? static_cast<int>(phase * 4.0f) % 4 : -1;
			for (int i = 0; i < 4; ++i) {
				const float x = min.x + w * 0.18f + step * static_cast<float>(i);
				if (i == active) {
					drawList->AddRectFilled(ImVec2(x - 30.0f, y - 20.0f), ImVec2(x + 30.0f, y + 20.0f), IM_COL32(255, 255, 255, 65), 0.0f);
				}
				drawList->AddRect(ImVec2(x - 30.0f, y - 20.0f), ImVec2(x + 30.0f, y + 20.0f), fg, 0.0f, 0, 2.0f);
				if (i < 3) {
					const bool activeEdge = hovered && i == std::max(0, active - 1);
					drawList->AddLine(ImVec2(x + 30.0f, y), ImVec2(x + step - 30.0f, y), fg, activeEdge ? 3.5f : 2.0f);
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
			const ImVec2 root(min.x + w * 0.50f, min.y + h * 0.20f);
			const ImVec2 n1(min.x + w * 0.34f, min.y + h * 0.38f);
			const ImVec2 n2(min.x + w * 0.50f, min.y + h * 0.38f);
			const ImVec2 n3(min.x + w * 0.66f, min.y + h * 0.38f);
			const ImVec2 n4(min.x + w * 0.28f, min.y + h * 0.58f);
			const ImVec2 n5(min.x + w * 0.40f, min.y + h * 0.58f);
			const ImVec2 n6(min.x + w * 0.56f, min.y + h * 0.58f);
			const ImVec2 n7(min.x + w * 0.30f, min.y + h * 0.78f);

			drawList->AddLine(root, n1, fg, 2.0f);
			drawList->AddLine(root, n2, fg, 2.0f);
			drawList->AddLine(root, n3, fg, 2.0f);
			drawList->AddLine(n1, n4, fg, 2.0f);
			drawList->AddLine(n1, n5, fg, 2.0f);
			drawList->AddLine(n2, n6, fg, 2.0f);
			drawList->AddLine(n4, n7, fg, 2.0f);

			for (const ImVec2 p : { root, n1, n2, n3, n4, n5, n6, n7 }) {
				drawList->AddCircleFilled(p, 8.0f, fg);
			}

			if (hovered) {
				const float phase = std::fmod(previewTime, 1.8f) / 1.8f;
				const int step = 1 + static_cast<int>(phase * 3.0f);
				drawList->AddLine(root, n1, IM_COL32(255, 230, 140, 255), 3.2f);
				if (step >= 2) drawList->AddLine(n1, n4, IM_COL32(255, 230, 140, 255), 3.2f);
				if (step >= 3) drawList->AddLine(n4, n7, IM_COL32(255, 230, 140, 255), 3.2f);

				const ImVec2 pathNodes[] = { root, n1, n4, n7 };
				const int dotIdx = static_cast<int>(phase * 4.0f) % 4;
				drawList->AddCircleFilled(pathNodes[dotIdx], 9.0f, IM_COL32(255, 245, 180, 255));
			}
		}
		else if (type == 2) {
			const ImVec2 root(min.x + w * 0.50f, min.y + h * 0.24f);
			const ImVec2 l1(min.x + w * 0.35f, min.y + h * 0.47f);
			const ImVec2 r1(min.x + w * 0.65f, min.y + h * 0.47f);
			const ImVec2 l2a(min.x + w * 0.26f, min.y + h * 0.76f);
			const ImVec2 l2b(min.x + w * 0.44f, min.y + h * 0.76f);
			const ImVec2 r2a(min.x + w * 0.56f, min.y + h * 0.76f);
			const ImVec2 r2b(min.x + w * 0.74f, min.y + h * 0.76f);
			const float phase = std::fmod(previewTime, 1.6f) / 1.6f;
			const int active = hovered ? static_cast<int>(phase * 4.0f) % 4 : -1;
			drawList->AddLine(root, l1, fg, 2.0f);
			drawList->AddLine(root, r1, fg, 2.0f);
			drawList->AddLine(l1, l2a, fg, 2.0f);
			drawList->AddLine(l1, l2b, fg, 2.0f);
			drawList->AddLine(r1, r2a, fg, 2.0f);
			drawList->AddLine(r1, r2b, fg, 2.0f);
			const ImVec2 activePath[] = { root, l1, l2b, root };
			for (const ImVec2 p : { root, l1, r1, l2a, l2b, r2a, r2b }) {
				drawList->AddCircleFilled(p, 11.0f, fg);
			}
			if (hovered) {
				drawList->AddCircleFilled(activePath[active], 12.0f, IM_COL32(255, 245, 180, 255));
			}
		}
		else {
			const ImVec2 a(min.x + w * 0.18f, min.y + h * 0.68f);
			const ImVec2 b(min.x + w * 0.36f, min.y + h * 0.30f);
			const ImVec2 c(min.x + w * 0.58f, min.y + h * 0.56f);
			const ImVec2 d(min.x + w * 0.78f, min.y + h * 0.28f);
			const ImVec2 e(min.x + w * 0.72f, min.y + h * 0.78f);
			const float phase = std::fmod(previewTime, 1.8f) / 1.8f;
			const int reveal = hovered ? (1 + static_cast<int>(phase * 3.0f)) : 3;
			drawList->AddLine(a, b, fg, 2.0f);
			drawList->AddLine(a, c, fg, 2.0f);
			drawList->AddLine(b, c, fg, 2.0f);
			drawList->AddLine(b, d, fg, 2.0f);
			drawList->AddLine(c, d, fg, 2.0f);
			drawList->AddLine(c, e, fg, 2.0f);
			drawList->AddLine(d, e, fg, 2.0f);
			if (reveal >= 1) drawList->AddLine(a, b, IM_COL32(255, 210, 90, 255), 3.5f);
			if (reveal >= 2) drawList->AddLine(b, d, IM_COL32(255, 210, 90, 255), 3.5f);
			if (reveal >= 3) drawList->AddLine(d, e, IM_COL32(255, 210, 90, 255), 3.5f);
			if (hovered) {
				const ImVec2 path[] = { a, b, d, e };
				const int dotIdx = static_cast<int>(phase * 4.0f) % 4;
				drawList->AddCircleFilled(path[dotIdx], 6.0f, IM_COL32(255, 245, 180, 255));
			}
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

		drawList->AddRectFilled(min, max, cardBg, 8.0f * uiScale);
		drawList->AddRect(min, max, border, 8.0f * uiScale, 0, hovered ? 3.0f * uiScale : 2.0f * uiScale);

		const ImVec2 previewMin(min.x + 14.0f * uiScale, min.y + 14.0f * uiScale);
		const ImVec2 previewMax(max.x - 14.0f, min.y + cardHeight * 0.68f);
		drawList->AddRectFilled(previewMin, previewMax, previewBg, 4.0f * uiScale);
		drawPreview(iconType, previewMin, previewMax, IM_COL32(244, 248, 255, 245), hovered, static_cast<float>(ImGui::GetTime()));

		if (cardTitleFont != nullptr && cardDescFont != nullptr) {
			drawList->AddText(cardTitleFont, cardTitleFontSize, ImVec2(min.x + 18.0f * uiScale, previewMax.y + 14.0f * uiScale), IM_COL32(19, 32, 43, 255), label);
			drawList->AddText(cardDescFont, cardDescFontSize, ImVec2(min.x + 18.0f * uiScale, previewMax.y + 42.0f * uiScale), IM_COL32(84, 97, 110, 255), desc);
		}
		else {
			drawList->AddText(ImVec2(min.x + 18.0f * uiScale, previewMax.y + 14.0f * uiScale), IM_COL32(19, 32, 43, 255), label);
			drawList->AddText(ImVec2(min.x + 18.0f * uiScale, previewMax.y + 42.0f * uiScale), IM_COL32(84, 97, 110, 255), desc);
		}
	};

	drawCard("card_ll", "Linked List", "Node and pointer operations", UIState::SinglyLinkedList, IM_COL32(76, 97, 185, 255), 0, ImVec2(startX, startY));
	drawCard("card_trie", "Trie", "Prefix tree visualization", UIState::Trie, IM_COL32(61, 176, 199, 255), 1, ImVec2(startX + cardWidth + gap, startY));
	drawCard("card_heap", "Heap", "Binary heap relationships", UIState::Heap, IM_COL32(245, 193, 36, 255), 2, ImVec2(startX, startY + cardHeight + gap));
	drawCard("card_sp", "Shortest Path on Graph", "Weighted graph path exploration", UIState::ShortestPath, IM_COL32(220, 83, 68, 255), 3, ImVec2(startX + cardWidth + gap, startY + cardHeight + gap));

	const ImVec2 quitSize(180.0f * uiScale, 44.0f * uiScale);
	ImGui::SetCursorScreenPos(ImVec2(
		origin.x + (vpSize.x - quitSize.x) * 0.5f,
		origin.y + vpSize.y - quitSize.y - 26.0f * uiScale
	));
	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(209, 67, 49, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(221, 86, 67, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(180, 54, 40, 255));
	if (ImGui::Button("QUIT", quitSize)) {
		uiConfig.requestAppQuit = true;
	}
	ImGui::PopStyleColor(3);

	ImGui::End();
	ImGui::PopStyleColor();
}
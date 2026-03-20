#include <ui/singlylinkedlist.h>
#include <ui/common.h>

#include <imgui.h>

#include <algorithm>
#include <string>

namespace {
	float lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	const char* kAddCode[] = {
		"1  if (index > size) return false;",
		"2  for (i = 0; i < index; ++i)",
		"3  newNode = Node(value);",
		"4  prev->next = newNode (or head = newNode);",
		"5  newNode->next = next;",
		"6  return true;"
	};

	const char* kDeleteCode[] = {
		"1  if (list.empty()) return false;",
		"2  for (i = 0; i <= index; ++i)",
		"3  target = node at index;",
		"4  remove target node;",
		"5  prev->next = afterTarget (or head = head->next);",
		"6  return true;"
	};

	const char* kUpdateCode[] = {
		"1  if (index >= size) return false;",
		"2  for (i = 0; i <= index; ++i)",
		"3      values[i] = newVal; done;",
		"4  return true;"
	};

	const char* kSearchCode[] = {
		"1  for (i = 0; i < size; ++i)",
		"2      if (values[i] == target)",
		"3          return found_at(i);",
		"4  return not_found;"
	};

	void pickCodeBlock(SLLOperationType opType, const char**& codeArray, int& lineCount)
	{
		switch (opType) {
		case SLLOperationType::Add:
			codeArray = kAddCode;
			lineCount = 6;
			break;
		case SLLOperationType::Delete:
			codeArray = kDeleteCode;
			lineCount = 6;
			break;
		case SLLOperationType::Update:
			codeArray = kUpdateCode;
			lineCount = 4;
			break;
		case SLLOperationType::Search:
			codeArray = kSearchCode;
			lineCount = 4;
			break;
		default:
			codeArray = nullptr;
			lineCount = 0;
			break;
		}
	}

	void drawCodePanel(int activeLine, SLLOperationType opType)
	{
		ImGui::BeginChild("SLLCode", ImVec2(0.0f, 100.0f), true);
		ImGui::TextUnformatted("Source Code Highlight");
		ImGui::Separator();

		const char** codeArray = nullptr;
		int lineCount = 0;
		pickCodeBlock(opType, codeArray, lineCount);
		switch (opType) {
		case SLLOperationType::Add:
			ImGui::TextUnformatted("Operation: Add");
			break;
		case SLLOperationType::Delete:
			ImGui::TextUnformatted("Operation: Delete");
			break;
		case SLLOperationType::Update:
			ImGui::TextUnformatted("Operation: Update");
			break;
		case SLLOperationType::Search:
			ImGui::TextUnformatted("Operation: Search");
			break;
		default:
			ImGui::TextUnformatted("Operation: Initialize");
			ImGui::EndChild();
			return;
		}

		ImGui::Separator();
		for (int i = 0; i < lineCount; ++i) {
			if ((i + 1) == activeLine) {
				ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.15f, 1.0f), "> %s", codeArray[i]);
			}
			else {
				ImGui::TextUnformatted(codeArray[i]);
			}
		}
		ImGui::EndChild();
	}

	void drawListCanvas(const SLLFrame& frame, float& scrollOffset, const SLLInterpolationState& interpolation, float& maxScrollOut)
	{
		ImGui::BeginChild("SLLCanvas", ImVec2(0.0f, 240.0f), true);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 origin = ImGui::GetCursorScreenPos();
		const ImVec2 available = ImGui::GetContentRegionAvail();

		drawList->AddRectFilled(origin,
			ImVec2(origin.x + available.x, origin.y + available.y),
			IM_COL32(18, 21, 30, 255), 8.0f);

		if (frame.values.empty()) {
			drawList->AddText(ImVec2(origin.x + 16.0f, origin.y + 16.0f), IM_COL32(220, 220, 220, 255), "Empty list");
			maxScrollOut = 0.0f;
			ImGui::EndChild();
			return;
		}

		const float userRadius = uiConfig.style.nodeRadius;
		const float radius = std::clamp(userRadius, 18.0f, 42.0f);
		const float diameter = radius * 2.0f;
		const float spacing = diameter + 50.0f;
		const float totalWidth = static_cast<float>(frame.values.size()) * spacing + 60.0f;

		float mouseWheel = ImGui::GetIO().MouseWheel;
		if (mouseWheel != 0.0f) {
			scrollOffset += mouseWheel * 40.0f;
		}

		float minScroll = 0.0f;
		float maxScroll = std::max(0.0f, totalWidth - available.x);
		maxScrollOut = maxScroll;

		int activeIdx = frame.activeIndex;
		if (interpolation.isTransitioning && interpolation.previousFrame.activeIndex >= 0) {
			float prev_x = 30.0f + static_cast<float>(interpolation.previousFrame.activeIndex) * spacing;
			float curr_x = 30.0f + static_cast<float>(activeIdx) * spacing;
			float smooth_x = prev_x + (curr_x - prev_x) * interpolation.transitionProgress;
			float target_scroll = std::clamp(smooth_x - available.x * 0.5f, minScroll, maxScroll);
			scrollOffset += (target_scroll - scrollOffset) * 0.15f;
		} else if (activeIdx >= 0) {
			float node_x = 30.0f + static_cast<float>(activeIdx) * spacing;
			float target_scroll = std::clamp(node_x - available.x * 0.5f, minScroll, maxScroll);
			scrollOffset += (target_scroll - scrollOffset) * 0.15f;
		}

		scrollOffset = std::clamp(scrollOffset, minScroll, maxScroll);

		const float startX = origin.x + 30.0f - scrollOffset;
		const float centerY = origin.y + available.y * 0.5f;

		for (std::size_t i = 0; i < frame.values.size(); ++i) {
			const float cx = startX + static_cast<float>(i) * spacing;

			if (cx + radius < origin.x || cx - radius > origin.x + available.x) {
				continue;
			}

			const ImVec2 center(cx, centerY);

			ImU32 nodeColor = IM_COL32(72, 149, 239, 255);
			if (static_cast<int>(i) == frame.secondaryIndex) {
				nodeColor = IM_COL32(156, 163, 175, 255);
			}

			if (static_cast<int>(i) == frame.activeIndex) {
				nodeColor = IM_COL32(245, 158, 11, 255);
			}

			drawList->AddCircleFilled(center, radius, nodeColor, 32);
			drawList->AddCircle(center, radius, IM_COL32(15, 23, 42, 255), 32, uiConfig.style.edgeThickness);

			const std::string valueText = std::to_string(frame.values[i]);
			const ImVec2 valueSize = ImGui::CalcTextSize(valueText.c_str());
			drawList->AddText(ImVec2(center.x - valueSize.x * 0.5f, center.y - valueSize.y * 0.5f), IM_COL32(255, 255, 255, 255), valueText.c_str());

			const std::string indexText = std::to_string(i);
			const ImVec2 indexSize = ImGui::CalcTextSize(indexText.c_str());
			drawList->AddText(ImVec2(center.x - indexSize.x * 0.5f, center.y - radius - 28.0f), IM_COL32(144, 202, 249, 255), indexText.c_str());

			if (i + 1 < frame.values.size()) {
				const ImVec2 arrowStart(center.x + radius, center.y);
				const ImVec2 arrowEnd(center.x + spacing - radius, center.y);
				drawList->AddLine(arrowStart, arrowEnd, IM_COL32(148, 163, 184, 255), uiConfig.style.edgeThickness);
				drawList->AddTriangleFilled(
					ImVec2(arrowEnd.x + 8.0f, arrowEnd.y),
					ImVec2(arrowEnd.x - 4.0f, arrowEnd.y - 6.0f),
					ImVec2(arrowEnd.x - 4.0f, arrowEnd.y + 6.0f),
					IM_COL32(148, 163, 184, 255));
			}
		}

		if (frame.activeIndex >= 0) {
			float fromX = startX + static_cast<float>(frame.activeIndex) * spacing;
			if (interpolation.isTransitioning && interpolation.previousFrame.activeIndex >= 0) {
				fromX = startX + static_cast<float>(interpolation.previousFrame.activeIndex) * spacing;
			}
			float toX = startX + static_cast<float>(frame.activeIndex) * spacing;
			float markerX = interpolation.isTransitioning ? lerp(fromX, toX, interpolation.transitionProgress) : toX;

			if (markerX + radius >= origin.x && markerX - radius <= origin.x + available.x) {
				drawList->AddCircle(ImVec2(markerX, centerY), radius + 7.0f, IM_COL32(255, 214, 102, 235), 36, 3.2f);
			}
		}

		ImGui::EndChild();
	}
}

SinglyLinkedListUI::SinglyLinkedListUI() {
	// Initialize playback mode and speed locally
	playbackMode_ = PlaybackMode::StepByStep;
	playbackSpeed_ = 1.0f;
	scrollOffset_ = 0.0f;
}

void SinglyLinkedListUI::draw() {
	ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(1280.0f, 820.0f), ImGuiCond_Once);

	if (!ImGui::Begin("Singly Linked List Visualizer")) {
		ImGui::End();
		return;
	}

	singlyLinkedList.updateInterpolation(ImGui::GetIO().DeltaTime);
	singlyLinkedList.updateAutoplay(ImGui::GetIO().DeltaTime, playbackSpeed_, autoplay_ && playbackMode_ == PlaybackMode::RunAtOnce);

	const SLLFrame& displayFrame = singlyLinkedList.getInterpolatedFrame();

	ImGui::TextUnformatted("Initialize");
	if (ImGui::Button("Create empty list")) {
		singlyLinkedList.initializeEmpty();
	}
	ImGui::SameLine();
	ImGui::PushItemWidth(70.0f);
	ImGui::InputInt("Random count", &randomCount_);
	ImGui::SameLine();
	ImGui::InputInt("Min##range", &randomMin_);
	ImGui::SameLine();
	ImGui::InputInt("Max##range", &randomMax_);
	ImGui::PopItemWidth();
	ImGui::SameLine();
	if (ImGui::Button("Randomize")) {
		singlyLinkedList.initializeRandom(randomCount_, randomMin_, randomMax_);
	}

	ImGui::InputText("Text file path##sll", txtPath_.data(), txtPath_.size());
	ImGui::SameLine();
	if (ImGui::Button("Load txt")) {
		singlyLinkedList.initializeFromTextFile(txtPath_.data());
	}
	ImGui::InputText("JSON file path##sll", jsonPath_.data(), jsonPath_.size());
	ImGui::SameLine();
	if (ImGui::Button("Load json")) {
		singlyLinkedList.initializeFromJsonFile(jsonPath_.data());
	}

	ImGui::SeparatorText("Operations");
	ImGui::PushItemWidth(90.0f);
	ImGui::InputInt("Add index##add", &addIndex_);
	ImGui::SameLine();
	ImGui::InputInt("Add value##add", &addValue_);
	ImGui::SameLine();
	if (ImGui::Button("Add")) {
		singlyLinkedList.addAt(static_cast<std::size_t>(std::max(0, addIndex_)), addValue_);
	}

	ImGui::InputInt("Delete index##del", &deleteIndex_);
	ImGui::SameLine();
	if (ImGui::Button("Delete")) {
		singlyLinkedList.deleteAt(static_cast<std::size_t>(std::max(0, deleteIndex_)));
	}

	ImGui::InputInt("Update index##upd", &updateIndex_);
	ImGui::SameLine();
	ImGui::InputInt("Update value##upd", &updateValue_);
	ImGui::SameLine();
	if (ImGui::Button("Update")) {
		singlyLinkedList.updateAt(static_cast<std::size_t>(std::max(0, updateIndex_)), updateValue_);
	}

	ImGui::InputInt("Search value##search", &searchValue_);
	ImGui::SameLine();
	if (ImGui::Button("Search")) {
		singlyLinkedList.searchValue(searchValue_);
	}
	ImGui::PopItemWidth();

	ImGui::SeparatorText("Playback Controls");
	int mode = playbackMode_ == PlaybackMode::StepByStep ? 0 : 1;
	ImGui::RadioButton("Step by step##sll", &mode, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Run at once##sll", &mode, 1);
	playbackMode_ = (mode == 0) ? PlaybackMode::StepByStep : PlaybackMode::RunAtOnce;

	if (ImGui::Button("Prev")) {
		autoplay_ = false;
		playbackMode_ = PlaybackMode::StepByStep;
		singlyLinkedList.stepBackward();
	}
	ImGui::SameLine();
	if (ImGui::Button("Next")) {
		autoplay_ = false;
		playbackMode_ = PlaybackMode::StepByStep;
		singlyLinkedList.stepForward();
	}
	ImGui::SameLine();
	if (ImGui::Button("Final")) {
		autoplay_ = false;
		singlyLinkedList.jumpToFinal();
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset")) {
		autoplay_ = false;
		singlyLinkedList.jumpToStart();
	}
	ImGui::SameLine();
	ImGui::Checkbox("Autoplay##sll", &autoplay_);

	ImGui::SliderFloat("Playback Speed##sll", &playbackSpeed_, 0.25f, 5.0f, "%.2fx");

	ImGui::Text("Frame: %zu / %zu | Status: %s",
		singlyLinkedList.timeline.empty() ? 0 : (singlyLinkedList.cursor + 1),
		singlyLinkedList.timeline.size(),
		displayFrame.message.empty() ? singlyLinkedList.lastMessage.c_str() : displayFrame.message.c_str());

	float maxScroll = 0.0f;
	drawListCanvas(displayFrame, scrollOffset_, singlyLinkedList.interpolation, maxScroll);
	if (maxScroll > 0.0f) {
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderFloat("Horizontal Scroll##sll", &scrollOffset_, 0.0f, maxScroll, "%.0f");
	}
	drawCodePanel(displayFrame.codeLine, displayFrame.operationType);

	if (ImGui::Button("Back to menu")) {
		autoplay_ = false;
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}
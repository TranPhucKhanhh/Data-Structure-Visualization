#include <ui/heap.h>
#include <ui/common.h>
#include <ui/audio_imgui.h>

#include <imgui.h>

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "utils/SimpleFileDialog.h"

namespace {
	float lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	float easeInOut(float t) {
		t = std::clamp(t, 0.0f, 1.0f);
		return t * t * (3.0f - 2.0f * t);
	}

	const sf::Font* getHeapFont() {
		static sf::Font font;
		static bool attempted = false;
		static bool loaded = false;

		if (!attempted) {
			attempted = true;
			const std::string font_name = "/DroidSans.ttf";
			const auto path = std::filesystem::path(std::string(ASSET_FONT + font_name));
			if (std::filesystem::exists(path)) {
				if (font.openFromFile(path)) {
					loaded = true;
				}
			}
		}

		return loaded ? &font : nullptr;
	}

	const char* kCreateCode[] = {
		"1  FUNCTION createHeap(values):",
		"2      heap = values",
		"3      FOR i = heap.size / 2 - 1 DOWNTO 0:",
		"4          current = i",
		"5          WHILE true:",
		"6              left = 2 * current + 1; right = 2 * current + 2; target = current",
		"7              IF left < heap.size AND !compare(target, left): target = left",
		"8              IF right < heap.size AND !compare(target, right): target = right",
		"9              IF target == current: BREAK",
		"10             SWAP(heap[current], heap[target]); current = target",
		"11     RETURN heap"
	};

	const char* kSearchCode[] = {
		"1  FUNCTION search(value):",
		"2      FOR i = 0 TO heap.size - 1:",
		"3          IF heap[i] == value: RETURN true",
		"4      RETURN false"
	};

	const char* kInsertCode[] = {
		"1  FUNCTION insert(value):",
		"2      heap.push_back(value)",
		"3      i = heap.size - 1",
		"4      WHILE i > 0 AND !compare(parent(i), i):",
		"5          SWAP(heap[i], heap[parent(i)])",
		"6          i = parent(i)",
		"7      RETURN heap"
	};

	const char* kRemoveCode[] = {
		"1  FUNCTION removeTop():",
		"2      IF heap.empty(): RETURN",
		"3      heap[0] = heap.back(); heap.pop_back()",
		"4      current = 0",
		"5      WHILE true:",
		"6          left = 2 * current + 1; right = 2 * current + 2; target = current",
		"7          IF left < heap.size AND !compare(target, left): target = left",
		"8          IF right < heap.size AND !compare(target, right): target = right",
		"9          IF target == current: BREAK",
		"10         SWAP(heap[current], heap[target]); current = target",
		"11     RETURN heap"
	};

	const char* kUpdateCode[] = {
		"1  FUNCTION update(oldValue, newValue):",
		"2      FOR i = 0 TO heap.size - 1:",
		"3          IF heap[i] == oldValue: BREAK",
		"4      IF i == heap.size: RETURN false",
		"5      heap[i] = newValue",
		"6      IF i > 0 AND !compare(parent(i), i):",
		"7          WHILE i > 0 AND !compare(parent(i), i):",
		"8              SWAP(heap[i], heap[parent(i)])",
		"9              i = parent(i)",
		"10     ELSE:",
		"11         WHILE true:",
		"12             left = 2 * i + 1; right = 2 * i + 2; target = i",
		"13             IF left < heap.size AND !compare(target, left): target = left",
		"14             IF right < heap.size AND !compare(target, right): target = right",
		"15             IF target == i: BREAK",
		"16             SWAP(heap[i], heap[target]); i = target",
		"17     RETURN true"
	};

	void pickCodeBlock(int opIndex, const char**& codeArray, int& lineCount, const char*& title) {
		switch (opIndex) {
		case 0:
			codeArray = kCreateCode;
			lineCount = 11;
			title = "CREATE";
			break;
		case 1:
			codeArray = kSearchCode;
			lineCount = 4;
			title = "SEARCH";
			break;
		case 2:
			codeArray = kInsertCode;
			lineCount = 7;
			title = "INSERT";
			break;
		case 3:
			codeArray = kRemoveCode;
			lineCount = 11;
			title = "REMOVE";
			break;
		case 4:
			codeArray = kUpdateCode;
			lineCount = 17;
			title = "UPDATE";
			break;
		default:
			codeArray = nullptr;
			lineCount = 0;
			title = "HEAP";
			break;
		}
	}

	std::vector<int> mapInstructionToCodeLines(const HeapInstruction* instruction, int opIndex, bool operationFinished) {
		if (operationFinished && !(instruction != nullptr && instruction->heap_op == HeapOp::NotFound)) {
			switch (opIndex) {
			case 0:
				return {11};
			case 2:
				return {7};
			case 3:
				return {11};
			case 4:
				return {17};
			default:
				break;
			}
		}

		if (instruction == nullptr) {
			return {1};
		}

		switch (opIndex) {
		case 0:
			if (instruction->heap_op == HeapOp::ReturnHeap) {
				return {11};
			}
			if (instruction->heap_op == HeapOp::SwapLeftChild || instruction->heap_op == HeapOp::SwapRightChild) {
				return {5, 6, 7, 8, 10};
			}
			if (instruction->heap_op == HeapOp::HeapifyDownDone) {
				return {5, 6, 7, 8, 9};
			}
			return {3, 4};
		case 1:
			if (instruction->heap_op == HeapOp::FoundValue) {
				return {3};
			}
			if (instruction->heap_op == HeapOp::NotFound) {
				return {4};
			}
			return {2};
		case 2:
			if (instruction->heap_op == HeapOp::ReturnHeap) {
				return {7};
			}
			if (instruction->heap_op == HeapOp::AddBackValue) {
				return {2, 3};
			}
			if (instruction->heap_op == HeapOp::SwapParent) {
				return {4, 5, 6};
			}
			return {7};
		case 3:
			if (instruction->heap_op == HeapOp::ReturnHeap) {
				return {11};
			}
			if (instruction->heap_op == HeapOp::MoveBackToTop) {
				return {2, 3, 4};
			}
			if (instruction->heap_op == HeapOp::SwapLeftChild || instruction->heap_op == HeapOp::SwapRightChild) {
				return {5, 6, 7, 8, 10};
			}
			if (instruction->heap_op == HeapOp::HeapifyDownDone) {
				return {5, 6, 7, 8, 9};
			}
			return {11};
		case 4:
			if (instruction->heap_op == HeapOp::ReturnHeap) {
				return {17};
			}
			if (instruction->heap_op == HeapOp::VisitStraight) {
				return {2, 3};
			}
			if (instruction->heap_op == HeapOp::UpdateValue) {
				return {5};
			}
			if (instruction->heap_op == HeapOp::SwapParent) {
				return {6, 7, 8, 9};
			}
			if (instruction->heap_op == HeapOp::SwapLeftChild || instruction->heap_op == HeapOp::SwapRightChild) {
				return {10, 11, 12, 13, 14, 16};
			}
			if (instruction->heap_op == HeapOp::HeapifyDownDone) {
				return {10, 11, 12, 13, 14, 15};
			}
			if (instruction->heap_op == HeapOp::NotFound) {
				return {4};
			}
			return {2, 3};
		default:
			return {1};
		}
	}

	std::string instructionToComment(const HeapInstruction* instruction) {
		if (instruction == nullptr) {
			return "Ready";
		}

		switch (instruction->heap_op) {
		case HeapOp::SwapLeftChild:
			return "Swap with left child";
		case HeapOp::SwapRightChild:
			return "Swap with right child";
		case HeapOp::SwapParent:
			return "Swap with parent";
		case HeapOp::VisitStraight:
			return "Visit next node";
		case HeapOp::UpdateValue:
			return std::string("Update value to ") + std::to_string(instruction->data);
		case HeapOp::AddBackValue:
			return std::string("Add ") + std::to_string(instruction->data) + " at back";
		case HeapOp::MoveBackToTop:
			return "Move back node to top";
		case HeapOp::HeapifyDownDone:
			return "Heap property satisfied";
		case HeapOp::ReturnHeap:
			return "Return heap";
		case HeapOp::FoundValue:
			return "Found value";
		case HeapOp::NotFound:
			return "Value not found";
		default:
			return "Processing";
		}
	}
}

HeapUI::HeapUI() {}

void HeapUI::rebuildViewFromStep(int stepIndex) {
	timelineHeap_ = timelineBaseHeap_;
	activeIndex_ = -1;
	secondaryIndex_ = -1;
	displayCursorIndex_ = timelineHeap_.empty() ? -1 : 0;

	const int clamped = std::clamp(stepIndex, 0, static_cast<int>(currentSteps_.size()));
	for (int i = 0; i < clamped; ++i) {
		const HeapInstruction& step = currentSteps_[i];

		int _old_cursor = displayCursorIndex_;
        heap.applyInstructions(timelineHeap_, step, displayCursorIndex_);

        if (step.heap_op == HeapOp::SwapParent ||
            step.heap_op == HeapOp::SwapLeftChild ||
            step.heap_op == HeapOp::SwapRightChild)
        {
            secondaryIndex_ = _old_cursor;
        }
        else
        {
            secondaryIndex_ = -1;
        }
	}

	activeIndex_ = displayCursorIndex_;
}

void HeapUI::startTimeline(std::vector<HeapInstruction>&& steps, const std::vector<int>& baseData, int sourceMenuIndex, const std::string& fallbackMessage) {
	currentSteps_ = std::move(steps);
	timelineBaseHeap_ = baseData;
	timelineHeap_ = baseData;
	currentStepIndex_ = 0;
	transitionFromStep_ = 0;
	transitionToStep_ = 0;
	stepTransitioning_ = false;
	stepTransitionProgress_ = 1.0f;
	autoplayAccumulator_ = 0.0f;
	autoplay_ = true;
	playbackMode_ = HeapPlaybackMode::RunAtOnce;
	lastOperationMenuIndex_ = sourceMenuIndex;
	operationResult_ = fallbackMessage;
	rebuildViewFromStep(0);
}

void HeapUI::startStepTransition(int targetStep) {
	const int clampedTarget = std::clamp(targetStep, 0, static_cast<int>(currentSteps_.size()));
	if (clampedTarget == currentStepIndex_) {
		stepTransitioning_ = false;
		stepTransitionProgress_ = 1.0f;
		transitionFromStep_ = currentStepIndex_;
		transitionToStep_ = currentStepIndex_;
		return;
	}

	transitionFromStep_ = currentStepIndex_;
	transitionToStep_ = clampedTarget;
	stepTransitionProgress_ = 0.0f;
	stepTransitioning_ = true;
}

void HeapUI::draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vpPos = viewport->Pos;
	const ImVec2 vpSize = viewport->Size;
	const float layoutScale = std::max(0.70f, std::min(vpSize.x / 1920.0f, vpSize.y / 1080.0f));
	const float controlScale = std::max(0.90f, layoutScale);
	const float dt = ImGui::GetIO().DeltaTime;
	const float foldLerp = 1.0f - std::exp(-14.0f * dt);

	operationPanelOpenT_ = std::clamp(lerp(operationPanelOpenT_, operationPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
	commentPanelOpenT_ = std::clamp(lerp(commentPanelOpenT_, commentPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
	codePanelOpenT_ = std::clamp(lerp(codePanelOpenT_, codePanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);

	ImGui::SetNextWindowPos(vpPos);
	ImGui::SetNextWindowSize(ImVec2(vpSize.x, 44.0f * layoutScale));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.06f, 0.98f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f * layoutScale, 8.0f * layoutScale));
	if (ImGui::Begin("##HeapTopBar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetWindowFontScale(layoutScale);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.24f, 0.33f, 0.65f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.34f, 0.46f, 0.80f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.84f, 0.88f, 0.94f, 1.0f));

		if (AudioButton("MAIN MENU", ImVec2(112.0f * layoutScale, 26.0f * layoutScale))) {
			uiConfig.state = UIState::Menu;
		}

		auto navButton = [&](const char* label, UIState target, bool active) {
			ImGui::SameLine();
			if (active) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.30f, 0.42f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
			}
			if (AudioButton(label, ImVec2(132.0f * layoutScale, 26.0f * layoutScale))) {
				uiConfig.state = target;
			}
			if (active) {
				ImGui::PopStyleColor(2);
			}
		};

		navButton("LINKED LIST", UIState::SinglyLinkedList, uiConfig.state == UIState::SinglyLinkedList);
		navButton("TRIE", UIState::Trie, uiConfig.state == UIState::Trie);
		navButton("HEAP", UIState::Heap, uiConfig.state == UIState::Heap);
		navButton("SHORTEST PATH", UIState::ShortestPath, uiConfig.state == UIState::ShortestPath);

		ImGui::SameLine();
		const float resetButtonWidth = 116.0f * layoutScale;
		ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), vpSize.x - resetButtonWidth - 20.0f * layoutScale));
		if (AudioButton("Reset View", ImVec2(resetButtonWidth, 26.0f * layoutScale))) {
			zoomScale_ = 1.0f;
			scrollOffsetX_ = 0.0f;
			scrollOffsetY_ = 0.0f;
			isCanvasDragging_ = false;
		}

		ImGui::PopStyleColor(4);
	}
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	auto switchHeapType = [&]() {
		const std::vector<int> current = heap.getData();
		heap.swapType();
		heap.clear();
		std::vector<HeapInstruction> steps = heap.initFromListStep(current);
		startTimeline(std::move(steps), current, 0, heap.getType() == HeapType::MinHeap ? "Switched to Min Heap" : "Switched to Max Heap");
	};

	const float drawerBottomY = vpPos.y + vpSize.y - 300.0f * layoutScale;
	ImGui::SetNextWindowPos(ImVec2(vpPos.x, drawerBottomY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(52.0f * layoutScale, 200.0f * layoutScale), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
	if (ImGui::Begin("Operation Toggle##Heap", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings)) {
		ImGui::SetWindowFontScale(controlScale);
		ImGui::SetCursorPosY(84.0f * layoutScale);
		if (AudioButton(operationPanelCollapsed_ ? ">" : "<", ImVec2(34.0f * layoutScale, 32.0f * layoutScale))) {
			operationPanelCollapsed_ = !operationPanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	const float operationPanelWidth = 190.0f * layoutScale * operationPanelOpenT_;
	if (operationPanelWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(vpPos.x + 52.0f * layoutScale, drawerBottomY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(operationPanelWidth, 200.0f * layoutScale), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
		if (ImGui::Begin("Operations##HeapOperations", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar)) {
			ImGui::SetWindowFontScale(controlScale);
			if (operationPanelOpenT_ > 0.6f) {
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.14f, 0.48f, 0.22f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.58f, 0.30f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.12f, 0.42f, 0.20f, 0.98f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f * controlScale, 9.0f * controlScale));
				const char* operationNames[] = { "Create(A)", "Search", "Insert", "Remove", "Update", "Customize" };
				const float menuRowWidth = ImGui::GetContentRegionAvail().x;
				for (int i = 0; i < 6; ++i) {
					if (ImGui::Selectable(operationNames[i], operationMenuIndex_ == i, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(menuRowWidth, 0.0f))) {
						audioManager.playClick();
						operationMenuIndex_ = i;
					}
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor(3);
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();

		const float inputPanelWidth = 700.0f * layoutScale * operationPanelOpenT_;
		const float inputPanelHeight = 200.0f * layoutScale;
		const float inputPanelX = vpPos.x + 52.0f * layoutScale + operationPanelWidth + 2.0f * layoutScale;
		ImGui::SetNextWindowPos(ImVec2(inputPanelX, drawerBottomY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(inputPanelWidth, inputPanelHeight), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
		if (ImGui::Begin("Operation Inputs##HeapInputs", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar)) {
			ImGui::SetWindowFontScale(controlScale);
			if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 0) {
				ImGui::Text("Heap Type: %s", heap.getType() == HeapType::MinHeap ? "MinHeap" : "MaxHeap");
				ImGui::SameLine();
				if (AudioButton("Swap Type", ImVec2(100.0f * controlScale, 0.0f))) {
					switchHeapType();
				}

				if (AudioButton("Empty", ImVec2(56.0f * controlScale, 0.0f))) {
					heap.clear();
					startTimeline({}, {}, 0, "Initialized empty heap");
				}
				ImGui::SameLine();
				ImGui::TextUnformatted("N =");
				ImGui::SameLine();
				ImGui::PushItemWidth(110.0f * controlScale);
				ImGui::InputInt("##HeapCreateCount", &randomCount_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (AudioButton("Random", ImVec2(78.0f * controlScale, 0.0f))) {
					const std::vector<int> values = heap.generateRandomValues(randomCount_);
					heap.clear();
					std::vector<HeapInstruction> steps = heap.initFromListStep(values);
					startTimeline(std::move(steps), values, 0, "Initialized random heap");
				}

				ImGui::Separator();
				ImGui::TextUnformatted("A =");
				ImGui::SameLine();
				ImGui::PushItemWidth(260.0f * controlScale);
				ImGui::InputText("##HeapCreateValues", createValues_.data(), createValues_.size());
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (AudioButton("Go", ImVec2(56.0f * controlScale, 0.0f))) {
					const std::vector<int> values = heap.parseIntegers(createValues_.data());
					if (values.empty()) {
						operationResult_ = "Create failed: enter comma-separated integers";
					}
					else {
						heap.clear();
						std::vector<HeapInstruction> steps = heap.initFromListStep(values);
						startTimeline(std::move(steps), values, 0, "Initialized from input list");
					}
				}

				const float browseButtonW = 110.0f * controlScale;
				const float loadButtonW = 92.0f * controlScale;
				const float pathInputW = 320.0f * controlScale;

				ImGui::PushItemWidth(pathInputW);
				ImGui::InputText(".txt path", txtPath_.data(), txtPath_.size());
				ImGui::PopItemWidth();

				ImGui::SameLine();
				if (AudioButton("Browse File", ImVec2(browseButtonW, 0.0f))) {
					std::string selectedPath = cr::utils::SimpleFileDialog::dialog ();
					if (!selectedPath.empty()) {
						std::snprintf(txtPath_.data(), txtPath_.size(), "%s", selectedPath.c_str());
					}
				}

				ImGui::SameLine();
				if (AudioButton("Load txt", ImVec2(loadButtonW, 0.0f))) {
					try {
						const std::vector<int> values = heap.loadValuesFromFile(txtPath_.data());
						heap.clear();
						std::vector<HeapInstruction> steps = heap.initFromListStep(values);
						startTimeline(std::move(steps), values, 0, "Initialized from text file");
					}
					catch (...) {
						operationResult_ = "Load failed: cannot open file";
					}
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 1) {
				ImGui::TextUnformatted("Value:");
				ImGui::SameLine();
				ImGui::PushItemWidth(100.0f * controlScale);
				ImGui::InputInt("##HeapSearchValue", &searchValue_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (AudioButton("Search", ImVec2(90.0f * controlScale, 0.0f))) {
					const std::vector<int> base = heap.getData();
					std::vector<HeapInstruction> steps = heap.searchValueStep(searchValue_);
					const bool found = !steps.empty() && steps.back().heap_op == HeapOp::FoundValue;
					startTimeline(std::move(steps), base, 1, found ? "Found" : "Not Found");
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 2) {
				ImGui::TextUnformatted("Value:");
				ImGui::SameLine();
				ImGui::PushItemWidth(100.0f * controlScale);
				ImGui::InputInt("##HeapInsertValue", &insertValue_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (AudioButton("Insert", ImVec2(90.0f * controlScale, 0.0f))) {
					const std::vector<int> base = heap.getData();
					std::vector<HeapInstruction> steps = heap.insertValueStep(insertValue_);
					startTimeline(std::move(steps), base, 2, "Inserted value");
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 3) {
				if (AudioButton("Remove Top", ImVec2(110.0f * controlScale, 0.0f))) {
					const std::vector<int> base = heap.getData();
					std::vector<HeapInstruction> steps = heap.deleteTopStep();
					startTimeline(std::move(steps), base, 3, "Removed top node");
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 4) {
				ImGui::TextUnformatted("Old:");
				ImGui::SameLine();
				ImGui::PushItemWidth(100.0f * controlScale);
				ImGui::InputInt("##HeapUpdateOld", &updateOldValue_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::TextUnformatted("New:");
				ImGui::SameLine();
				ImGui::PushItemWidth(100.0f * controlScale);
				ImGui::InputInt("##HeapUpdateNew", &updateNewValue_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (AudioButton("Update", ImVec2(90.0f * controlScale, 0.0f))) {
					const std::vector<int> base = heap.getData();
					std::vector<HeapInstruction> steps = heap.updateValueStep(updateOldValue_, updateNewValue_);
					startTimeline(std::move(steps), base, 4, "Update operation");
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 5) {
				ImGui::Text("Heap Type: %s", heap.getType() == HeapType::MinHeap ? "MinHeap" : "MaxHeap");
				ImGui::SameLine();
				if (AudioButton("Swap Type##Customize", ImVec2(110.0f * controlScale, 0.0f))) {
					switchHeapType();
				}

				ImGui::PushItemWidth(230.0f * controlScale);
				ImGui::SliderFloat("Node Radius", &nodeRadius_, 12.0f, 44.0f, "%.1f");
				ImGui::SliderFloat("Edge Thickness", &edgeThickness_, 1.0f, 6.0f, "%.1f");
				ImGui::SliderFloat("Font Scale", &fontScale_, 0.7f, 1.8f, "%.2f");
				ImGui::PopItemWidth();
				ImGui::Checkbox("Code Overlay", &showCodeOverlay_);
				if (AudioButton("Reset Visuals", ImVec2(124.0f * controlScale, 0.0f))) {
					nodeRadius_ = 28.0f;
					edgeThickness_ = 3.0f;
					fontScale_ = 1.0f;
					zoomScale_ = 1.0f;
					scrollOffsetX_ = 0.0f;
					scrollOffsetY_ = 0.0f;
					canvasBgColor_ = sf::Color(255, 255, 255, 255);
					nodeBaseColor_ = sf::Color(230, 230, 230, 255);
					activeNodeColor_ = sf::Color(245, 158, 11, 255);
					secondaryNodeColor_ = sf::Color(156, 163, 175, 255);
					deleteNodeColor_ = sf::Color(239, 68, 68, 255);
					edgeColor_ = sf::Color(70, 70, 70, 255);
					valueTextColor_ = sf::Color(42, 42, 42, 255);
					indexTextColor_ = sf::Color(68, 68, 68, 255);
					highlightRingColor_ = sf::Color(255, 214, 102, 230);
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	int displayStepIndex = currentStepIndex_;
	if (stepTransitioning_ && stepTransitionProgress_ > 0.5f) {
		displayStepIndex = transitionToStep_;
	}
	displayStepIndex = std::clamp(displayStepIndex, 0, static_cast<int>(currentSteps_.size()));

	const HeapInstruction* activeInstruction = nullptr;
	if (!currentSteps_.empty() && displayStepIndex > 0) {
		activeInstruction = &currentSteps_[displayStepIndex - 1];
	}
	const std::string currentComment = (activeInstruction != nullptr)
		? instructionToComment(activeInstruction)
		: (operationResult_.empty() ? "Ready" : operationResult_);

	const float rightTabWidth = 26.0f * layoutScale;
	const float rightPanelWidth = 480.0f * layoutScale;
	const float commentY = vpPos.y + vpSize.y - 450.0f * layoutScale;
	const float commentH = 115.0f * layoutScale;
	const float codeY = vpPos.y + vpSize.y - 300.0f * layoutScale;
	const float codeH = 170.0f * layoutScale;
	const float rightTabX = vpPos.x + vpSize.x - rightTabWidth;

	const float animatedCommentWidth = rightPanelWidth * commentPanelOpenT_;
	if (animatedCommentWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(rightTabX - animatedCommentWidth, commentY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(animatedCommentWidth, commentH), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.55f, 0.16f, 0.96f));
		if (ImGui::Begin("Traversal Comment##HeapComment", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar)) {
			if (commentPanelOpenT_ > 0.55f) {
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.14f, 0.10f, 0.07f, 1.0f));
				ImGui::TextWrapped("%s", currentComment.c_str());
				ImGui::PopStyleColor();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	ImGui::SetNextWindowPos(ImVec2(rightTabX, commentY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(rightTabWidth, commentH), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.55f, 0.16f, 0.96f));
	if (ImGui::Begin("Traversal Comment Toggle##HeapCommentToggle", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(commentH * 0.5f - 12.0f * layoutScale);
		if (AudioButton(commentPanelCollapsed_ ? "<" : ">", ImVec2(18.0f * layoutScale, 24.0f * layoutScale))) {
			commentPanelCollapsed_ = !commentPanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	const float animatedCodeWidth = rightPanelWidth * codePanelOpenT_;
	if (animatedCodeWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(rightTabX - animatedCodeWidth, codeY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(animatedCodeWidth, codeH), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.78f, 0.08f, 0.96f));
		if (ImGui::Begin("Source Code##HeapCode", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar)) {
			if (codePanelOpenT_ > 0.55f) {
				const char** codeArray = nullptr;
				int lineCount = 0;
				const char* opTitle = "HEAP";
				pickCodeBlock(lastOperationMenuIndex_, codeArray, lineCount, opTitle);
				if (codeArray != nullptr && lineCount > 0) {
					const bool operationFinished = !stepTransitioning_ && !currentSteps_.empty() && currentStepIndex_ >= static_cast<int>(currentSteps_.size());
					const std::vector<int> highlightedLines = mapInstructionToCodeLines(activeInstruction, lastOperationMenuIndex_, operationFinished);
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.10f, 0.05f, 1.0f));
					ImGui::Text("Operation: %s", opTitle);
					ImGui::Separator();
					for (int i = 0; i < lineCount; ++i) {
						if (std::find(highlightedLines.begin(), highlightedLines.end(), i + 1) != highlightedLines.end()) {
							ImGui::TextColored(ImVec4(0.82f, 0.12f, 0.08f, 1.0f), "> %s", codeArray[i]);
						}
						else {
							ImGui::TextUnformatted(codeArray[i]);
						}
					}
					ImGui::PopStyleColor();
				}
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	ImGui::SetNextWindowPos(ImVec2(rightTabX, codeY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(rightTabWidth, codeH), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.78f, 0.08f, 0.96f));
	if (ImGui::Begin("Source Code Toggle##HeapCodeToggle", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(codeH * 0.5f - 12.0f * layoutScale);
		if (AudioButton(codePanelCollapsed_ ? "<" : ">", ImVec2(18.0f * layoutScale, 24.0f * layoutScale))) {
			codePanelCollapsed_ = !codePanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::SetNextWindowPos(ImVec2(vpPos.x, vpPos.y + vpSize.y - 48.0f * layoutScale), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(vpSize.x, 48.0f * layoutScale), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.03f, 0.03f, 0.98f));
	if (ImGui::Begin("Playback##HeapBottomPlayback", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetWindowFontScale(controlScale);
		ImGui::PushItemWidth(140.0f * controlScale);
		ImGui::SliderFloat("##HeapBottomPlaybackSpeed", &playbackSpeed_, 0.25f, 5.0f, "");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text("%.2gx", playbackSpeed_);

		ImGui::SameLine(vpSize.x * 0.43f);
		if (AudioButton("|<")) {
			autoplay_ = false;
			currentStepIndex_ = 0;
			autoplayAccumulator_ = 0.0f;
			stepTransitioning_ = false;
			stepTransitionProgress_ = 1.0f;
		}
		ImGui::SameLine();
		if (AudioButton("<")) {
			autoplay_ = false;
			playbackMode_ = HeapPlaybackMode::StepByStep;
			startStepTransition(currentStepIndex_ - 1);
		}
		ImGui::SameLine();
		if (AudioButton(autoplay_ ? "[]" : "|>")) {
			autoplay_ = !autoplay_;
			if (autoplay_) {
				playbackMode_ = HeapPlaybackMode::RunAtOnce;
			}
		}
		ImGui::SameLine();
		if (AudioButton(">")) {
			autoplay_ = false;
			playbackMode_ = HeapPlaybackMode::StepByStep;
			startStepTransition(currentStepIndex_ + 1);
		}
		ImGui::SameLine();
		if (AudioButton(">|")) {
			autoplay_ = false;
			currentStepIndex_ = static_cast<int>(currentSteps_.size());
			stepTransitioning_ = false;
			stepTransitionProgress_ = 1.0f;
		}

		if (!currentSteps_.empty()) {
			int frameIndex = currentStepIndex_;
			const int maxFrame = static_cast<int>(currentSteps_.size());
			const float timelineWidth = std::max(220.0f * layoutScale, vpSize.x * 0.30f * layoutScale);
			ImGui::SameLine(vpSize.x * 0.58f);
			ImGui::PushItemWidth(timelineWidth);
			if (ImGui::SliderInt("##HeapBottomTimeline", &frameIndex, 0, maxFrame, "")) {
				autoplay_ = false;
				startStepTransition(std::clamp(frameIndex, 0, maxFrame));
			}
			ImGui::PopItemWidth();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	if (stepTransitioning_) {
		const float safeDuration = std::max(0.08f, stepTransitionDuration_ / std::max(0.25f, playbackSpeed_));
		stepTransitionProgress_ += dt / safeDuration;
		if (stepTransitionProgress_ >= 1.0f) {
			stepTransitionProgress_ = 1.0f;
			stepTransitioning_ = false;
			currentStepIndex_ = transitionToStep_;
		}
	}

	if (!ImGui::GetIO().WantTextInput && autoplay_ && playbackMode_ == HeapPlaybackMode::RunAtOnce && currentStepIndex_ < static_cast<int>(currentSteps_.size()) && !stepTransitioning_) {
		autoplayAccumulator_ += dt * playbackSpeed_;
		constexpr float kStepInterval = 0.45f;
		while (autoplayAccumulator_ >= kStepInterval && currentStepIndex_ < static_cast<int>(currentSteps_.size()) && !stepTransitioning_) {
			startStepTransition(currentStepIndex_ + 1);
			autoplayAccumulator_ -= kStepInterval;
		}
		if (currentStepIndex_ >= static_cast<int>(currentSteps_.size())) {
			autoplay_ = false;
			autoplayAccumulator_ = 0.0f;
		}
	}
}

void HeapUI::drawSfml(sf::RenderWindow& window) {
	const sf::Vector2u size = window.getSize();
	const sf::Font* font = getHeapFont();

	sf::RectangleShape background(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
	background.setFillColor(canvasBgColor_);
	window.draw(background);

	const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
	const bool mouseInsideCanvas =
		mousePos.x >= 0 && mousePos.y >= 0 &&
		mousePos.x < static_cast<int>(size.x) && mousePos.y < static_cast<int>(size.y);
	const bool canDragCanvas = uiConfig.windowFocused && mouseInsideCanvas && !ImGui::GetIO().WantCaptureMouse;

	if (canDragCanvas) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (std::abs(wheel) > 0.001f) {
			const float zoomStep = 1.0f + wheel * 0.12f;
			zoomScale_ = std::clamp(zoomScale_ * zoomStep, 0.55f, 2.2f);
		}
	}

	const bool isDragPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
	if (canDragCanvas && isDragPressed) {
		if (!isCanvasDragging_) {
			isCanvasDragging_ = true;
			lastDragMousePos_ = mousePos;
		}
		else {
			const int deltaX = mousePos.x - lastDragMousePos_.x;
			const int deltaY = mousePos.y - lastDragMousePos_.y;
			scrollOffsetX_ += static_cast<float>(deltaX);
			scrollOffsetY_ += static_cast<float>(deltaY);
			lastDragMousePos_ = mousePos;
		}
	}
	else {
		isCanvasDragging_ = false;
	}

	scrollOffsetX_ = std::clamp(scrollOffsetX_, -10000.0f, 10000.0f);
	scrollOffsetY_ = std::clamp(scrollOffsetY_, -10000.0f, 10000.0f);

	struct HeapVisualState {
		std::vector<int> heapValues;
		int cursorIndex = -1;
		int activeIndex = -1;
		int secondaryIndex = -1;
	};

	auto buildState = [&](int stepIndex) {
		HeapVisualState state;
		state.heapValues = timelineBaseHeap_;
		state.cursorIndex = state.heapValues.empty() ? -1 : 0;

		const int clamped = std::clamp(stepIndex, 0, static_cast<int>(currentSteps_.size()));
		for (int i = 0; i < clamped; ++i) {
			const HeapInstruction& step = currentSteps_[static_cast<std::size_t>(i)];
			switch (step.heap_op) {
			case HeapOp::AddBackValue:
				state.heapValues.push_back(step.data);
				state.cursorIndex = static_cast<int>(state.heapValues.size()) - 1;
				state.activeIndex = state.cursorIndex;
				state.secondaryIndex = -1;
				break;
			case HeapOp::SwapParent: {
				const int parent = step.data;
				if (state.cursorIndex >= 0 && state.cursorIndex < static_cast<int>(state.heapValues.size()) &&
					parent >= 0 && parent < static_cast<int>(state.heapValues.size())) {
					std::swap(state.heapValues[static_cast<std::size_t>(state.cursorIndex)], state.heapValues[static_cast<std::size_t>(parent)]);
					state.secondaryIndex = state.cursorIndex;
					state.cursorIndex = parent;
					state.activeIndex = state.cursorIndex;
				}
				break;
			}
			case HeapOp::SwapLeftChild:
			case HeapOp::SwapRightChild: {
				const int child = step.data;
				if (state.cursorIndex >= 0 && state.cursorIndex < static_cast<int>(state.heapValues.size()) &&
					child >= 0 && child < static_cast<int>(state.heapValues.size())) {
					std::swap(state.heapValues[static_cast<std::size_t>(state.cursorIndex)], state.heapValues[static_cast<std::size_t>(child)]);
					state.secondaryIndex = state.cursorIndex;
					state.cursorIndex = child;
					state.activeIndex = state.cursorIndex;
				}
				break;
			}
			case HeapOp::MoveBackToTop:
				if (!state.heapValues.empty()) {
					state.heapValues[0] = state.heapValues.back();
					state.heapValues.pop_back();
					state.cursorIndex = state.heapValues.empty() ? -1 : 0;
					state.activeIndex = state.cursorIndex;
					state.secondaryIndex = -1;
				}
				break;
			case HeapOp::HeapifyDownDone:
				if (step.data >= 0 && step.data < static_cast<int>(state.heapValues.size())) {
					state.cursorIndex = step.data;
					state.activeIndex = state.cursorIndex;
				}
				state.secondaryIndex = -1;
				break;
			case HeapOp::ReturnHeap:
				state.secondaryIndex = -1;
				break;
			case HeapOp::VisitStraight:
				if (!state.heapValues.empty()) {
					const bool useExplicitIndex = (lastOperationMenuIndex_ == 0 || lastOperationMenuIndex_ == 1 || lastOperationMenuIndex_ == 4);
					if (useExplicitIndex && step.data >= 0 && step.data < static_cast<int>(state.heapValues.size())) {
						state.cursorIndex = step.data;
					}
					else if (state.cursorIndex < 0) {
						state.cursorIndex = 0;
					}
					else {
						state.cursorIndex = std::min(state.cursorIndex + 1, static_cast<int>(state.heapValues.size()) - 1);
					}
					state.activeIndex = state.cursorIndex;
					state.secondaryIndex = -1;
				}
				break;
			case HeapOp::UpdateValue:
				if (state.cursorIndex >= 0 && state.cursorIndex < static_cast<int>(state.heapValues.size())) {
					state.heapValues[static_cast<std::size_t>(state.cursorIndex)] = step.data;
					state.activeIndex = state.cursorIndex;
					state.secondaryIndex = -1;
				}
				break;
			case HeapOp::FoundValue:
				state.activeIndex = state.cursorIndex;
				state.secondaryIndex = -1;
				break;
			case HeapOp::NotFound:
				state.activeIndex = state.cursorIndex;
				state.secondaryIndex = -1;
				break;
			}
		}

		return state;
	};

	const int fromStep = stepTransitioning_ ? transitionFromStep_ : currentStepIndex_;
	const int toStep = stepTransitioning_ ? transitionToStep_ : currentStepIndex_;
	const float transitionT = stepTransitioning_ ? easeInOut(stepTransitionProgress_) : 1.0f;
	const HeapVisualState fromState = buildState(fromStep);
	const HeapVisualState toState = buildState(toStep);

	timelineHeap_ = toState.heapValues;
	activeIndex_ = toState.activeIndex;
	secondaryIndex_ = toState.secondaryIndex;
	displayCursorIndex_ = toState.cursorIndex;

	const HeapInstruction* transitionInstruction = nullptr;
	if (!currentSteps_.empty() && toStep > 0) {
		transitionInstruction = &currentSteps_[static_cast<std::size_t>(toStep - 1)];
	}

	const float radius = std::clamp(nodeRadius_ * zoomScale_, 12.0f, 84.0f);
	const float centerX = static_cast<float>(size.x) * 0.5f + scrollOffsetX_;
	const float topY = 120.0f + scrollOffsetY_;
	const float levelGap = 100.0f * zoomScale_;

	auto makePositions = [&](int count) {
		std::vector<sf::Vector2f> positions(static_cast<std::size_t>(std::max(0, count)));
		for (int i = 0; i < count; ++i) {
			int depth = 0;
			for (int k = i + 1; k > 1; k /= 2) {
				++depth;
			}
			const int levelStart = (1 << depth) - 1;
			const int indexInLevel = i - levelStart;
			const int levelCount = 1 << depth;
			const float spread = std::max(180.0f, 760.0f * zoomScale_) / static_cast<float>(1 << depth);
			const float x = centerX + (static_cast<float>(indexInLevel) - (static_cast<float>(levelCount - 1) * 0.5f)) * spread * 2.0f;
			const float y = topY + static_cast<float>(depth) * levelGap;
			positions[static_cast<std::size_t>(i)] = sf::Vector2f(x, y);
		}
		return positions;
	};

	auto drawEdge = [&](const sf::Vector2f& from, const sf::Vector2f& to, sf::Color color) {
		sf::Vertex line[2];
		line[0].position = from;
		line[0].color = color;
		line[1].position = to;
		line[1].color = color;
		window.draw(line, 2, sf::PrimitiveType::Lines);
	};

	auto drawNode = [&](float x, float y, int value, int index, sf::Color fill, bool highlightRing, std::uint8_t alpha) {
		sf::CircleShape node(radius);
		node.setOrigin(sf::Vector2f(radius, radius));
		node.setPosition(sf::Vector2f(x, y));
		node.setOutlineThickness(edgeThickness_ * zoomScale_);
		node.setOutlineColor(sf::Color(edgeColor_.r, edgeColor_.g, edgeColor_.b, alpha));
		node.setFillColor(sf::Color(fill.r, fill.g, fill.b, alpha));
		window.draw(node);

		if (highlightRing) {
			sf::CircleShape ring(radius + 8.0f);
			ring.setOrigin(sf::Vector2f(radius + 8.0f, radius + 8.0f));
			ring.setPosition(sf::Vector2f(x, y));
			ring.setFillColor(sf::Color::Transparent);
			ring.setOutlineThickness(3.0f);
			ring.setOutlineColor(sf::Color(highlightRingColor_.r, highlightRingColor_.g, highlightRingColor_.b, alpha));
			window.draw(ring);
		}

		if (font != nullptr) {
			sf::Text valueText(*font, std::to_string(value), static_cast<unsigned int>(20.0f * fontScale_));
			valueText.setFillColor(sf::Color(valueTextColor_.r, valueTextColor_.g, valueTextColor_.b, alpha));
			const sf::FloatRect vb = valueText.getLocalBounds();
			valueText.setPosition(sf::Vector2f(
				x - (vb.position.x + vb.size.x * 0.5f),
				y - (vb.position.y + vb.size.y * 0.5f)
			));
			window.draw(valueText);

			sf::Text indexText(*font, std::to_string(index), static_cast<unsigned int>(14.0f * fontScale_));
			indexText.setFillColor(sf::Color(indexTextColor_.r, indexTextColor_.g, indexTextColor_.b, alpha));
			const sf::FloatRect ib = indexText.getLocalBounds();
			indexText.setPosition(sf::Vector2f(
				x - (ib.position.x + ib.size.x * 0.5f),
				y - radius - 24.0f
			));
			window.draw(indexText);
		}
	};

	const int n = static_cast<int>(toState.heapValues.size());
	if (n == 0) {
		return;
	}

	const std::vector<sf::Vector2f> fromPositions = makePositions(static_cast<int>(fromState.heapValues.size()));
	const std::vector<sf::Vector2f> toPositions = makePositions(static_cast<int>(toState.heapValues.size()));

	for (int i = 1; i < static_cast<int>(toState.heapValues.size()); ++i) {
		const int parent = (i - 1) / 2;
		sf::Color edgeCol = edgeColor_;
		if ((i == toState.activeIndex && parent == toState.secondaryIndex) || (i == toState.secondaryIndex && parent == toState.activeIndex)) {
			edgeCol = highlightRingColor_;
		}
		drawEdge(toPositions[static_cast<std::size_t>(parent)], toPositions[static_cast<std::size_t>(i)], edgeCol);
	}

	std::vector<bool> skipTo(static_cast<std::size_t>(toState.heapValues.size()), false);
	std::vector<bool> skipFrom(static_cast<std::size_t>(fromState.heapValues.size()), false);

	if (stepTransitioning_ && transitionInstruction != nullptr &&
		(transitionInstruction->heap_op == HeapOp::SwapParent || transitionInstruction->heap_op == HeapOp::SwapLeftChild || transitionInstruction->heap_op == HeapOp::SwapRightChild)) {
		const int a = fromState.activeIndex;
		const int b = toState.activeIndex;
		if (a >= 0 && b >= 0 &&
			a < static_cast<int>(fromState.heapValues.size()) && b < static_cast<int>(fromState.heapValues.size()) &&
			a < static_cast<int>(toPositions.size()) && b < static_cast<int>(toPositions.size())) {
			skipFrom[static_cast<std::size_t>(a)] = true;
			skipFrom[static_cast<std::size_t>(b)] = true;
			skipTo[static_cast<std::size_t>(a)] = true;
			skipTo[static_cast<std::size_t>(b)] = true;

			const sf::Vector2f pa = fromPositions[static_cast<std::size_t>(a)];
			const sf::Vector2f pb = fromPositions[static_cast<std::size_t>(b)];
			const sf::Vector2f ta = toPositions[static_cast<std::size_t>(a)];
			const sf::Vector2f tb = toPositions[static_cast<std::size_t>(b)];

			drawNode(
				lerp(pa.x, tb.x, transitionT),
				lerp(pa.y, tb.y, transitionT),
				fromState.heapValues[static_cast<std::size_t>(a)],
				a,
				activeNodeColor_,
				true,
				255);
			drawNode(
				lerp(pb.x, ta.x, transitionT),
				lerp(pb.y, ta.y, transitionT),
				fromState.heapValues[static_cast<std::size_t>(b)],
				b,
				secondaryNodeColor_,
				false,
				255);
		}
	}

	if (stepTransitioning_ && transitionInstruction != nullptr && transitionInstruction->heap_op == HeapOp::AddBackValue) {
		const int newIndex = static_cast<int>(toState.heapValues.size()) - 1;
		if (newIndex >= 0 && newIndex < static_cast<int>(skipTo.size())) {
			skipTo[static_cast<std::size_t>(newIndex)] = true;
			const sf::Vector2f targetPos = toPositions[static_cast<std::size_t>(newIndex)];
			drawNode(targetPos.x, lerp(targetPos.y - 130.0f, targetPos.y, transitionT), toState.heapValues[static_cast<std::size_t>(newIndex)], newIndex, activeNodeColor_, true, static_cast<std::uint8_t>(255.0f * transitionT));
		}
	}

	if (stepTransitioning_ && transitionInstruction != nullptr && transitionInstruction->heap_op == HeapOp::MoveBackToTop && !fromState.heapValues.empty()) {
		const int lastIndex = static_cast<int>(fromState.heapValues.size()) - 1;
		if (lastIndex >= 0 && lastIndex < static_cast<int>(skipFrom.size())) {
			skipFrom[static_cast<std::size_t>(lastIndex)] = true;
			if (!toState.heapValues.empty()) {
				skipTo[0] = true;
				const sf::Vector2f fromPos = fromPositions[static_cast<std::size_t>(lastIndex)];
				const sf::Vector2f rootPos = toPositions[0];
				drawNode(
					lerp(fromPos.x, rootPos.x, transitionT),
					lerp(fromPos.y, rootPos.y, transitionT),
					fromState.heapValues[static_cast<std::size_t>(lastIndex)],
					0,
					activeNodeColor_,
					true,
					255);
			}
		}
	}

	for (int i = 0; i < static_cast<int>(toState.heapValues.size()); ++i) {
		if (skipTo[static_cast<std::size_t>(i)]) {
			continue;
		}

		sf::Color fill = nodeBaseColor_;
		if (i == toState.secondaryIndex) {
			fill = secondaryNodeColor_;
		}
		if (i == toState.activeIndex) {
			fill = activeNodeColor_;
		}
		if (transitionInstruction != nullptr && transitionInstruction->heap_op == HeapOp::NotFound && i == toState.activeIndex) {
			fill = deleteNodeColor_;
		}

		drawNode(
			toPositions[static_cast<std::size_t>(i)].x,
			toPositions[static_cast<std::size_t>(i)].y,
			toState.heapValues[static_cast<std::size_t>(i)],
			i,
			fill,
			i == toState.activeIndex,
			255);
	}

	if (stepTransitioning_ && transitionInstruction != nullptr && transitionInstruction->heap_op == HeapOp::UpdateValue && fromState.activeIndex >= 0 && fromState.activeIndex < static_cast<int>(fromState.heapValues.size())) {
		const int idx = fromState.activeIndex;
		const sf::Vector2f pos = toPositions[static_cast<std::size_t>(idx)];
		drawNode(pos.x, pos.y, fromState.heapValues[static_cast<std::size_t>(idx)], idx, secondaryNodeColor_, false, static_cast<std::uint8_t>(255.0f * (1.0f - transitionT)));
	}

	if (stepTransitioning_ && transitionInstruction != nullptr &&
		(transitionInstruction->heap_op == HeapOp::VisitStraight || transitionInstruction->heap_op == HeapOp::FoundValue || transitionInstruction->heap_op == HeapOp::NotFound) &&
		fromState.activeIndex >= 0 && toState.activeIndex >= 0 &&
		fromState.activeIndex < static_cast<int>(fromPositions.size()) && toState.activeIndex < static_cast<int>(toPositions.size())) {
		const sf::Vector2f fromPos = fromPositions[static_cast<std::size_t>(fromState.activeIndex)];
		const sf::Vector2f toPos = toPositions[static_cast<std::size_t>(toState.activeIndex)];
		sf::CircleShape travelRing(radius + 11.0f);
		travelRing.setOrigin(sf::Vector2f(radius + 11.0f, radius + 11.0f));
		travelRing.setPosition(sf::Vector2f(
			lerp(fromPos.x, toPos.x, transitionT),
			lerp(fromPos.y, toPos.y, transitionT)
		));
		travelRing.setFillColor(sf::Color::Transparent);
		travelRing.setOutlineThickness(3.0f);
		travelRing.setOutlineColor(highlightRingColor_);
		window.draw(travelRing);
	}

	const float stripY = topY - (radius * 2.0f) - 44.0f;
	const float cellW = std::max(44.0f, radius * 1.7f);
	const float stripStartX = centerX - (static_cast<float>(n) * cellW) * 0.5f;
	for (int i = 0; i < n; ++i) {
		const float x = stripStartX + static_cast<float>(i) * cellW;
		sf::RectangleShape cell(sf::Vector2f(cellW - 4.0f, 36.0f));
		cell.setPosition(sf::Vector2f(x, stripY));
		sf::Color stripFill = (i == toState.activeIndex) ? activeNodeColor_ : nodeBaseColor_;
		if (transitionInstruction != nullptr && transitionInstruction->heap_op == HeapOp::NotFound && i == toState.activeIndex) {
			stripFill = deleteNodeColor_;
		}
		cell.setFillColor(stripFill);
		cell.setOutlineThickness(1.8f);
		cell.setOutlineColor(edgeColor_);
		window.draw(cell);

		if (font != nullptr) {
			sf::Text text(*font, std::to_string(toState.heapValues[static_cast<std::size_t>(i)]), static_cast<unsigned int>(16.0f * fontScale_));
			text.setFillColor(valueTextColor_);
			const sf::FloatRect tb = text.getLocalBounds();
			text.setPosition(sf::Vector2f(
				x + (cellW - 4.0f) * 0.5f - (tb.position.x + tb.size.x * 0.5f),
				stripY + 18.0f - (tb.position.y + tb.size.y * 0.5f)
			));
			window.draw(text);
		}
	}
}


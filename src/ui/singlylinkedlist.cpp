#include <ui/singlylinkedlist.h>
#include <ui/common.h>

#include <SFML/Graphics.hpp>
#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <string>

namespace {
	enum class SLLCodeVariant {
		None,
		InsertFirst,
		InsertEnd,
		InsertMiddle,
		DeleteFirst,
		DeleteEnd,
		DeleteMiddle,
		Search,
		Update
	};

	//Linear interpolation helper for smooth animated transitions.
	float lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}

	const char* kInsertFirstCode[] = {
		"1  FUNCTION insertFirst(head, value):",
		"2      // no traversal needed (insert at head)",
		"3      newNode = Create Node(value)",
		"4      head = newNode",
		"5      newNode.next = oldHead"
	};

	const char* kInsertEndCode[] = {
		"1  FUNCTION insertEnd(head, value):",
		"2      temp = head; WHILE temp.next != null: temp = temp.next",
		"3      newNode = Create Node(value)",
		"4      temp.next = newNode",
		"5      newNode.next = null"
	};

	const char* kInsertMiddleCode[] = {
		"1  FUNCTION insertMiddle(head, value, position):",
		"2      temp = head; FOR i in [1..position-1]: temp = temp.next",
		"3      newNode = Create Node(value)",
		"4      temp.next = newNode",
		"5      newNode.next = nextNode"
	};

	const char* kDeleteFirstCode[] = {
		"1  FUNCTION deleteFirst(head):",
		"2      // no traversal needed (target is head)",
		"3      target = head",
		"4      remove target node",
		"5      head = head.next"
	};

	const char* kDeleteEndCode[] = {
		"1  FUNCTION deleteEnd(head):",
		"2      temp = head; WHILE temp.next != tail: temp = temp.next",
		"3      target = tail",
		"4      remove target node",
		"5      temp.next = null"
	};

	const char* kDeleteMiddleCode[] = {
		"1  FUNCTION deleteMiddle(head, position):",
		"2      temp = head; FOR i in [1..position-1]: temp = temp.next",
		"3      target = temp.next",
		"4      remove target node",
		"5      temp.next = nodeAfterTarget"
	};

	const char* kSearchCode[] = {
		"1  FUNCTION search(head, target):",
		"2      compare current node with target",
		"3      RETURN True (Found)",
		"4      RETURN False (Not Found)"
	};

	const char* kUpdateCode[] = {
		"1  FUNCTION update(head, oldValue, newValue):",
		"2      traverse to target node",
		"3      current.data = newValue"
	};

	//Resolve add pseudocode variant (first/end/middle) from current frame context.
	SLLCodeVariant detectAddVariant(const SLLFrame* frame, int activeLine)
	{
		if (frame == nullptr) {
			return SLLCodeVariant::None;
		}

		int index = -1;
		if (activeLine == 5) {
			index = frame->activeIndex;
		}
		else {
			index = frame->secondaryIndex;
		}

		if (index < 0) {
			return SLLCodeVariant::None;
		}

		const int currentSize = static_cast<int>(frame->values.size());
		const bool isStartFrame = frame->message.find("Start add operation") != std::string::npos;
		const int endIndex = isStartFrame ? currentSize : (currentSize - 1);

		if (index == 0) {
			return SLLCodeVariant::InsertFirst;
		}
		else if (index == endIndex) {
			return SLLCodeVariant::InsertEnd;
		}
		else {
			return SLLCodeVariant::InsertMiddle;
		}
	}

	//Resolve delete pseudocode variant (first/end/middle) from current frame context.
	SLLCodeVariant detectDeleteVariant(const SLLFrame* frame, int activeLine)
	{
		if (frame == nullptr) {
			return SLLCodeVariant::None;
		}

		if (frame->message.find("Move head to next node") != std::string::npos
			|| frame->message.find("Delete first") != std::string::npos
			|| frame->message.find("Deleted first node") != std::string::npos) {
			return SLLCodeVariant::DeleteFirst;
		}

		if (activeLine == 5) {
			return (frame->secondaryIndex < 0) ? SLLCodeVariant::DeleteEnd : SLLCodeVariant::DeleteMiddle;
		}

		if ((activeLine == 3 || activeLine == 4) && frame->activeIndex == 0) {
			return SLLCodeVariant::DeleteFirst;
		}

		return SLLCodeVariant::None;
	}

	//Convert timeline code line to displayed pseudocode line for the active variant.
	int mapTimelineLineToPseudoLine(SLLCodeVariant variant, int timelineLine)
	{
		switch (variant) {
		case SLLCodeVariant::InsertFirst:
			return std::clamp(timelineLine, 1, 5);
		case SLLCodeVariant::InsertEnd:
			return std::clamp(timelineLine, 1, 5);
		case SLLCodeVariant::InsertMiddle:
			return std::clamp(timelineLine, 1, 5);
		case SLLCodeVariant::DeleteFirst:
			return std::clamp(timelineLine, 1, 5);
		case SLLCodeVariant::DeleteEnd:
			return std::clamp(timelineLine, 1, 5);
		case SLLCodeVariant::DeleteMiddle:
			return std::clamp(timelineLine, 1, 5);
		case SLLCodeVariant::Search:
			return std::clamp(timelineLine, 1, 4);
		case SLLCodeVariant::Update:
			return std::clamp(timelineLine, 1, 3);
		default:
			return timelineLine;
		}
	}

	//Select pseudocode block and line count for the current operation/frame.
	void pickCodeBlock(SLLOperationType opType, int activeLine, const SLLFrame* frame, const char**& codeArray, int& lineCount, SLLCodeVariant* variantOut = nullptr)
	{
		static SLLCodeVariant lastAddVariant = SLLCodeVariant::InsertMiddle;
		static SLLCodeVariant lastDeleteVariant = SLLCodeVariant::DeleteMiddle;
		static bool addVariantLocked = false;

		if (variantOut != nullptr) {
			*variantOut = SLLCodeVariant::None;
		}
		switch (opType) {
		case SLLOperationType::Add:
			{
				if (frame != nullptr && frame->message.find("Start add operation") != std::string::npos) {
					addVariantLocked = false;
				}

				SLLCodeVariant variant = addVariantLocked ? lastAddVariant : detectAddVariant(frame, activeLine);
				if (variant == SLLCodeVariant::None) {
					variant = lastAddVariant;
				}
				else {
					lastAddVariant = variant;
					addVariantLocked = true;
				}
				if (variant == SLLCodeVariant::InsertFirst) {
					codeArray = kInsertFirstCode;
					lineCount = 5;
				}
				else if (variant == SLLCodeVariant::InsertEnd) {
					codeArray = kInsertEndCode;
					lineCount = 5;
				}
				else {
					variant = SLLCodeVariant::InsertMiddle;
					codeArray = kInsertMiddleCode;
					lineCount = 5;
				}
				if (variantOut != nullptr) *variantOut = variant;
			}
			break;
		case SLLOperationType::Delete:
			{
				SLLCodeVariant variant = detectDeleteVariant(frame, activeLine);
				if (variant == SLLCodeVariant::None) {
					variant = lastDeleteVariant;
				}
				if (variant == SLLCodeVariant::DeleteFirst) {
					codeArray = kDeleteFirstCode;
					lineCount = 5;
				}
				else if (variant == SLLCodeVariant::DeleteEnd) {
					codeArray = kDeleteEndCode;
					lineCount = 5;
				}
				else {
					variant = SLLCodeVariant::DeleteMiddle;
					codeArray = kDeleteMiddleCode;
					lineCount = 5;
				}
				lastDeleteVariant = variant;
				if (variantOut != nullptr) *variantOut = variant;
			}
			break;
		case SLLOperationType::Update:
			codeArray = kUpdateCode;
			lineCount = 3;
			if (variantOut != nullptr) *variantOut = SLLCodeVariant::Update;
			break;
		case SLLOperationType::Search:
			codeArray = kSearchCode;
			lineCount = 4;
			if (variantOut != nullptr) *variantOut = SLLCodeVariant::Search;
			break;
		default:
			addVariantLocked = false;
			codeArray = nullptr;
			lineCount = 0;
			if (variantOut != nullptr) *variantOut = SLLCodeVariant::None;
			break;
		}
	}

	//Optional floating code overlay used for quick operation-line inspection.
	void drawCodeOverlay(int activeLine, SLLOperationType opType)
	{
		const ImVec2 viewportPos = ImGui::GetMainViewport()->Pos;
		const ImVec2 viewportSize = ImGui::GetMainViewport()->Size;
		ImGui::SetNextWindowPos(ImVec2(viewportPos.x + viewportSize.x - 430.0f, viewportPos.y + 20.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImVec2(410.0f, 250.0f), ImGuiCond_FirstUseEver);

		if (!ImGui::Begin("Source Code Highlight##SLL", nullptr,
			ImGuiWindowFlags_NoCollapse)) {
			ImGui::End();
			return;
		}

		ImGui::TextUnformatted("Source Code Highlight");
		ImGui::Separator();

		const char** codeArray = nullptr;
		int lineCount = 0;
		SLLCodeVariant codeVariant = SLLCodeVariant::None;
		pickCodeBlock(opType, activeLine, nullptr, codeArray, lineCount, &codeVariant);
		const int highlightedLine = mapTimelineLineToPseudoLine(codeVariant, activeLine);
		switch (opType) {
		case SLLOperationType::Initialize:
			ImGui::TextUnformatted("Operation: Initialize");
			ImGui::End();
			return;
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
			ImGui::End();
			return;
		}

		ImGui::Separator();
		for (int i = 0; i < lineCount; ++i) {
			if ((i + 1) == highlightedLine) {
				ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.15f, 1.0f), "> %s", codeArray[i]);
			}
			else {
				ImGui::TextUnformatted(codeArray[i]);
			}
		}
		ImGui::End();
	}

	//Compute max horizontal pan based on current frame width and viewport size.
	float computeMaxScroll(const SLLFrame& frame, float viewportWidth, float nodeRadius)
	{
		const float radius = std::clamp(nodeRadius, 18.0f, 42.0f);
		const float diameter = radius * 2.0f;
		const float spacing = diameter + 50.0f;
		const float totalWidth = static_cast<float>(frame.values.size()) * spacing + 60.0f;
		return std::max(0.0f, totalWidth - viewportWidth);
	}

	//Load and cache a font for SFML node/value rendering.
	const sf::Font* getSllFont()
	{
		static sf::Font font;
		static bool attempted = false;
		static bool loaded = false;

		if (!attempted) {
			attempted = true;
			const std::filesystem::path candidates[] = {
				std::filesystem::path("../_deps/imgui-src/misc/fonts/DroidSans.ttf"),
				std::filesystem::path("../_deps/imgui-src/misc/fonts/Roboto-Medium.ttf"),
				std::filesystem::path("C:/Windows/Fonts/arial.ttf")
			};

			for (const auto& path : candidates) {
				if (font.openFromFile(path)) {
					loaded = true;
					break;
				}
			}
		}

		return loaded ? &font : nullptr;
	}
}

//Construct UI state for the Singly Linked List screen.
SinglyLinkedListUI::SinglyLinkedListUI() {
	// This is where you can initialize any resources or variables needed
}

//Render all ImGui controls and panels for the Singly Linked List visualizer.
void SinglyLinkedListUI::draw() {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 vpPos = viewport->Pos;
	const ImVec2 vpSize = viewport->Size;
	const float dt = ImGui::GetIO().DeltaTime;
	bool userDefinedInputActiveThisFrame = false;
	const bool wantsTextInput = ImGui::GetIO().WantTextInput;
	const float foldLerp = 1.0f - std::exp(-14.0f * dt);

	const float operationTarget = operationPanelCollapsed_ ? 0.0f : 1.0f;
	const float commentTarget = commentPanelCollapsed_ ? 0.0f : 1.0f;
	const float codeTarget = codePanelCollapsed_ ? 0.0f : 1.0f;
	operationPanelOpenT_ = lerp(operationPanelOpenT_, operationTarget, foldLerp);
	commentPanelOpenT_ = lerp(commentPanelOpenT_, commentTarget, foldLerp);
	codePanelOpenT_ = lerp(codePanelOpenT_, codeTarget, foldLerp);
	operationPanelOpenT_ = std::clamp(operationPanelOpenT_, 0.0f, 1.0f);
	commentPanelOpenT_ = std::clamp(commentPanelOpenT_, 0.0f, 1.0f);
	codePanelOpenT_ = std::clamp(codePanelOpenT_, 0.0f, 1.0f);

	const bool hasTimelineFrames = singlyLinkedList.hasTimeline();
	const SLLFrame displayFrame = singlyLinkedList.getInterpolatedFrame();
	int displayedCodeLine = displayFrame.codeLine;
	SLLOperationType displayedOpType = displayFrame.operationType;
	if (singlyLinkedList.interpolation.isTransitioning) {
		const SLLFrame& prevFrame = singlyLinkedList.interpolation.previousFrame;
		const SLLFrame& currFrame = singlyLinkedList.interpolation.currentFrame;
		const float t = singlyLinkedList.interpolation.transitionProgress;

		if (prevFrame.operationType == SLLOperationType::Delete && currFrame.operationType == SLLOperationType::Delete &&
			prevFrame.codeLine == 4 && currFrame.codeLine == 5) {
			displayedOpType = SLLOperationType::Delete;
			displayedCodeLine = (t < 0.65f) ? 4 : 5;
		}
	}

	ImGui::SetNextWindowPos(vpPos);
	ImGui::SetNextWindowSize(ImVec2(vpSize.x, 44.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.06f, 0.98f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
	if (ImGui::Begin("##SLLTopBar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.00f, 0.00f, 0.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.24f, 0.33f, 0.65f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.34f, 0.46f, 0.80f));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.84f, 0.88f, 0.94f, 1.0f));

		if (ImGui::Button("MAIN MENU", ImVec2(112.0f, 26.0f))) {
			uiConfig.state = UIState::Menu;
		}

		auto navButton = [&](const char* label, UIState target, bool active) {
			ImGui::SameLine();
			if (active) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.30f, 0.42f, 0.85f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			}
			if (ImGui::Button(label, ImVec2(132.0f, 26.0f))) {
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

		ImGui::PopStyleColor(4);
	}
	ImGui::End();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();

	const float drawerBottomY = vpPos.y + vpSize.y - 300.0f;
	ImGui::SetNextWindowPos(ImVec2(vpPos.x, drawerBottomY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(52.0f, 200.0f), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
	if (ImGui::Begin("Operation Toggle##SLL", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings)) {
		ImGui::SetCursorPosY(84.0f);
		if (ImGui::Button(operationPanelCollapsed_ ? ">" : "<", ImVec2(34.0f, 32.0f))) {
			operationPanelCollapsed_ = !operationPanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	const float operationPanelWidth = 190.0f * operationPanelOpenT_;
	if (operationPanelWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(vpPos.x + 52.0f, drawerBottomY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(operationPanelWidth, 200.0f), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
		if (ImGui::Begin("Operations##SLLOperations", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar)) {
			const bool usingMenuListFont = (menuCardTitleFont != nullptr);
			if (usingMenuListFont) {
				ImGui::PushFont(menuCardTitleFont);
			}
			if (operationPanelOpenT_ > 0.6f) {
				ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.14f, 0.48f, 0.22f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.58f, 0.30f, 0.95f));
				ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.12f, 0.42f, 0.20f, 0.98f));
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 9.0f));
				const char* operationNames[] = { "Create(A)", "Search", "Insert", "Remove", "Update", "Customize" };
				const float menuRowWidth = ImGui::GetContentRegionAvail().x;
				for (int i = 0; i < 6; ++i) {
					if (ImGui::Selectable(operationNames[i], operationMenuIndex_ == i, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(menuRowWidth, 0.0f))) {
						operationMenuIndex_ = i;
					}
				}
				ImGui::PopStyleVar();
				ImGui::PopStyleColor(3);
			}
			if (usingMenuListFont) {
				ImGui::PopFont();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();

		const float inputPanelWidth = 700.0f * operationPanelOpenT_;
		//const bool showCreateEditor = operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 0 && userDefinedListExpanded_;
		const float inputPanelHeight = 200.0f; //showCreateEditor ? 150.0f : 72.0f;
		const float inputPanelX = vpPos.x + 52.0f + operationPanelWidth + 2.0f;
		ImGui::SetNextWindowPos(ImVec2(inputPanelX, drawerBottomY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(inputPanelWidth, inputPanelHeight), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
		if (ImGui::Begin("Operation Inputs##SLLOpInputs", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoScrollbar)) {
			const bool usingMenuInputFont = (menuCardDescFont != nullptr);
			if (usingMenuInputFont) {
				ImGui::PushFont(menuCardDescFont);
			}
			if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 0) {
				if (ImGui::Button("Empty", ImVec2(56.0f, 0.0f))) {
					singlyLinkedList.initializeEmpty();
					userDefinedListExpanded_ = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("User Defined List", ImVec2(200.0f, 0.0f))) {
					userDefinedListExpanded_ = !userDefinedListExpanded_;
				}
				ImGui::SameLine();
				ImGui::TextUnformatted("N =");
				ImGui::SameLine();
				ImGui::PushItemWidth(100.0f);
				ImGui::InputInt("##CreateCount", &randomCount_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Random", ImVec2(78.0f, 0.0f))) {
					singlyLinkedList.initializeRandom(randomCount_, randomMin_, randomMax_);
					userDefinedListExpanded_ = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Random Sorted", ImVec2(122.0f, 0.0f))) {
					singlyLinkedList.initializeRandomSorted(randomCount_, randomMin_, randomMax_);
					userDefinedListExpanded_ = false;
				}

				if (userDefinedListExpanded_) {
					ImGui::Separator();
					ImGui::TextUnformatted("A =");
					ImGui::SameLine();
					ImGui::PushItemWidth(240.0f);
					ImGui::InputText("##UserDefinedValues", userDefinedList_.data(), userDefinedList_.size());
					userDefinedInputActiveThisFrame = ImGui::IsItemActive();
					ImGui::PopItemWidth();
					ImGui::SameLine();
					if (ImGui::Button("Go", ImVec2(56.0f, 0.0f))) {
						const std::vector<int> parsed = singlyLinkedList.parseIntegers(userDefinedList_.data());
						if (parsed.empty()) {
							singlyLinkedList.lastMessage = "Initialize failed: enter comma-separated integers";
							singlyLinkedList.rebuildIdleTimeline(singlyLinkedList.lastMessage, 1);
						}
						else {
							singlyLinkedList.initializeFromValues(parsed, "Initialized from user defined list");
						}
					}

					ImGui::PushItemWidth(320.0f);
					ImGui::InputText(".txt path", txtPath_.data(), txtPath_.size());
					ImGui::PopItemWidth();
					ImGui::SameLine();
					if (ImGui::Button("Load txt", ImVec2(92.0f, 0.0f))) {
						singlyLinkedList.initializeFromTextFile(txtPath_.data());
					}
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 1) {
				ImGui::TextUnformatted("Value:");
				ImGui::SameLine();
				ImGui::PushItemWidth(90.0f);
				ImGui::InputInt("##SearchValue", &searchValue_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Search", ImVec2(90.0f, 0.0f))) {
					singlyLinkedList.searchValueViz(searchValue_);
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 2) {
				ImGui::TextUnformatted("Index:");
				ImGui::SameLine();
				ImGui::PushItemWidth(90.0f);
				ImGui::InputInt("##InsertIndex", &addIndex_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::TextUnformatted("Value:");
				ImGui::SameLine();
				ImGui::PushItemWidth(90.0f);
				ImGui::InputInt("##InsertValue", &addValue_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Insert", ImVec2(90.0f, 0.0f))) {
					singlyLinkedList.addAtViz(static_cast<std::size_t>(std::max(0, addIndex_)), addValue_);
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 3) {
				ImGui::TextUnformatted("Index:");
				ImGui::SameLine();
				ImGui::PushItemWidth(90.0f);
				ImGui::InputInt("##RemoveIndex", &deleteIndex_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Remove", ImVec2(90.0f, 0.0f))) {
					singlyLinkedList.deleteAtViz(static_cast<std::size_t>(std::max(0, deleteIndex_)));
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 4) {
				ImGui::TextUnformatted("Index:");
				ImGui::SameLine();
				ImGui::PushItemWidth(90.0f);
				ImGui::InputInt("##UpdateIndex", &updateIndex_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				ImGui::TextUnformatted("Value:");
				ImGui::SameLine();
				ImGui::PushItemWidth(90.0f);
				ImGui::InputInt("##UpdateValue", &updateValue_);
				ImGui::PopItemWidth();
				ImGui::SameLine();
				if (ImGui::Button("Update", ImVec2(90.0f, 0.0f))) {
					singlyLinkedList.updateAtViz(static_cast<std::size_t>(std::max(0, updateIndex_)), updateValue_);
				}
			}
			else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 5) {
				ImGui::TextUnformatted("Style");
				ImGui::SameLine();
				const char* stylePresets[] = { "Classic", "Minimal", "Bold" };
				ImGui::PushItemWidth(150.0f);
				ImGui::Combo("##StylePreset", &visualStylePreset_, stylePresets, IM_ARRAYSIZE(stylePresets));
				ImGui::PopItemWidth();

				ImGui::PushItemWidth(240.0f);
				ImGui::SliderFloat("Node Size", &nodeRadius_, 18.0f, 44.0f, "%.1f");
				ImGui::SliderFloat("Border Size", &edgeThickness_, 1.0f, 8.0f, "%.1f");
				ImGui::SliderFloat("Font Scale", &fontScale_, 0.7f, 1.8f, "%.2f");
				ImGui::PopItemWidth();

				auto editColor = [&](const char* label, sf::Color& c) {
					float col[4] = {
						static_cast<float>(c.r) / 255.0f,
						static_cast<float>(c.g) / 255.0f,
						static_cast<float>(c.b) / 255.0f,
						static_cast<float>(c.a) / 255.0f
					};
					if (ImGui::ColorEdit4(label, col, ImGuiColorEditFlags_NoInputs)) {
						c = sf::Color(
							static_cast<std::uint8_t>(std::clamp(col[0], 0.0f, 1.0f) * 255.0f),
							static_cast<std::uint8_t>(std::clamp(col[1], 0.0f, 1.0f) * 255.0f),
							static_cast<std::uint8_t>(std::clamp(col[2], 0.0f, 1.0f) * 255.0f),
							static_cast<std::uint8_t>(std::clamp(col[3], 0.0f, 1.0f) * 255.0f)
						);
					}
				};

				editColor("Canvas", canvasBgColor_);
				editColor("Node", nodeBaseColor_);
				editColor("Active Node", activeNodeColor_);
				editColor("Secondary Node", secondaryNodeColor_);
				editColor("Edge", edgeColor_);
				editColor("Value Text", valueTextColor_);
				editColor("Index Text", indexTextColor_);

				if (ImGui::Button("Reset Visuals", ImVec2(120.0f, 0.0f))) {
					visualStylePreset_ = 0;
					nodeRadius_ = 28.0f;
					edgeThickness_ = 3.0f;
					fontScale_ = 1.0f;
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
			if (usingMenuInputFont) {
				ImGui::PopFont();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	const float rightTabWidth = 26.0f;
	const float rightPanelWidth = 480.0f;
	const float commentY = vpPos.y + vpSize.y - 450.0f;
	const float commentH = 115.0f;
	const float codeY = vpPos.y + vpSize.y - 300.0f;
	const float codeH = 170.0f;
	const float rightTabX = vpPos.x + vpSize.x - rightTabWidth;

	const float animatedCommentWidth = rightPanelWidth * commentPanelOpenT_;
	if (animatedCommentWidth > 6.0f) {
		ImGui::SetNextWindowPos(ImVec2(rightTabX - animatedCommentWidth, commentY), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(animatedCommentWidth, commentH), ImGuiCond_Always);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.55f, 0.16f, 0.96f));
		if (ImGui::Begin("Traversal Comment##SLLComment", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar)) {
			if (commentPanelOpenT_ > 0.55f) {
				const char* status = displayFrame.message.empty() ? singlyLinkedList.lastMessage.c_str() : displayFrame.message.c_str();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.14f, 0.10f, 0.07f, 1.0f));
				const bool usingCommentFont = (menuCardDescFont != nullptr);
				if (usingCommentFont) {
					ImGui::PushFont(menuCardDescFont);
				}
				ImGui::TextWrapped("%s", status);
				if (usingCommentFont) {
					ImGui::PopFont();
				}
				ImGui::PopStyleColor();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
	}

	ImGui::SetNextWindowPos(ImVec2(rightTabX, commentY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(rightTabWidth, commentH), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.55f, 0.16f, 0.96f));
	if (ImGui::Begin("Traversal Comment Toggle##SLLCommentToggle", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(commentH * 0.5f - 12.0f);
		if (ImGui::Button(commentPanelCollapsed_ ? "<" : ">", ImVec2(18.0f, 24.0f))) {
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
		if (ImGui::Begin("Source Code##SLLCode", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoScrollbar)) {
			if (codePanelOpenT_ > 0.55f && hasTimelineFrames) {
				const char** codeArray = nullptr;
				int lineCount = 0;
				SLLCodeVariant codeVariant = SLLCodeVariant::None;
				pickCodeBlock(displayedOpType, displayedCodeLine, &displayFrame, codeArray, lineCount, &codeVariant);
				const int highlightedLine = mapTimelineLineToPseudoLine(codeVariant, displayedCodeLine);
				if (codeArray != nullptr && lineCount > 0) {
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.15f, 0.10f, 0.05f, 1.0f));
					for (int i = 0; i < lineCount; ++i) {
						if ((i + 1) == highlightedLine) {
							const ImVec2 textPos = ImGui::GetCursorScreenPos();
							ImGui::TextColored(ImVec4(0.82f, 0.12f, 0.08f, 1.0f), "> %s", codeArray[i]);
							std::string boldLine = std::string("> ") + codeArray[i];
							ImGui::GetWindowDrawList()->AddText(
								ImVec2(textPos.x + 0.8f, textPos.y),
								IM_COL32(209, 30, 20, 255),
								boldLine.c_str());
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
	if (ImGui::Begin("Source Code Toggle##SLLCodeToggle", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::SetCursorPosY(codeH * 0.5f - 12.0f);
		if (ImGui::Button(codePanelCollapsed_ ? "<" : ">", ImVec2(18.0f, 24.0f))) {
			codePanelCollapsed_ = !codePanelCollapsed_;
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	ImGui::SetNextWindowPos(ImVec2(vpPos.x, vpPos.y + vpSize.y - 58.0f), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(vpSize.x, 48.0f), ImGuiCond_Always);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.03f, 0.03f, 0.98f));
	if (ImGui::Begin("Playback##SLLBottomPlayback", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar)) {
		ImGui::PushItemWidth(140.0f);
		ImGui::SliderFloat("##BottomPlaybackSpeed", &playbackSpeed_, 0.25f, 5.0f, "");
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::Text("%.2gx", playbackSpeed_);

		ImGui::SameLine(vpSize.x * 0.43f);
		if (ImGui::Button("|<")) {
			autoplay_ = false;
			singlyLinkedList.jumpToStart();
		}
		ImGui::SameLine();
		if (ImGui::Button("<")) {
			autoplay_ = false;
			playbackMode_ = PlaybackMode::StepByStep;
			singlyLinkedList.stepBackward();
		}
		ImGui::SameLine();
		if (ImGui::Button(autoplay_ ? "[]" : "|>")) {
			autoplay_ = !autoplay_;
			if (autoplay_) {
				playbackMode_ = PlaybackMode::RunAtOnce;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(">")) {
			autoplay_ = false;
			playbackMode_ = PlaybackMode::StepByStep;
			singlyLinkedList.stepForward();
		}
		ImGui::SameLine();
		if (ImGui::Button(">|")) {
			autoplay_ = false;
			singlyLinkedList.jumpToFinal();
		}

		if (!singlyLinkedList.timeline.empty()) {
			int frameIndex = static_cast<int>(singlyLinkedList.cursor);
			const int maxFrame = static_cast<int>(singlyLinkedList.timeline.size() - 1);
			ImGui::SameLine(vpSize.x * 0.58f);
			ImGui::PushItemWidth(vpSize.x * 0.36f);
			if (ImGui::SliderInt("##BottomTimeline", &frameIndex, 0, maxFrame, "")) {
				autoplay_ = false;
				singlyLinkedList.cursor = static_cast<std::size_t>(std::clamp(frameIndex, 0, maxFrame));
				singlyLinkedList.interpolation.isTransitioning = false;
				singlyLinkedList.interpolation.transitionProgress = 0.0f;
			}
			ImGui::PopItemWidth();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor();

	isEditingUserDefinedInput_ = userDefinedInputActiveThisFrame || wantsTextInput;
	if (isEditingUserDefinedInput_) {
		autoplay_ = false;
	}
	if (!isEditingUserDefinedInput_) {
		singlyLinkedList.updateInterpolation(dt);
		singlyLinkedList.updateAutoplay(dt, playbackSpeed_, autoplay_ && playbackMode_ == PlaybackMode::RunAtOnce);
	}
}

//Render SFML canvas content (nodes, edges, animations, panning, zoom).
void SinglyLinkedListUI::drawSfml(sf::RenderWindow& window)
{
	const SLLFrame frame = singlyLinkedList.getInterpolatedFrame();
	const SLLInterpolationState& interpolation = singlyLinkedList.interpolation;
	const sf::Vector2u size = window.getSize();
	const sf::Font* font = getSllFont();

	sf::Color styleNodeColor = nodeBaseColor_;
	sf::Color styleActiveColor = activeNodeColor_;
	sf::Color styleSecondaryColor = secondaryNodeColor_;
	sf::Color styleDeleteColor = deleteNodeColor_;
	sf::Color styleEdgeColor = edgeColor_;
	float styleEdgeThickness = edgeThickness_;
	float styleFontScale = fontScale_;

	if (visualStylePreset_ == 1) {
		styleNodeColor = sf::Color(245, 245, 245, 255);
		styleSecondaryColor = sf::Color(210, 210, 210, 255);
		styleEdgeThickness = std::max(1.5f, edgeThickness_ * 0.7f);
		styleFontScale = std::max(0.8f, fontScale_ * 0.95f);
	}
	else if (visualStylePreset_ == 2) {
		styleEdgeThickness = edgeThickness_ * 1.35f;
		styleFontScale = fontScale_ * 1.05f;
	}

	sf::RectangleShape background(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
	background.setFillColor(canvasBgColor_);
	window.draw(background);

	if (frame.values.empty()) {
		return;
	}

	const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
	const bool mouseInsideCanvas =
		mousePos.x >= 0 && mousePos.y >= 0 &&
		mousePos.x < static_cast<int>(size.x) &&
		mousePos.y < static_cast<int>(size.y);
	const bool canDragCanvas = mouseInsideCanvas && !ImGui::GetIO().WantCaptureMouse;

	if (canDragCanvas) {
		const float wheel = ImGui::GetIO().MouseWheel;
		if (std::abs(wheel) > 0.001f) {
			const float zoomStep = 1.0f + wheel * 0.12f;
			zoomScale_ = std::clamp(zoomScale_ * zoomStep, 0.55f, 2.2f);
		}
	}

	const float radius = std::clamp(nodeRadius_ * zoomScale_, 12.0f, 84.0f);
	const float diameter = radius * 2.0f;
	const float spacing = diameter + (50.0f * zoomScale_);
	const bool isDragPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);

	if (canDragCanvas && isDragPressed) {
		if (!isCanvasDragging_) {
			isCanvasDragging_ = true;
			lastDragMousePos_ = mousePos;
		}
		else {
			const int deltaX = mousePos.x - lastDragMousePos_.x;
			const int deltaY = mousePos.y - lastDragMousePos_.y;
			scrollOffset_ -= static_cast<float>(deltaX);
			canvasOffsetY_ += static_cast<float>(deltaY);
			lastDragMousePos_ = mousePos;
		}
	}
	else {
		isCanvasDragging_ = false;
	}

	scrollOffset_ = std::clamp(scrollOffset_, -10000.0f, 10000.0f);
	canvasOffsetY_ = std::clamp(canvasOffsetY_, -10000.0f, 10000.0f);

	const float startX = 40.0f - scrollOffset_;
	const float centerY = static_cast<float>(size.y) * 0.50f + canvasOffsetY_;

	auto drawArrow = [&](float fromX, float fromY, float toX, float toY, sf::Color color) {
		sf::Vertex line[2];
		line[0].position = sf::Vector2f(fromX, fromY);
		line[0].color = color;
		line[1].position = sf::Vector2f(toX, toY);
		line[1].color = color;
		window.draw(line, 2, sf::PrimitiveType::Lines);

		const float dx = toX - fromX;
		const float dy = toY - fromY;
		const float len = std::max(1.0f, std::sqrt(dx * dx + dy * dy));
		const float ux = dx / len;
		const float uy = dy / len;
		const float px = -uy;
		const float py = ux;

		sf::ConvexShape head(3);
		head.setPoint(0, sf::Vector2f(toX, toY));
		head.setPoint(1, sf::Vector2f(toX - ux * 12.0f + px * 5.0f, toY - uy * 12.0f + py * 5.0f));
		head.setPoint(2, sf::Vector2f(toX - ux * 12.0f - px * 5.0f, toY - uy * 12.0f - py * 5.0f));
		head.setFillColor(color);
		window.draw(head);
	};

	auto drawNode = [&](float cx, float cy, int value, int index, sf::Color fill, std::uint8_t alpha = 255) {
		sf::CircleShape node(radius);
		node.setOrigin(sf::Vector2f(radius, radius));
		node.setPosition(sf::Vector2f(cx, cy));
		node.setOutlineThickness(styleEdgeThickness);
		node.setOutlineColor(sf::Color(styleEdgeColor.r, styleEdgeColor.g, styleEdgeColor.b, alpha));
		node.setFillColor(sf::Color(fill.r, fill.g, fill.b, alpha));
		window.draw(node);

		if (font != nullptr) {
			sf::Text valueText(*font, "", static_cast<unsigned int>(20.0f * styleFontScale));
			valueText.setString(std::to_string(value));
			valueText.setFillColor(sf::Color(valueTextColor_.r, valueTextColor_.g, valueTextColor_.b, alpha));
			const sf::FloatRect vb = valueText.getLocalBounds();
			valueText.setPosition(sf::Vector2f(
				cx - (vb.position.x + vb.size.x * 0.5f),
				cy - (vb.position.y + vb.size.y * 0.5f)
			));
			window.draw(valueText);

			sf::Text indexText(*font, "", static_cast<unsigned int>(14.0f * styleFontScale));
			indexText.setString(std::to_string(index));
			indexText.setFillColor(sf::Color(indexTextColor_.r, indexTextColor_.g, indexTextColor_.b, alpha));
			const sf::FloatRect ib = indexText.getLocalBounds();
			indexText.setPosition(sf::Vector2f(
				cx - (ib.position.x + ib.size.x * 0.5f),
				cy - radius - 26.0f
			));
			window.draw(indexText);
		}
	};

	const bool addPrepareTransition = interpolation.isTransitioning &&
		interpolation.previousFrame.operationType == SLLOperationType::Add &&
		frame.operationType == SLLOperationType::Add &&
		interpolation.previousFrame.codeLine == 3 && frame.codeLine == 4;

	const bool addCreateTransition = interpolation.isTransitioning &&
		interpolation.previousFrame.operationType == SLLOperationType::Add &&
		frame.operationType == SLLOperationType::Add &&
		interpolation.previousFrame.codeLine == 2 && frame.codeLine == 3;

	const bool addCreateStatic = !interpolation.isTransitioning &&
		frame.operationType == SLLOperationType::Add &&
		frame.codeLine == 3;

	const bool addPrepareStatic = !interpolation.isTransitioning &&
		frame.operationType == SLLOperationType::Add &&
		frame.codeLine == 4;

	const bool addFinalizeTransition = interpolation.isTransitioning &&
		interpolation.previousFrame.operationType == SLLOperationType::Add &&
		frame.operationType == SLLOperationType::Add &&
		interpolation.previousFrame.codeLine == 4 && frame.codeLine == 5;

	const bool deleteTransition = interpolation.isTransitioning &&
		interpolation.previousFrame.operationType == SLLOperationType::Delete &&
		frame.operationType == SLLOperationType::Delete &&
		interpolation.previousFrame.codeLine == 4 && frame.codeLine == 5;

	const bool initializeTransition = interpolation.isTransitioning &&
		interpolation.previousFrame.operationType == SLLOperationType::Initialize &&
		frame.operationType == SLLOperationType::Initialize;

	const bool initializeStatic = !interpolation.isTransitioning &&
		frame.operationType == SLLOperationType::Initialize;

	if (initializeTransition || initializeStatic) {
		const float t = initializeTransition ? interpolation.transitionProgress : 1.0f;
		const float smoothT = t * t * (3.0f - 2.0f * t);
		const std::size_t nodeCount = frame.values.size();
		const float revealSpan = 0.70f;
		const float fadeWindow = 0.30f;
		auto staggerDelay = [&](std::size_t idx) {
			if (nodeCount <= 1) {
				return 0.0f;
			}
			return revealSpan * (static_cast<float>(idx) / static_cast<float>(nodeCount - 1));
		};

		for (std::size_t i = 0; i < frame.values.size(); ++i) {
			const float cx = startX + static_cast<float>(i) * spacing;
			if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
				continue;
			}

			const float nodeDelay = staggerDelay(i);
			const float appear = std::clamp((smoothT - nodeDelay) / fadeWindow, 0.0f, 1.0f);
			const float riseOffset = (1.0f - appear) * 26.0f;
			const std::uint8_t alpha = static_cast<std::uint8_t>(255.0f * appear);
			drawNode(cx, centerY + riseOffset, frame.values[i], static_cast<int>(i), styleNodeColor, alpha);
		}

		for (std::size_t i = 0; i + 1 < frame.values.size(); ++i) {
			const float x0 = startX + static_cast<float>(i) * spacing;
			const float x1 = startX + static_cast<float>(i + 1) * spacing;
			const float nodeDelay0 = staggerDelay(i);
			const float nodeDelay1 = staggerDelay(i + 1);
			const float appear0 = std::clamp((smoothT - nodeDelay0) / fadeWindow, 0.0f, 1.0f);
			const float appear1 = std::clamp((smoothT - nodeDelay1) / fadeWindow, 0.0f, 1.0f);
			const float edgeAppear = std::min(appear0, appear1);
			if (edgeAppear <= 0.05f) {
				continue;
			}
			drawArrow(x0 + radius, centerY, x1 - radius, centerY,
				sf::Color(styleEdgeColor.r, styleEdgeColor.g, styleEdgeColor.b, static_cast<std::uint8_t>(255.0f * edgeAppear)));
		}
	}
	else if (addCreateTransition || addCreateStatic) {
		const int rawInsertSlot =
			frame.secondaryIndex >= 0 ? frame.secondaryIndex :
			(frame.activeIndex >= 0 ? frame.activeIndex + 1 : static_cast<int>(frame.values.size()));
		const int insertSlot = std::clamp(rawInsertSlot, 0, static_cast<int>(frame.values.size()));
		const float t = addCreateTransition ? interpolation.transitionProgress : 1.0f;

		auto resolveInsertedValue = [&]() {
			if (singlyLinkedList.cursor + 1 < singlyLinkedList.timeline.size()) {
				const SLLFrame& next = singlyLinkedList.timeline[singlyLinkedList.cursor + 1];
				if (next.operationType == SLLOperationType::Add &&
					rawInsertSlot >= 0 && rawInsertSlot < static_cast<int>(next.values.size())) {
					return next.values[rawInsertSlot];
				}
			}
			if (interpolation.isTransitioning) {
				const SLLFrame& curr = interpolation.currentFrame;
				if (curr.operationType == SLLOperationType::Add &&
					rawInsertSlot >= 0 && rawInsertSlot < static_cast<int>(curr.values.size())) {
					return curr.values[rawInsertSlot];
				}
			}
			if (!frame.values.empty()) {
				return (insertSlot == 0) ? frame.values.front() : frame.values.back();
			}
			return 0;
		};
		const int insertedValue = resolveInsertedValue();

		for (std::size_t i = 0; i < frame.values.size(); ++i) {
			const float cx = startX + static_cast<float>(i) * spacing;
			if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
				continue;
			}

			sf::Color fill = styleNodeColor;
			if (static_cast<int>(i) == interpolation.previousFrame.activeIndex) {
				fill = styleActiveColor;
			}
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);
		}

		for (std::size_t i = 0; i + 1 < frame.values.size(); ++i) {
			const float x0 = startX + static_cast<float>(i) * spacing;
			const float x1 = startX + static_cast<float>(i + 1) * spacing;
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, styleEdgeColor);
		}

		const float insertX = startX + static_cast<float>(insertSlot) * spacing;
		const float insertY = centerY - 140.0f;
		drawNode(insertX, insertY, insertedValue, insertSlot, styleActiveColor, static_cast<std::uint8_t>(255.0f * t));
	}
	else if (addPrepareTransition || addPrepareStatic) {
		const int insertedIndex = std::clamp(
			frame.secondaryIndex >= 0 ? frame.secondaryIndex : (frame.activeIndex >= 0 ? frame.activeIndex + 1 : 0),
			0,
			static_cast<int>(frame.values.size()) - 1);
		const bool insertAtEnd = insertedIndex == static_cast<int>(frame.values.size()) - 1;
		const float t = addPrepareTransition ? interpolation.transitionProgress : 1.0f;
		const float gapT = t;
		const float connectT = std::clamp((t - 0.15f) / 0.85f, 0.0f, 1.0f);

		if (insertAtEnd) {
			for (int i = 0; i < insertedIndex; ++i) {
				const float cx = startX + static_cast<float>(i) * spacing;
				if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
					continue;
				}
				sf::Color fill = styleNodeColor;
				if (i == frame.activeIndex) {
					fill = styleActiveColor;
				}
				drawNode(cx, centerY, frame.values[i], i, fill);
			}

			for (int i = 0; i + 1 < insertedIndex; ++i) {
				const float x0 = startX + static_cast<float>(i) * spacing;
				const float x1 = startX + static_cast<float>(i + 1) * spacing;
				drawArrow(x0 + radius, centerY, x1 - radius, centerY, styleEdgeColor);
			}

			const float insertX = startX + static_cast<float>(insertedIndex) * spacing;
			const float insertY = centerY - 120.0f;
			drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, styleActiveColor);

			if (insertedIndex > 0) {
				const float prevX = startX + static_cast<float>(insertedIndex - 1) * spacing;
				drawArrow(prevX + radius, centerY, insertX - radius, centerY,
					sf::Color(styleEdgeColor.r, styleEdgeColor.g, styleEdgeColor.b, static_cast<std::uint8_t>(255.0f * connectT)));
			}
		}
		else {

			auto addX = [&](int i) {
			if (i < insertedIndex) {
				return startX + static_cast<float>(i) * spacing;
			}
			if (i == insertedIndex) {
				return startX + static_cast<float>(i) * spacing;
			}
			if (insertAtEnd) {
				return startX + static_cast<float>(i - 1) * spacing;
			}
			return startX + (static_cast<float>(i) - 1.0f + gapT) * spacing;
			};

		for (std::size_t i = 0; i < frame.values.size(); ++i) {
			if (static_cast<int>(i) == insertedIndex) {
				continue;
			}
			const float cx = addX(static_cast<int>(i));
			if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
				continue;
			}

			sf::Color fill = styleNodeColor;
			if (static_cast<int>(i) == frame.activeIndex) {
				fill = styleActiveColor;
			}
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);
		}

		for (std::size_t i = 0; i + 1 < frame.values.size(); ++i) {
			if (static_cast<int>(i) == insertedIndex || static_cast<int>(i + 1) == insertedIndex) {
				continue;
			}
			const float x0 = addX(static_cast<int>(i));
			const float x1 = addX(static_cast<int>(i + 1));
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, styleEdgeColor);
		}

			const float insertX = addX(insertedIndex);
			const float insertY = centerY - 135.0f;
			drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, styleActiveColor);

			if (insertedIndex > 0) {
				const float prevX = addX(insertedIndex - 1);
				drawArrow(prevX + radius, centerY, insertX - radius * 0.6f, insertY,
					sf::Color(styleEdgeColor.r, styleEdgeColor.g, styleEdgeColor.b, static_cast<std::uint8_t>(255.0f * connectT)));
			}
		}
	}
	else if (addFinalizeTransition) {
		const int insertedIndex = std::clamp(
			interpolation.previousFrame.secondaryIndex >= 0 ? interpolation.previousFrame.secondaryIndex : frame.activeIndex,
			0,
			static_cast<int>(frame.values.size()) - 1);
		const bool insertAtEnd = insertedIndex == static_cast<int>(frame.values.size()) - 1;
		const float t = interpolation.transitionProgress;
		const float smoothT = t * t * (3.0f - 2.0f * t);

		if (insertAtEnd) {
			for (int i = 0; i < insertedIndex; ++i) {
				const float cx = startX + static_cast<float>(i) * spacing;
				if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
					continue;
				}
				drawNode(cx, centerY, frame.values[i], i, styleNodeColor);
			}

			for (int i = 0; i + 1 < insertedIndex; ++i) {
				const float x0 = startX + static_cast<float>(i) * spacing;
				const float x1 = startX + static_cast<float>(i + 1) * spacing;
				drawArrow(x0 + radius, centerY, x1 - radius, centerY, styleEdgeColor);
			}

			const float insertX = startX + static_cast<float>(insertedIndex) * spacing;
			const float startY = centerY - 120.0f;
			const float insertY = startY + (centerY - startY) * smoothT;
			drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, styleActiveColor);

			if (insertedIndex > 0) {
				const float prevX = startX + static_cast<float>(insertedIndex - 1) * spacing;
				drawArrow(prevX + radius, centerY, insertX - radius, centerY, styleEdgeColor);
			}
		}
		else {

		for (std::size_t i = 0; i < frame.values.size(); ++i) {
			if (static_cast<int>(i) == insertedIndex) {
				continue;
			}
			const float cx = startX + static_cast<float>(i) * spacing;
			if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
				continue;
			}

			sf::Color fill = styleNodeColor;
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);
		}

		for (std::size_t i = 0; i + 1 < frame.values.size(); ++i) {
			if (static_cast<int>(i) == insertedIndex || static_cast<int>(i + 1) == insertedIndex) {
				continue;
			}
			const float x0 = startX + static_cast<float>(i) * spacing;
			const float x1 = startX + static_cast<float>(i + 1) * spacing;
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, styleEdgeColor);
		}

		const float insertX = startX + static_cast<float>(insertedIndex) * spacing;
		const float startY = centerY - (insertAtEnd ? 120.0f : 135.0f);
		const float insertY = startY + (centerY - startY) * smoothT;
		drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, styleActiveColor);

		if (insertedIndex > 0) {
			const float prevX = startX + static_cast<float>(insertedIndex - 1) * spacing;
			if (insertAtEnd) {
				drawArrow(prevX + radius, centerY, insertX - radius, centerY, styleEdgeColor);
			}
			else {
				drawArrow(prevX + radius, centerY, insertX - radius * 0.6f, insertY, styleEdgeColor);
			}
		}
		if (insertedIndex + 1 < static_cast<int>(frame.values.size())) {
			const float nextX = startX + static_cast<float>(insertedIndex + 1) * spacing;
			drawArrow(insertX + radius * 0.6f, insertY, nextX - radius, centerY,
				sf::Color(styleEdgeColor.r, styleEdgeColor.g, styleEdgeColor.b, static_cast<std::uint8_t>(255.0f * t)));
		}
		}
	}
	else if (deleteTransition) {
		const int deletedIndex = std::clamp(
			interpolation.previousFrame.activeIndex >= 0 ? interpolation.previousFrame.activeIndex : interpolation.previousFrame.secondaryIndex,
			0,
			static_cast<int>(interpolation.previousFrame.values.size()) - 1);
		const bool deleteAtEnd = deletedIndex == static_cast<int>(interpolation.previousFrame.values.size()) - 1;
		const float t = interpolation.transitionProgress;
		const float fadeT = std::clamp(t / (deleteAtEnd ? 0.55f : 0.35f), 0.0f, 1.0f);
		const float reconnectT = std::clamp((t - (deleteAtEnd ? 0.45f : 0.35f)) / 0.30f, 0.0f, 1.0f);
		const float shiftT = deleteAtEnd ? 0.0f : std::clamp((t - 0.65f) / 0.35f, 0.0f, 1.0f);

		for (std::size_t i = 0; i < interpolation.previousFrame.values.size(); ++i) {
			float cx = startX + static_cast<float>(i) * spacing;
			float cy = centerY;
			std::uint8_t alpha = 255;
			if (static_cast<int>(i) == deletedIndex) {
				alpha = static_cast<std::uint8_t>(255.0f * (1.0f - fadeT));
				cy = centerY + 30.0f * fadeT;
			}
			else if (static_cast<int>(i) > deletedIndex) {
				cx -= spacing * shiftT;
			}

			if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
				continue;
			}

			sf::Color fill = styleNodeColor;
			if (static_cast<int>(i) == deletedIndex) {
				fill = styleDeleteColor;
			}
			drawNode(cx, cy, interpolation.previousFrame.values[i], static_cast<int>(i), fill, alpha);
		}

		for (std::size_t i = 0; i + 1 < interpolation.previousFrame.values.size(); ++i) {
			if (static_cast<int>(i) == deletedIndex || static_cast<int>(i + 1) == deletedIndex) {
				continue;
			}
			float x0 = startX + static_cast<float>(i) * spacing;
			float x1 = startX + static_cast<float>(i + 1) * spacing;
			if (static_cast<int>(i) > deletedIndex) {
				x0 -= spacing * shiftT;
			}
			if (static_cast<int>(i + 1) > deletedIndex) {
				x1 -= spacing * shiftT;
			}
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, styleEdgeColor);
		}

		if (deleteAtEnd && deletedIndex > 0) {
			const float prevX = startX + static_cast<float>(deletedIndex - 1) * spacing;
			const float oldTailX = startX + static_cast<float>(deletedIndex) * spacing;
			const float tailEndX = (oldTailX - radius) + ((prevX + radius) - (oldTailX - radius)) * reconnectT;
			drawArrow(prevX + radius, centerY, tailEndX, centerY,
				sf::Color(highlightRingColor_.r, highlightRingColor_.g, highlightRingColor_.b, static_cast<std::uint8_t>(255.0f * (1.0f - std::clamp(t - 0.15f, 0.0f, 1.0f)))));
		}

		if (deletedIndex > 0 && deletedIndex + 1 < static_cast<int>(interpolation.previousFrame.values.size())) {
			const float leftX = startX + static_cast<float>(deletedIndex - 1) * spacing;
			const float rightBaseX = startX + static_cast<float>(deletedIndex + 1) * spacing;
			const float rightX = rightBaseX - spacing * shiftT;
			drawArrow(leftX + radius, centerY, rightX - radius, centerY,
				sf::Color(highlightRingColor_.r, highlightRingColor_.g, highlightRingColor_.b, static_cast<std::uint8_t>(255.0f * reconnectT)));
		}
	}
	else {
		for (std::size_t i = 0; i < frame.values.size(); ++i) {
			const float cx = startX + static_cast<float>(i) * spacing;
			if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
				continue;
			}

			sf::Color fill = styleNodeColor;
			if (static_cast<int>(i) == frame.secondaryIndex) {
				fill = styleSecondaryColor;
			}
			if (static_cast<int>(i) == frame.activeIndex) {
				fill = styleActiveColor;
			}
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);

			if (i + 1 < frame.values.size()) {
				const float nx = startX + static_cast<float>(i + 1) * spacing;
				drawArrow(cx + radius, centerY, nx - radius, centerY, styleEdgeColor);
			}
		}
	}

	if (frame.activeIndex >= 0) {
		float fromX = startX + static_cast<float>(frame.activeIndex) * spacing;
		if (interpolation.isTransitioning && interpolation.previousFrame.activeIndex >= 0) {
			fromX = startX + static_cast<float>(interpolation.previousFrame.activeIndex) * spacing;
		}
		const float toX = startX + static_cast<float>(frame.activeIndex) * spacing;
		const float markerX = interpolation.isTransitioning ? lerp(fromX, toX, interpolation.transitionProgress) : toX;

		sf::CircleShape ring(radius + 8.0f);
		ring.setOrigin(sf::Vector2f(radius + 8.0f, radius + 8.0f));
		ring.setPosition(sf::Vector2f(markerX, centerY));
		ring.setFillColor(sf::Color::Transparent);
		ring.setOutlineThickness(3.0f);
		ring.setOutlineColor(highlightRingColor_);
		window.draw(ring);
	}

}

///-----------------------------------
/// TIMELINE MANAGEMENT (Moved from logic)
///-----------------------------------

namespace {
	constexpr float kBaseStepIntervalSeconds = 0.55f;

	//Choose transition duration between two frames for smoother visual pacing.
	float pickTransitionDuration(const SLLFrame& from, const SLLFrame& to)
	{
		if (from.operationType == SLLOperationType::Add && to.operationType == SLLOperationType::Add) {
			if (from.codeLine == 2 && to.codeLine == 3) {
				return 0.90f; // Create new node above list
			}
			if (from.codeLine == 3 && to.codeLine == 4) {
				return 1.05f; // Open gap and connect prev -> new
			}
			if (from.codeLine == 4 && to.codeLine == 5) {
				return 0.95f; // Connect new -> next and settle
			}
		}

		if (from.operationType == SLLOperationType::Initialize && to.operationType == SLLOperationType::Initialize) {
			return 1.0f;
		}

		if (from.operationType == SLLOperationType::Delete && to.operationType == SLLOperationType::Delete &&
			from.codeLine == 4 && to.codeLine == 5) {
			return 0.95f;
		}

		return 0.45f;
	}
}

//Move timeline cursor forward by one step and start interpolation.
void SinglyLinkedList::stepForward()
{
	if (timeline.empty()) {
		return;
	}
	if (cursor + 1 < timeline.size()) {
		interpolation.previousFrame = currentFrame();
		++cursor;
		interpolation.currentFrame = timeline[cursor];
		interpolation.transitionDuration = pickTransitionDuration(interpolation.previousFrame, interpolation.currentFrame);
		interpolation.transitionProgress = 0.0f;
		interpolation.isTransitioning = true;
	}
}

//Move timeline cursor backward by one step and start interpolation.
void SinglyLinkedList::stepBackward()
{
	if (timeline.empty()) {
		return;
	}
	if (cursor > 0) {
		interpolation.previousFrame = currentFrame();
		--cursor;
		interpolation.currentFrame = timeline[cursor];
		interpolation.transitionDuration = pickTransitionDuration(interpolation.previousFrame, interpolation.currentFrame);
		interpolation.transitionProgress = 0.0f;
		interpolation.isTransitioning = true;
	}
}

//Jump directly to the final timeline frame.
void SinglyLinkedList::jumpToFinal()
{
	if (!timeline.empty()) {
		cursor = timeline.size() - 1;
		interpolation.isTransitioning = false;
		interpolation.transitionProgress = 0.0f;
	}
}

//Jump directly to the first timeline frame and reset playback state.
void SinglyLinkedList::jumpToStart()
{
	cursor = 0;
	autoplayAccumulator = 0.0f;
	interpolation.isTransitioning = false;
	interpolation.transitionProgress = 0.0f;
}

//Advance timeline automatically based on elapsed time and playback speed.
void SinglyLinkedList::updateAutoplay(float deltaSeconds, float speedMultiplier, bool enabled)
{
	if (!enabled || timeline.empty() || cursor + 1 >= timeline.size()) {
		autoplayAccumulator = 0.0f;
		return;
	}
	if (interpolation.isTransitioning) {
		return;
	}

	const float safeSpeed = std::max(0.1f, speedMultiplier);
	const float stepInterval = kBaseStepIntervalSeconds / safeSpeed;
	autoplayAccumulator += deltaSeconds;

	if (autoplayAccumulator >= stepInterval && cursor + 1 < timeline.size()) {
		autoplayAccumulator -= stepInterval;
		stepForward();
	}
}

//Return the currently selected timeline frame with bounds safety.
const SLLFrame& SinglyLinkedList::currentFrame() const
{
	static const SLLFrame kEmptyFrame{};
	if (timeline.empty()) {
		return kEmptyFrame;
	}
	if (cursor >= timeline.size()) {
		return timeline.back();
	}
	return timeline[cursor];
}

//Update interpolation progress between previous and current timeline frame.
void SinglyLinkedList::updateInterpolation(float deltaSeconds)
{
	if (!interpolation.isTransitioning) {
		return;
	}

	interpolation.transitionProgress += deltaSeconds / interpolation.transitionDuration;
	if (interpolation.transitionProgress >= 1.0f) {
		interpolation.transitionProgress = 1.0f;
		interpolation.isTransitioning = false;
	}
}

//Return interpolated frame while animating, otherwise current frame.
const SLLFrame& SinglyLinkedList::getInterpolatedFrame() const
{
	if (!interpolation.isTransitioning) {
		return currentFrame();
	}
	return interpolation.currentFrame;
}

//Check whether timeline contains any frames.
bool SinglyLinkedList::hasTimeline() const
{
	return !timeline.empty();
}

//Rebuild a single-frame idle timeline for non-animated state/messages.
void SinglyLinkedList::rebuildIdleTimeline(const std::string& message, int codeLine)
{
	timeline.clear();
	pushFrame(-1, -1, codeLine, message, SLLOperationType::Initialize);
	commitTimeline(message);
	if (!timeline.empty()) {
		interpolation.previousFrame = timeline.front();
		interpolation.currentFrame = timeline.front();
	}
	interpolation.isTransitioning = false;
	interpolation.transitionProgress = 0.0f;
}

//Capture current list snapshot as a timeline frame with markers and message.
void SinglyLinkedList::pushFrame(int activeIndex, int secondaryIndex, int codeLine, const std::string& message, SLLOperationType opType)
{
	SLLFrame frame;
	frame.values = values;
	frame.activeIndex = activeIndex;
	frame.secondaryIndex = secondaryIndex;
	frame.codeLine = codeLine;
	frame.message = message;
	frame.operationType = opType;
	timeline.push_back(std::move(frame));
}

//Finalize timeline after frame generation and sync playback metadata.
void SinglyLinkedList::commitTimeline(const std::string& fallbackMessage)
{
	if (timeline.empty()) {
		pushFrame(-1, -1, -1, fallbackMessage, SLLOperationType::Initialize);
	}
	cursor = 0;
	autoplayAccumulator = 0.0f;
	lastMessage = timeline.back().message.empty() ? fallbackMessage : timeline.back().message;
}

///-----------------------------------
/// VISUALIZATION WRAPPERS
/// These build timestep-by-step animations before executing operations
///-----------------------------------

//Build animated add-operation timeline and then apply data mutation.
void SinglyLinkedList::addAtViz(std::size_t index, int value)
{
	if (index > values.size()) {
		lastMessage = "Add failed: index out of range";
		rebuildIdleTimeline(lastMessage, 2);
		return;
	}

	timeline.clear();
	pushFrame(-1, static_cast<int>(index), 1, "Start add operation", SLLOperationType::Add);

	for (std::size_t i = 0; i < index; ++i) {
		pushFrame(static_cast<int>(i), -1, 2, "Traverse to insertion position", SLLOperationType::Add);
	}
	pushFrame(index == 0 ? -1 : static_cast<int>(index - 1), static_cast<int>(index), 3,
		"Create new node with value", SLLOperationType::Add);

	addAt(index, value);

	if (index == 0) {
		pushFrame(0, 0, 4,
			"Set head to new node", SLLOperationType::Add);
	}
	else {
		pushFrame(static_cast<int>(index - 1), static_cast<int>(index), 4,
			"Set prev->next to new node", SLLOperationType::Add);
	}

	pushFrame(static_cast<int>(index),
		(index + 1 < values.size()) ? static_cast<int>(index + 1) : -1,
		5,
		"Set new node next pointer", SLLOperationType::Add);
	commitTimeline("Add complete");
}

//Build animated delete-operation timeline and then apply data mutation.
void SinglyLinkedList::deleteAtViz(std::size_t index)
{
	if (values.empty()) {
		lastMessage = "Delete failed: list is empty";
		rebuildIdleTimeline(lastMessage, 1);
		return;
	}
	if (index >= values.size()) {
		lastMessage = "Delete failed: index out of range";
		rebuildIdleTimeline(lastMessage, 2);
		return;
	}

	timeline.clear();
	pushFrame(-1, -1, 1, "Start delete operation", SLLOperationType::Delete);

	for (std::size_t i = 0; i <= index; ++i) {
		pushFrame(static_cast<int>(i), -1, 2, "Traverse to target node", SLLOperationType::Delete);
	}
	pushFrame(index == 0 ? -1 : static_cast<int>(index - 1), static_cast<int>(index), 3,
		"Target node selected", SLLOperationType::Delete);

	int nextIndexBeforeErase = (index + 1 < values.size()) ? static_cast<int>(index + 1) : -1;
	pushFrame(static_cast<int>(index), nextIndexBeforeErase, 4,
		"Remove target node", SLLOperationType::Delete);

	deleteAt(index);
	
	int prevIndexAfterErase = (index == 0) ? -1 : static_cast<int>(index - 1);
	int nextIndexAfterErase = (index < values.size()) ? static_cast<int>(index) : -1;
	if (index == 0) {
		pushFrame(nextIndexAfterErase, -1, 5, "Move head to next node", SLLOperationType::Delete);
	}
	else {
		pushFrame(prevIndexAfterErase, nextIndexAfterErase, 5,
			"Set prev->next to node after deleted", SLLOperationType::Delete);
	}
	commitTimeline("Delete complete");
}

//Build animated update-operation timeline and then apply data mutation.
void SinglyLinkedList::updateAtViz(std::size_t index, int value)
{
	if (index >= values.size()) {
		lastMessage = "Update failed: index out of range";
		rebuildIdleTimeline(lastMessage, 2);
		return;
	}

	timeline.clear();
	pushFrame(-1, -1, 1, "Start update operation", SLLOperationType::Update);

	for (std::size_t i = 0; i <= index; ++i) {
		pushFrame(static_cast<int>(i), -1, 2, "Traverse to target node", SLLOperationType::Update);
	}

	updateAt(index, value);
	pushFrame(static_cast<int>(index), -1, 3, "Updated node value", SLLOperationType::Update);
	commitTimeline("Update complete");
}

//Build animated search-operation timeline and stop on found/not-found result.
void SinglyLinkedList::searchValueViz(int value)
{
	timeline.clear();
	pushFrame(-1, -1, 1, "Start search operation", SLLOperationType::Search);

	for (std::size_t i = 0; i < values.size(); ++i) {
		if (values[i] == value) {
			pushFrame(static_cast<int>(i), -1, 3, "Value found", SLLOperationType::Search);
			commitTimeline("Search complete: found");
			return;
		}
		pushFrame(static_cast<int>(i), -1, 2, "Compare current node", SLLOperationType::Search);
	}

	pushFrame(-1, -1, 4, "Value not found", SLLOperationType::Search);
	commitTimeline("Search complete: not found");
}
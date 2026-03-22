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
			ImGui::End();
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
		ImGui::End();
	}

	float computeMaxScroll(const SLLFrame& frame, float viewportWidth, float nodeRadius)
	{
		const float radius = std::clamp(nodeRadius, 18.0f, 42.0f);
		const float diameter = radius * 2.0f;
		const float spacing = diameter + 50.0f;
		const float totalWidth = static_cast<float>(frame.values.size()) * spacing + 60.0f;
		return std::max(0.0f, totalWidth - viewportWidth);
	}

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

SinglyLinkedListUI::SinglyLinkedListUI() {
	// This is where you can initialize any resources or variables needed
}

void SinglyLinkedListUI::draw() {
	ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(450.0f, 860.0f), ImGuiCond_Once);

	if (!ImGui::Begin("Singly Linked List Visualizer")) {
		ImGui::End();
		return;
	}

	singlyLinkedList.updateInterpolation(ImGui::GetIO().DeltaTime);
	singlyLinkedList.updateAutoplay(ImGui::GetIO().DeltaTime, playbackSpeed_, autoplay_ && playbackMode_ == PlaybackMode::RunAtOnce);

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
	ImGui::Checkbox("Show code overlay##sll", &showCodeOverlay_);
	ImGui::SeparatorText("Style");
	ImGui::SliderFloat("Node radius##sll", &nodeRadius_, 12.0f, 60.0f, "%.1f");
	ImGui::SliderFloat("Edge thickness##sll", &edgeThickness_, 1.0f, 8.0f, "%.1f");
	ImGui::SliderFloat("Font scale##sll", &fontScale_, 0.75f, 2.0f, "%.2f");

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

	ImGui::Text("Frame: %zu / %zu | Status: %s",
		singlyLinkedList.timeline.empty() ? 0 : (singlyLinkedList.cursor + 1),
		singlyLinkedList.timeline.size(),
		displayFrame.message.empty() ? singlyLinkedList.lastMessage.c_str() : displayFrame.message.c_str());

	float maxScroll = computeMaxScroll(displayFrame, ImGui::GetMainViewport()->Size.x - 80.0f, nodeRadius_);
	if (maxScroll > 0.0f) {
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderFloat("Horizontal Scroll##sll", &scrollOffset_, 0.0f, maxScroll, "%.0f");
		scrollOffset_ = std::clamp(scrollOffset_, 0.0f, maxScroll);
	}
	else {
		scrollOffset_ = 0.0f;
	}
	if (showCodeOverlay_) {
		drawCodeOverlay(displayedCodeLine, displayedOpType);
	}

	if (ImGui::Button("Back to menu")) {
		autoplay_ = false;
		uiConfig.state = UIState::Menu;
	}

	ImGui::End();
}

void SinglyLinkedListUI::drawSfml(sf::RenderWindow& window)
{
	const SLLFrame frame = singlyLinkedList.getInterpolatedFrame();
	const SLLInterpolationState& interpolation = singlyLinkedList.interpolation;
	const sf::Vector2u size = window.getSize();
	const sf::Font* font = getSllFont();

	sf::RectangleShape background(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
	background.setFillColor(sf::Color(16, 21, 30));
	window.draw(background);

	if (frame.values.empty()) {
		return;
	}

	const float radius = std::clamp(nodeRadius_, 18.0f, 42.0f);
	const float diameter = radius * 2.0f;
	const float spacing = diameter + 50.0f;
	const float viewportWidth = static_cast<float>(size.x) - 80.0f;
	const float maxScroll = computeMaxScroll(frame, viewportWidth, nodeRadius_);

	const float minScroll = 0.0f;
	const int activeIdx = frame.activeIndex;
	if (interpolation.isTransitioning && interpolation.previousFrame.activeIndex >= 0 && activeIdx >= 0) {
		const float prevX = 30.0f + static_cast<float>(interpolation.previousFrame.activeIndex) * spacing;
		const float currX = 30.0f + static_cast<float>(activeIdx) * spacing;
		const float smoothX = lerp(prevX, currX, interpolation.transitionProgress);
		const float target = std::clamp(smoothX - viewportWidth * 0.5f, minScroll, maxScroll);
		scrollOffset_ += (target - scrollOffset_) * 0.15f;
	}
	else if (activeIdx >= 0) {
		const float nodeX = 30.0f + static_cast<float>(activeIdx) * spacing;
		const float target = std::clamp(nodeX - viewportWidth * 0.5f, minScroll, maxScroll);
		scrollOffset_ += (target - scrollOffset_) * 0.15f;
	}

	scrollOffset_ = std::clamp(scrollOffset_, minScroll, maxScroll);

	const float startX = 40.0f - scrollOffset_;
	const float centerY = static_cast<float>(size.y) * 0.56f;

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
		node.setOutlineThickness(edgeThickness_);
		node.setOutlineColor(sf::Color(15, 23, 42, alpha));
		node.setFillColor(sf::Color(fill.r, fill.g, fill.b, alpha));
		window.draw(node);

		if (font != nullptr) {
			sf::Text valueText(*font, "", static_cast<unsigned int>(20.0f * fontScale_));
			valueText.setString(std::to_string(value));
			valueText.setFillColor(sf::Color(255, 255, 255, alpha));
			const sf::FloatRect vb = valueText.getLocalBounds();
			valueText.setPosition(sf::Vector2f(
				cx - (vb.position.x + vb.size.x * 0.5f),
				cy - (vb.position.y + vb.size.y * 0.5f)
			));
			window.draw(valueText);

			sf::Text indexText(*font, "", static_cast<unsigned int>(14.0f * fontScale_));
			indexText.setString(std::to_string(index));
			indexText.setFillColor(sf::Color(144, 202, 249, alpha));
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

	if (addCreateTransition || addCreateStatic) {
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

			sf::Color fill(72, 149, 239);
			if (static_cast<int>(i) == interpolation.previousFrame.activeIndex) {
				fill = sf::Color(245, 158, 11);
			}
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);
		}

		for (std::size_t i = 0; i + 1 < frame.values.size(); ++i) {
			const float x0 = startX + static_cast<float>(i) * spacing;
			const float x1 = startX + static_cast<float>(i + 1) * spacing;
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, sf::Color(148, 163, 184));
		}

		const float insertX = startX + static_cast<float>(insertSlot) * spacing;
		const float insertY = centerY - 140.0f;
		drawNode(insertX, insertY, insertedValue, insertSlot, sf::Color(245, 158, 11), static_cast<std::uint8_t>(255.0f * t));
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
				sf::Color fill(72, 149, 239);
				if (i == frame.activeIndex) {
					fill = sf::Color(245, 158, 11);
				}
				drawNode(cx, centerY, frame.values[i], i, fill);
			}

			for (int i = 0; i + 1 < insertedIndex; ++i) {
				const float x0 = startX + static_cast<float>(i) * spacing;
				const float x1 = startX + static_cast<float>(i + 1) * spacing;
				drawArrow(x0 + radius, centerY, x1 - radius, centerY, sf::Color(148, 163, 184));
			}

			const float insertX = startX + static_cast<float>(insertedIndex) * spacing;
			const float insertY = centerY - 120.0f;
			drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, sf::Color(245, 158, 11));

			if (insertedIndex > 0) {
				const float prevX = startX + static_cast<float>(insertedIndex - 1) * spacing;
				drawArrow(prevX + radius, centerY, insertX - radius, centerY,
					sf::Color(148, 163, 184, static_cast<std::uint8_t>(255.0f * connectT)));
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

			sf::Color fill(72, 149, 239);
			if (static_cast<int>(i) == frame.activeIndex) {
				fill = sf::Color(245, 158, 11);
			}
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);
		}

		for (std::size_t i = 0; i + 1 < frame.values.size(); ++i) {
			if (static_cast<int>(i) == insertedIndex || static_cast<int>(i + 1) == insertedIndex) {
				continue;
			}
			const float x0 = addX(static_cast<int>(i));
			const float x1 = addX(static_cast<int>(i + 1));
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, sf::Color(148, 163, 184));
		}

			const float insertX = addX(insertedIndex);
			const float insertY = centerY - 135.0f;
			drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, sf::Color(245, 158, 11));

			if (insertedIndex > 0) {
				const float prevX = addX(insertedIndex - 1);
				drawArrow(prevX + radius, centerY, insertX - radius * 0.6f, insertY,
					sf::Color(148, 163, 184, static_cast<std::uint8_t>(255.0f * connectT)));
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
				drawNode(cx, centerY, frame.values[i], i, sf::Color(72, 149, 239));
			}

			for (int i = 0; i + 1 < insertedIndex; ++i) {
				const float x0 = startX + static_cast<float>(i) * spacing;
				const float x1 = startX + static_cast<float>(i + 1) * spacing;
				drawArrow(x0 + radius, centerY, x1 - radius, centerY, sf::Color(148, 163, 184));
			}

			const float insertX = startX + static_cast<float>(insertedIndex) * spacing;
			const float startY = centerY - 120.0f;
			const float insertY = startY + (centerY - startY) * smoothT;
			drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, sf::Color(245, 158, 11));

			if (insertedIndex > 0) {
				const float prevX = startX + static_cast<float>(insertedIndex - 1) * spacing;
				drawArrow(prevX + radius, centerY, insertX - radius, centerY, sf::Color(148, 163, 184));
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

			sf::Color fill(72, 149, 239);
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);
		}

		for (std::size_t i = 0; i + 1 < frame.values.size(); ++i) {
			if (static_cast<int>(i) == insertedIndex || static_cast<int>(i + 1) == insertedIndex) {
				continue;
			}
			const float x0 = startX + static_cast<float>(i) * spacing;
			const float x1 = startX + static_cast<float>(i + 1) * spacing;
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, sf::Color(148, 163, 184));
		}

		const float insertX = startX + static_cast<float>(insertedIndex) * spacing;
		const float startY = centerY - (insertAtEnd ? 120.0f : 135.0f);
		const float insertY = startY + (centerY - startY) * smoothT;
		drawNode(insertX, insertY, frame.values[insertedIndex], insertedIndex, sf::Color(245, 158, 11));

		if (insertedIndex > 0) {
			const float prevX = startX + static_cast<float>(insertedIndex - 1) * spacing;
			if (insertAtEnd) {
				drawArrow(prevX + radius, centerY, insertX - radius, centerY, sf::Color(148, 163, 184));
			}
			else {
				drawArrow(prevX + radius, centerY, insertX - radius * 0.6f, insertY, sf::Color(148, 163, 184));
			}
		}
		if (insertedIndex + 1 < static_cast<int>(frame.values.size())) {
			const float nextX = startX + static_cast<float>(insertedIndex + 1) * spacing;
			drawArrow(insertX + radius * 0.6f, insertY, nextX - radius, centerY,
				sf::Color(148, 163, 184, static_cast<std::uint8_t>(255.0f * t)));
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

			sf::Color fill(72, 149, 239);
			if (static_cast<int>(i) == deletedIndex) {
				fill = sf::Color(239, 68, 68);
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
			drawArrow(x0 + radius, centerY, x1 - radius, centerY, sf::Color(148, 163, 184));
		}

		if (deleteAtEnd && deletedIndex > 0) {
			const float prevX = startX + static_cast<float>(deletedIndex - 1) * spacing;
			const float oldTailX = startX + static_cast<float>(deletedIndex) * spacing;
			const float tailEndX = (oldTailX - radius) + ((prevX + radius) - (oldTailX - radius)) * reconnectT;
			drawArrow(prevX + radius, centerY, tailEndX, centerY,
				sf::Color(250, 204, 21, static_cast<std::uint8_t>(255.0f * (1.0f - std::clamp(t - 0.15f, 0.0f, 1.0f)))));
		}

		if (deletedIndex > 0 && deletedIndex + 1 < static_cast<int>(interpolation.previousFrame.values.size())) {
			const float leftX = startX + static_cast<float>(deletedIndex - 1) * spacing;
			const float rightBaseX = startX + static_cast<float>(deletedIndex + 1) * spacing;
			const float rightX = rightBaseX - spacing * shiftT;
			drawArrow(leftX + radius, centerY, rightX - radius, centerY,
				sf::Color(250, 204, 21, static_cast<std::uint8_t>(255.0f * reconnectT)));
		}
	}
	else {
		for (std::size_t i = 0; i < frame.values.size(); ++i) {
			const float cx = startX + static_cast<float>(i) * spacing;
			if (cx + radius < 0.0f || cx - radius > static_cast<float>(size.x)) {
				continue;
			}

			sf::Color fill(72, 149, 239);
			if (static_cast<int>(i) == frame.secondaryIndex) {
				fill = sf::Color(156, 163, 175);
			}
			if (static_cast<int>(i) == frame.activeIndex) {
				fill = sf::Color(245, 158, 11);
			}
			drawNode(cx, centerY, frame.values[i], static_cast<int>(i), fill);

			if (i + 1 < frame.values.size()) {
				const float nx = startX + static_cast<float>(i + 1) * spacing;
				drawArrow(cx + radius, centerY, nx - radius, centerY, sf::Color(148, 163, 184));
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
		ring.setOutlineColor(sf::Color(255, 214, 102, 230));
		window.draw(ring);
	}
}
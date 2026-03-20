#include <logic/singlylinkedlist.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <random>
#include <regex>
#include <sstream>

namespace {
	constexpr float kBaseStepIntervalSeconds = 0.55f;
}

void SinglyLinkedList::initializeEmpty()
{
	values.clear();
	rebuildIdleTimeline("Initialized empty list", 1);
}

void SinglyLinkedList::initializeRandom(int count, int minValue, int maxValue)
{
	if (count < 0) {
		count = 0;
	}
	if (minValue > maxValue) {
		std::swap(minValue, maxValue);
	}

	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(minValue, maxValue);

	values.clear();
	values.reserve(static_cast<std::size_t>(count));
	for (int i = 0; i < count; ++i) {
		values.push_back(dist(rng));
	}

	std::ostringstream oss;
	oss << "Initialized random list with " << count << " nodes";
	rebuildIdleTimeline(oss.str(), 1);
}

bool SinglyLinkedList::initializeFromTextFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		lastMessage = "Could not open text file";
		rebuildIdleTimeline(lastMessage);
		return false;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	values = parseIntegers(buffer.str());
	rebuildIdleTimeline("Initialized from text file", 1);
	return true;
}

bool SinglyLinkedList::initializeFromJsonFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open()) {
		lastMessage = "Could not open JSON file";
		rebuildIdleTimeline(lastMessage);
		return false;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	const std::string content = buffer.str();

	if (content.find('[') == std::string::npos || content.find(']') == std::string::npos) {
		lastMessage = "Invalid JSON format: expected an array";
		rebuildIdleTimeline(lastMessage);
		return false;
	}

	values = parseIntegers(content);
	rebuildIdleTimeline("Initialized from JSON file", 1);
	return true;
}

bool SinglyLinkedList::addAt(std::size_t index, int value)
{
	if (index > values.size()) {
		lastMessage = "Add failed: index out of range";
		rebuildIdleTimeline(lastMessage, 2);
		return false;
	}

	timeline.clear();
	pushFrame(-1, -1, 1, "Start add operation", SLLOperationType::Add);

	for (std::size_t i = 0; i < index; ++i) {
		pushFrame(static_cast<int>(i), -1, 2, "Traverse to insertion position", SLLOperationType::Add);
	}
	pushFrame(index == 0 ? -1 : static_cast<int>(index - 1), static_cast<int>(index), 3,
		"Create new node with value", SLLOperationType::Add);

	std::vector<int> beforeInsert = values;
	values.insert(values.begin() + static_cast<std::ptrdiff_t>(index), value);

	if (index == 0) {
		pushFrame(0, beforeInsert.empty() ? -1 : 1, 4,
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
	return true;
}

bool SinglyLinkedList::deleteAt(std::size_t index)
{
	if (values.empty()) {
		lastMessage = "Delete failed: list is empty";
		rebuildIdleTimeline(lastMessage, 1);
		return false;
	}
	if (index >= values.size()) {
		lastMessage = "Delete failed: index out of range";
		rebuildIdleTimeline(lastMessage, 2);
		return false;
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

	values.erase(values.begin() + static_cast<std::ptrdiff_t>(index));
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
	return true;
}

bool SinglyLinkedList::updateAt(std::size_t index, int value)
{
	if (index >= values.size()) {
		lastMessage = "Update failed: index out of range";
		rebuildIdleTimeline(lastMessage, 2);
		return false;
	}

	timeline.clear();
	pushFrame(-1, -1, 1, "Start update operation", SLLOperationType::Update);

	for (std::size_t i = 0; i <= index; ++i) {
		pushFrame(static_cast<int>(i), -1, 2, "Traverse to target node", SLLOperationType::Update);
	}

	values[index] = value;
	pushFrame(static_cast<int>(index), -1, 3, "Updated node value", SLLOperationType::Update);
	commitTimeline("Update complete");
	return true;
}

bool SinglyLinkedList::searchValue(int value)
{
	timeline.clear();
	pushFrame(-1, -1, 1, "Start search operation", SLLOperationType::Search);

	for (std::size_t i = 0; i < values.size(); ++i) {
		if (values[i] == value) {
			pushFrame(static_cast<int>(i), -1, 3, "Value found", SLLOperationType::Search);
			commitTimeline("Search complete: found");
			return true;
		}
		pushFrame(static_cast<int>(i), -1, 2, "Compare current node", SLLOperationType::Search);
	}

	pushFrame(-1, -1, 4, "Value not found", SLLOperationType::Search);
	commitTimeline("Search complete: not found");
	return false;
}

void SinglyLinkedList::stepForward()
{
	if (timeline.empty()) {
		return;
	}
	if (cursor + 1 < timeline.size()) {
		interpolation.previousFrame = currentFrame();
		++cursor;
		interpolation.currentFrame = timeline[cursor];
		interpolation.transitionProgress = 0.0f;
		interpolation.isTransitioning = true;
	}
}

void SinglyLinkedList::stepBackward()
{
	if (timeline.empty()) {
		return;
	}
	if (cursor > 0) {
		interpolation.previousFrame = currentFrame();
		--cursor;
		interpolation.currentFrame = timeline[cursor];
		interpolation.transitionProgress = 0.0f;
		interpolation.isTransitioning = true;
	}
}

void SinglyLinkedList::jumpToFinal()
{
	if (!timeline.empty()) {
		cursor = timeline.size() - 1;
		interpolation.isTransitioning = false;
		interpolation.transitionProgress = 0.0f;
	}
}

void SinglyLinkedList::jumpToStart()
{
	cursor = 0;
	autoplayAccumulator = 0.0f;
	interpolation.isTransitioning = false;
	interpolation.transitionProgress = 0.0f;
}

void SinglyLinkedList::updateAutoplay(float deltaSeconds, float speedMultiplier, bool enabled)
{
	if (!enabled || timeline.empty() || cursor + 1 >= timeline.size()) {
		autoplayAccumulator = 0.0f;
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

const SLLFrame& SinglyLinkedList::getInterpolatedFrame() const
{
	if (!interpolation.isTransitioning) {
		return currentFrame();
	}
	return interpolation.currentFrame;
}

bool SinglyLinkedList::hasTimeline() const
{
	return !timeline.empty();
}

void SinglyLinkedList::rebuildIdleTimeline(const std::string& message, int codeLine)
{
	timeline.clear();
	pushFrame(-1, -1, codeLine, message);
	commitTimeline(message);
}

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

void SinglyLinkedList::commitTimeline(const std::string& fallbackMessage)
{
	if (timeline.empty()) {
		pushFrame(-1, -1, -1, fallbackMessage);
	}
	cursor = 0;
	autoplayAccumulator = 0.0f;
	lastMessage = timeline.back().message.empty() ? fallbackMessage : timeline.back().message;
}

std::vector<int> SinglyLinkedList::parseIntegers(const std::string& content)
{
	std::vector<int> result;
	const std::regex numberRegex("-?[0-9]+");
	for (std::sregex_iterator it(content.begin(), content.end(), numberRegex), end; it != end; ++it) {
		result.push_back(std::stoi(it->str()));
	}
	return result;
}
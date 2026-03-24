#include <logic/singlylinkedlist.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <random>
#include <regex>
#include <sstream>

namespace {
	float pickInitializeTransitionDuration(std::size_t nodeCount)
	{
		const float base = 0.85f;
		const float extra = std::min(0.5f, static_cast<float>(nodeCount) * 0.015f);
		return base + extra;
	}
}

///-----------------------------------
/// INITIALIZATION
///-----------------------------------

void SinglyLinkedList::initializeEmpty()
{
	values.clear();
	timeline.clear();
	pushFrame(-1, -1, 1, "Initialized empty list", SLLOperationType::Initialize);
	commitTimeline("Initialized empty list");
	interpolation.isTransitioning = false;
	interpolation.transitionProgress = 0.0f;
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
	timeline.clear();
	pushFrame(-1, -1, 1, "Preparing nodes", SLLOperationType::Initialize);
	pushFrame(-1, -1, 1, oss.str(), SLLOperationType::Initialize);
	commitTimeline(oss.str());
	if (timeline.size() > 1) {
		cursor = 1;
		interpolation.previousFrame = timeline[0];
		interpolation.currentFrame = timeline[1];
		interpolation.transitionDuration = pickInitializeTransitionDuration(values.size());
		interpolation.transitionProgress = 0.0f;
		interpolation.isTransitioning = true;
	}
}

void SinglyLinkedList::initializeRandomSorted(int count, int minValue, int maxValue)
{
	if (count < 0) {
		count = 0;
	}
	if (minValue > maxValue) {
		std::swap(minValue, maxValue);
	}

	std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(minValue, maxValue);

	std::vector<int> sortedValues;
	sortedValues.reserve(static_cast<std::size_t>(count));
	for (int i = 0; i < count; ++i) {
		sortedValues.push_back(dist(rng));
	}
	std::sort(sortedValues.begin(), sortedValues.end());

	std::ostringstream oss;
	oss << "Initialized random sorted list with " << count << " nodes";
	initializeFromValues(sortedValues, oss.str());
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
	const std::vector<int> parsed = parseIntegers(buffer.str());
	initializeFromValues(parsed, "Initialized from text file");
	return true;
}

void SinglyLinkedList::initializeFromValues(const std::vector<int>& newValues, const std::string& sourceMessage)
{
	values = newValues;
	timeline.clear();
	pushFrame(-1, -1, 1, "Preparing nodes", SLLOperationType::Initialize);
	pushFrame(-1, -1, 1, sourceMessage, SLLOperationType::Initialize);
	commitTimeline(sourceMessage);
	if (timeline.size() > 1) {
		cursor = 1;
		interpolation.previousFrame = timeline[0];
		interpolation.currentFrame = timeline[1];
		interpolation.transitionDuration = pickInitializeTransitionDuration(values.size());
		interpolation.transitionProgress = 0.0f;
		interpolation.isTransitioning = true;
	}
}

///-----------------------------------
/// CORE OPERATIONS
///-----------------------------------

bool SinglyLinkedList::addAt(std::size_t index, int value)
{
	if (index > values.size()) {
		lastMessage = "Add failed: index out of range";
		return false;
	}

	values.insert(values.begin() + static_cast<std::ptrdiff_t>(index), value);
	lastMessage = "Add complete";
	return true;
}

bool SinglyLinkedList::deleteAt(std::size_t index)
{
	if (values.empty()) {
		lastMessage = "Delete failed: list is empty";
		return false;
	}
	if (index >= values.size()) {
		lastMessage = "Delete failed: index out of range";
		return false;
	}

	values.erase(values.begin() + static_cast<std::ptrdiff_t>(index));
	lastMessage = "Delete complete";
	return true;
}

bool SinglyLinkedList::updateAt(std::size_t index, int value)
{
	if (index >= values.size()) {
		lastMessage = "Update failed: index out of range";
		return false;
	}

	values[index] = value;
	lastMessage = "Update complete";
	return true;
}

bool SinglyLinkedList::searchValue(int value)
{
	for (std::size_t i = 0; i < values.size(); ++i) {
		if (values[i] == value) {
			lastMessage = "Search complete: found";
			return true;
		}
	}

	lastMessage = "Search complete: not found";
	return false;
}

///-----------------------------------
/// UTILITY
///-----------------------------------

std::vector<int> SinglyLinkedList::parseIntegers(const std::string& content)
{
	std::vector<int> result;
	const std::regex numberRegex("-?[0-9]+");
	for (std::sregex_iterator it(content.begin(), content.end(), numberRegex), end; it != end; ++it) {
		result.push_back(std::stoi(it->str()));
	}
	return result;
}

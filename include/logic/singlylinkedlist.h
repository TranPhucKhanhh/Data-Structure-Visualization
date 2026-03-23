#pragma once

#include <string>
#include <vector>

enum class SLLOperationType {
	Initialize,
	Add,
	Delete,
	Update,
	Search
};

struct SLLFrame {
	std::vector<int> values;
	int activeIndex = -1;
	int secondaryIndex = -1;
	int codeLine = -1;
	std::string message;
	SLLOperationType operationType = SLLOperationType::Add;
};

struct SLLInterpolationState {
	SLLFrame previousFrame;
	SLLFrame currentFrame;
	float transitionDuration = 0.45f;
	float transitionProgress = 0.0f;
	bool isTransitioning = false;
};

struct SinglyLinkedList {
	// Data
	std::vector<int> values;
	std::vector<SLLFrame> timeline;
	std::size_t cursor = 0;
	std::string lastMessage;
	SLLInterpolationState interpolation;
	float autoplayAccumulator = 0.0f;

	// Initialization
	void initializeEmpty();
	void initializeRandom(int count, int minValue, int maxValue);
	bool initializeFromTextFile(const std::string& path);

	// Operations
	bool addAt(std::size_t index, int value);
	bool deleteAt(std::size_t index);
	bool updateAt(std::size_t index, int value);
	bool searchValue(int value);

	// Playback
	void stepForward();
	void stepBackward();
	void jumpToFinal();
	void jumpToStart();
	void updateAutoplay(float deltaSeconds, float speedMultiplier, bool enabled);
	void updateInterpolation(float deltaSeconds);

	// Frame access
	const SLLFrame& currentFrame() const;
	const SLLFrame& getInterpolatedFrame() const;
	bool hasTimeline() const;

	// Internal
	void rebuildIdleTimeline(const std::string& message, int codeLine = 1);
	void pushFrame(int activeIndex, int secondaryIndex, int codeLine, const std::string& message, SLLOperationType opType = SLLOperationType::Add);
	void commitTimeline(const std::string& fallbackMessage);
	std::vector<int> parseIntegers(const std::string& content);

	// Visualization wrappers (build timeline before executing operation)
	void addAtViz(std::size_t index, int value);
	void deleteAtViz(std::size_t index);
	void updateAtViz(std::size_t index, int value);
	void searchValueViz(int value);
};

inline SinglyLinkedList singlyLinkedList;
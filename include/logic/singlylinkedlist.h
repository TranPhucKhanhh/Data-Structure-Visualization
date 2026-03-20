#pragma once

#include <string>
#include <vector>

enum class PlaybackMode {
	StepByStep,
	RunAtOnce,
};

enum class SLLOperationType {
	Initialize,
	Add,
	Delete,
	Update,
	Search,
};

struct SLLFrame {
	std::vector<int> values;
	int activeIndex = -1;
	int secondaryIndex = -1;
	int codeLine = -1;
	std::string message;
	SLLOperationType operationType = SLLOperationType::Initialize;
};

struct SLLInterpolationState {
	SLLFrame previousFrame;
	SLLFrame currentFrame;
	float transitionProgress = 0.0f;
	float transitionDuration = 0.45f;
	bool isTransitioning = false;
};

struct SinglyLinkedList {
	std::vector<int> values;
	std::vector<SLLFrame> timeline;
	std::size_t cursor = 0;
	float autoplayAccumulator = 0.0f;
	std::string lastMessage = "Ready";
	PlaybackMode playbackMode = PlaybackMode::StepByStep;
	float playbackSpeed = 1.0f;
	SLLInterpolationState interpolation;

	void initializeEmpty();
	void initializeRandom(int count, int minValue, int maxValue);
	bool initializeFromTextFile(const std::string& path);
	bool initializeFromJsonFile(const std::string& path);

	bool addAt(std::size_t index, int value);
	bool deleteAt(std::size_t index);
	bool updateAt(std::size_t index, int value);
	bool searchValue(int value);

	void stepForward();
	void stepBackward();
	void jumpToFinal();
	void jumpToStart();
	void updateAutoplay(float deltaSeconds, float speedMultiplier, bool enabled);
	void updateInterpolation(float deltaSeconds);

	const SLLFrame& currentFrame() const;
	const SLLFrame& getInterpolatedFrame() const;
	bool hasTimeline() const;

private:
	void rebuildIdleTimeline(const std::string& message, int codeLine = -1);
	void pushFrame(int activeIndex, int secondaryIndex, int codeLine, const std::string& message, SLLOperationType opType = SLLOperationType::Initialize);
	void commitTimeline(const std::string& fallbackMessage);
	static std::vector<int> parseIntegers(const std::string& content);

};

inline SinglyLinkedList singlyLinkedList;
#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "logic/trie.h"

enum class TriePlaybackMode {
    StepByStep,
    RunAtOnce
};

class TrieUI {
public:
    TrieUI();
    ~TrieUI();

    void draw();
    void drawSfml(sf::RenderWindow& window);

private:
    // Các biến lưu trữ input từ người dùng
    std::array<char, 512> createWords_{};
    std::array<char, 256> txtPath_{};
    std::array<char, 256> insertWord_{};
    std::array<char, 256> searchWord_{};
    std::array<char, 256> deleteWord_{};
    std::array<char, 256> updateOldWord_{};
    std::array<char, 256> updateNewWord_{};
    int randomWordCount_ = 8;
    int randomMinLength_ = 3;
    int randomMaxLength_ = 8;

    // Lưu trữ các bước mô phỏng
    std::vector<TrieInstruction> currentSteps_;
    int currentStepIndex_ = 0;
    int lastOperationMenuIndex_ = -1;
    float autoplayAccumulator_ = 0.0f;

    // Kết quả của hoạt động
    std::string operationResult_ = "";

    // Tùy chỉnh hiển thị
    float scrollOffsetX_ = 0.0f;
    float scrollOffsetY_ = 0.0f;
    float nodeRadius_ = 28.0f;
    float edgeThickness_ = 3.0f;
    float fontScale_ = 1.0f;
    bool showCodeOverlay_ = true;
    sf::Color canvasBgColor_{255, 255, 255, 255};
    sf::Color nodeBaseColor_{72, 149, 239, 255};
    sf::Color activeNodeColor_{96, 165, 250, 255};
    sf::Color secondaryNodeColor_{245, 204, 77, 255};
    sf::Color deleteNodeColor_{239, 68, 68, 255};
    sf::Color edgeColor_{70, 70, 70, 255};
    sf::Color valueTextColor_{42, 42, 42, 255};
    sf::Color highlightRingColor_{255, 214, 102, 230};

    // Panel state
    bool operationPanelCollapsed_ = false;
    bool commentPanelCollapsed_ = false;
    bool codePanelCollapsed_ = false;
    float operationPanelOpenT_ = 1.0f;
    float commentPanelOpenT_ = 1.0f;
    float codePanelOpenT_ = 1.0f;
    int operationMenuIndex_ = 0;

    // Playback
    bool autoplay_ = true;
    TriePlaybackMode playbackMode_ = TriePlaybackMode::RunAtOnce;
    float playbackSpeed_ = 1.0f;

    // Smooth transition state between timeline steps
    bool stepTransitioning_ = false;
    int transitionFromStep_ = 0;
    int transitionToStep_ = 0;
    float stepTransitionProgress_ = 1.0f;
    float stepTransitionDuration_ = 0.32f;

    // Cached node layout for animated active-marker movement
    std::unordered_map<std::string, sf::Vector2f> nodePositions_;

    // Snapshot root before an operation starts (used to replay visible animation state)
    TrieNode* animationBaseRoot_ = nullptr;
    bool hasAnimationBase_ = false;

    // Canvas interaction
    bool isCanvasDragging_ = false;
    sf::Vector2i lastDragMousePos_{0, 0};
    float zoomScale_ = 1.0f;

    // Hàm hỗ trợ vẽ đệ quy cây Trie
    void drawTrieNode(
        sf::RenderWindow& window,
        TrieNode* node,
        float x,
        float y,
        float horizontalGap,
        const sf::Font* font,
        const std::string& prefix,
        const std::string& activePath,
        const TrieInstruction* activeInstruction,
        float radius,
        float verticalGap,
        const std::unordered_set<std::string>* visiblePrefixes
    );

    void startStepTransition(int targetStep);
    std::string buildActivePath(int appliedCount) const;
};

inline TrieUI trie_ui;

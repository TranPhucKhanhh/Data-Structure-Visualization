#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <string>
#include <vector>
#include "logic/trie.h" 

enum class TriePlaybackMode {
    StepByStep,
    RunAtOnce
};

class TrieUI {
public:
    TrieUI();

    void draw();
    void drawSfml(sf::RenderWindow& window);

private:
    // Các biến lưu trữ input từ người dùng
    std::array<char, 256> insertWord_{};
    std::array<char, 256> searchWord_{};
    std::array<char, 256> deleteWord_{};
    std::array<char, 256> updateOldWord_{};
    std::array<char, 256> updateNewWord_{};

    // Điều khiển Playback
    bool autoplay_ = false;
    TriePlaybackMode playbackMode_ = TriePlaybackMode::StepByStep;
    float playbackSpeed_ = 1.0f;
    
    // Lưu trữ các bước mô phỏng (dựa trên TrieOp của bạn)
    std::vector<TrieInstruction> currentSteps_;
    int currentStepIndex_ = 0;
    
    // Kết quả của hoạt động
    std::string operationResult_ = "";

    // Tùy chỉnh hiển thị
    float scrollOffsetX_ = 0.0f;
    float scrollOffsetY_ = 0.0f;
    float nodeRadius_ = 20.0f;
    float edgeThickness_ = 2.0f;
    float fontScale_ = 1.0f;
    bool showCodeOverlay_ = true;

    // Hàm hỗ trợ vẽ đệ quy cây Trie
    void drawTrieNode(sf::RenderWindow& window, TrieNode* node, float x, float y, float horizontalGap, const sf::Font* font);
};

inline TrieUI trie_ui;
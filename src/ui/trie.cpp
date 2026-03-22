#include "ui/trie.h"
#include "ui/common.h"
#include <imgui.h>
#include <SFML/Graphics.hpp>
#include <cmath>

namespace {
    const sf::Font* getTrieFont() {
        static sf::Font font;
        static bool attempted = false;
        static bool loaded = false;
        if (!attempted) {
            attempted = true;
            // Thử load Arial trước
            if (font.openFromFile("C:/Windows/Fonts/arial.ttf") ||
                font.openFromFile("C:/Windows/Fonts/Arial.ttf") ||
                font.openFromFile("C:/Windows/Fonts/times.ttf") ||
                font.openFromFile("C:/Windows/Fonts/Times.ttf") ||
                font.openFromFile("C:/Windows/Fonts/tahoma.ttf") ||
                font.openFromFile("C:/Windows/Fonts/Tahoma.ttf")) {
                loaded = true;
            }
        }
        return loaded ? &font : nullptr;
    }
}

TrieUI::TrieUI() {
    // Khởi tạo nếu cần
}

void TrieUI::draw() {
    ImGui::SetNextWindowPos(ImVec2(16.0f, 16.0f), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(450.0f, 860.0f), ImGuiCond_Once);

    if (!ImGui::Begin("Trie Visualizer")) {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Operations");
    ImGui::PushItemWidth(120.0f);

    // Insert
    ImGui::InputText("##insert", insertWord_.data(), insertWord_.size());
    ImGui::SameLine();
    if (ImGui::Button("Insert Word")) {
        std::string word(insertWord_.data());
        if (!word.empty()) {
            currentSteps_ = trie.insertWordStep(word);
            currentStepIndex_ = 0;
            operationResult_ = "";  // Clear result
        }
    }

    // Search
    ImGui::InputText("##search", searchWord_.data(), searchWord_.size());
    ImGui::SameLine();
    if (ImGui::Button("Search Word")) {
        std::string word(searchWord_.data());
        if (!word.empty()) {
            currentSteps_ = trie.searchWordStep(word);
            currentStepIndex_ = 0;
            // Kiểm tra kết quả cuối cùng của search
            if (!currentSteps_.empty() && currentSteps_.back().trie_op == TrieOp::FOUND_WORD) {
                operationResult_ = "Found!";
            } else {
                operationResult_ = "Not Found!";
            }
        }
    }

    // Delete
    ImGui::InputText("##delete", deleteWord_.data(), deleteWord_.size());
    ImGui::SameLine();
    if (ImGui::Button("Delete Word")) {
        std::string word(deleteWord_.data());
        if (!word.empty()) {
            currentSteps_ = trie.deleteWordStep(word);
            currentStepIndex_ = 0;
            operationResult_ = "";  // Clear result
        }
    }

    ImGui::PopItemWidth();

    ImGui::SeparatorText("Playback Controls");
    if (ImGui::Button("Prev Step")) {
        if (currentStepIndex_ > 0) currentStepIndex_--;
    }
    ImGui::SameLine();
    if (ImGui::Button("Next Step")) {
        if (currentStepIndex_ < currentSteps_.size()) currentStepIndex_++;
    }
    ImGui::SameLine();
    ImGui::Text("Step: %d / %zu", currentStepIndex_, currentSteps_.size());
    
    // Hiển thị kết quả
    if (!operationResult_.empty()) {
        ImGui::SeparatorText("Result");
        ImGui::TextColored(
            operationResult_ == "Found!" ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1),
            "%s",
            operationResult_.c_str()
        );
    }

    ImGui::SeparatorText("Style");
    ImGui::SliderFloat("Node radius##trie", &nodeRadius_, 12.0f, 40.0f, "%.1f");
    
    if (ImGui::Button("Back to menu")) {
        uiConfig.state = UIState::Menu;
    }

    ImGui::End();
}

void TrieUI::drawSfml(sf::RenderWindow& window) {
    const sf::Vector2u size = window.getSize();
    const sf::Font* font = getTrieFont();

    // Background
    sf::RectangleShape background(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    background.setFillColor(sf::Color(16, 21, 30));
    window.draw(background);

    TrieNode* root = trie.getRoot();
    if (!root) return;

    float startX = size.x / 2.0f;
    float startY = 80.0f;
    float initialGap = size.x / 3.0f; // Khoảng cách lan tỏa ban đầu

    // Gọi hàm đệ quy để vẽ 
    drawTrieNode(window, root, startX, startY, initialGap, font);
}

void TrieUI::drawTrieNode(sf::RenderWindow& window, TrieNode* node, float x, float y, float horizontalGap, const sf::Font* font) {
    if (!node) return;

    // Vẽ vòng tròn cho Node
    sf::CircleShape circle(nodeRadius_);
    circle.setOrigin(sf::Vector2f(nodeRadius_, nodeRadius_));
    circle.setPosition(sf::Vector2f(x, y));
    circle.setOutlineThickness(edgeThickness_);
    
    // Đổi màu nếu là điểm kết thúc của từ (is_end_of_word)
    if (node->is_end_of_word) {
        circle.setFillColor(sf::Color(245, 158, 11)); // Màu vàng cam
    } else {
        circle.setFillColor(sf::Color(72, 149, 239)); // Màu xanh lơ
    }
    window.draw(circle);

    // Tính toán và vẽ các Node con (children)
    int childCount = 0;
    for (int i = 0; i < 26; ++i) {
        if (node->children[i] != nullptr) childCount++;
    }

    if (childCount == 0) return;

    float currentX = x - (horizontalGap * (childCount - 1)) / 2.0f;
    float nextY = y + 80.0f; // Khoảng cách dọc giữa các level

    for (int i = 0; i < 26; ++i) {
        if (node->children[i] != nullptr) {
            // Gọi đệ quy vẽ con trước (vẽ node con và subtree của nó)
            drawTrieNode(window, node->children[i], currentX, nextY, horizontalGap / 1.5f, font);

            // Vẽ đường nối (Edge) - vẽ SAU node con để nó nằm trên node
            sf::Vertex line[] = {
                sf::Vertex{sf::Vector2f(x, y + nodeRadius_), sf::Color(148, 163, 184)},
                sf::Vertex{sf::Vector2f(currentX, nextY - nodeRadius_), sf::Color(148, 163, 184)}
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);

            // Vẽ chữ cái tại node con - vẽ CUỐI cùng để nó chắc chắn nằm trên node
            char letter = 'a' + i;
            if (font) {
                sf::Text letterText(*font, std::string(1, letter), static_cast<unsigned int>(nodeRadius_ * 0.8f));
                letterText.setFillColor(sf::Color::White);

                sf::FloatRect textBounds = letterText.getLocalBounds();
                letterText.setOrigin(sf::Vector2f(textBounds.size.x / 2.0f, textBounds.size.y / 2.0f));
                letterText.setPosition(sf::Vector2f(currentX, nextY - nodeRadius_ * 0.3f));

                window.draw(letterText);
            }
            currentX += horizontalGap;
        }
    }
}
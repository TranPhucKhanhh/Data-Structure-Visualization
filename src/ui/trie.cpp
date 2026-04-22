#include "ui/trie.h"
#include "ui/common.h"

#include <imgui.h>

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
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

    const sf::Font* getTrieFont() {
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


    const char* kInsertCode[] = {
        "1  FUNCTION insert(word, root):",
        "2      node = root",
        "3      FOR EACH char in word:",
        "4          IF char NOT IN node.children THEN node.children[char] = NEW NODE",
        "5          node = node.children[char]",
        "6      node.is_end_of_word = true"
    };

    const char* kSearchCode[] = {
        "1  FUNCTION search(word, root):",
        "2      node = root",
        "2      FOR each char in word:",
        "3          IF char NOT IN node.children THEN return false ",
        "4          node = node.children[char]",
        "5      return node.is_end_of_word"
    };

    const char* kDeleteCode[] = {
        "1  FUNCTION deleteWord(node, word, depth):",
		"2      IF node IS NULL: RETURN NULL",
        "3      IF depth = LENGTH(word): node.is_end_of_word = false",
        "4      ELSE:",
        "5          c = word[depth]",
        "6          node.children[c] = deleteWord(node.children[c], word, depth + 1)",
        "7      IF node.children IS empty AND node.is_end_of_word == false:",
        "8          DELETE node",
        "9          RETURN NULL",
        "10      RETURN node"
    };

    int mapInstructionToCodeLine(const TrieInstruction* instruction, int operationMenuIndex) {
        if (instruction == nullptr) {
            return 1;
        }

        switch (operationMenuIndex) {
        case 0:
        case 2:
            switch (instruction->trie_op) {
            case TrieOp::CREATE_NODE: return 4;
            case TrieOp::MOVE_TO_NODE: return 5;
            case TrieOp::MARK_END: return 6;
            default: return 3;
            }
        case 1:
            switch (instruction->trie_op) {
            case TrieOp::MOVE_TO_NODE: return 5;
            case TrieOp::NOT_FOUND: return 4;
            case TrieOp::FOUND_WORD: return 6;
            default: return 3;
            }
        case 3:
            switch (instruction->trie_op) {
            case TrieOp::MOVE_TO_NODE: return 4;
            case TrieOp::UNMARK_END: return 3;
            case TrieOp::DELETE_PHYSICAL: return 7;
            case TrieOp::RETURN_NODE: return 10;
            case TrieOp::RETURN_NULL: return 2;
            case TrieOp::NOT_FOUND: return 2;
            default: return 1;
            }
        default:
            return 1;
        }
    }

    std::string instructionToComment(const TrieInstruction* instruction) {
        if (instruction == nullptr) {
            return "Ready";
        }

        switch (instruction->trie_op) {
        case TrieOp::MOVE_TO_NODE:
            return std::string("Move to node '") + instruction->character + "'";
        case TrieOp::CREATE_NODE:
            return std::string("Create new node '") + instruction->character + "'";
        case TrieOp::MARK_END:
            return "Mark end of word";
        case TrieOp::FOUND_WORD:
            return "Word found";
        case TrieOp::NOT_FOUND:
            return "Word not found";
        case TrieOp::UNMARK_END:
            return "Unmark end of word";
        case TrieOp::DELETE_PHYSICAL:
            return "Delete redundant node";
        case TrieOp::RETURN_NODE:
            return instruction->character >= 'a' && instruction->character <= 'z'
                ? std::string("Return current node after processing '") + instruction->character + "'"
                : "Return current node";
        case TrieOp::RETURN_NULL:
            return "Return NULL";
        default:
            return "Processing";
        }
    }

    std::vector<int> mapInstructionToCodeLines(const TrieInstruction* instruction, int operationMenuIndex) {
        if (instruction == nullptr) {
            return {1};
        }

        if (operationMenuIndex == 3) {
            switch (instruction->trie_op) {
            case TrieOp::MOVE_TO_NODE:
                return {4, 5, 6};
            case TrieOp::DELETE_PHYSICAL:
                return {7, 8, 9};
            case TrieOp::UNMARK_END:
                return {3};
            case TrieOp::RETURN_NODE:
                return {10};
            case TrieOp::RETURN_NULL:
                return {2};
            case TrieOp::NOT_FOUND:
                return {2};
            default:
                return {1};
            }
        }

        return {mapInstructionToCodeLine(instruction, operationMenuIndex)};
    }

    void pickCodeBlock(int operationMenuIndex, const char**& codeArray, int& lineCount, const char*& title) {
        switch (operationMenuIndex) {
        case 0:
            codeArray = kInsertCode;
            lineCount = 6;
            title = "CREATE / INSERT";
            break;
        case 1:
            codeArray = kSearchCode;
            lineCount = 6;
            title = "SEARCH";
            break;
        case 2:
            codeArray = kInsertCode;
            lineCount = 6;
            title = "INSERT";
            break;
        case 3:
            codeArray = kDeleteCode;
            lineCount = 10;
            title = "REMOVE";
            break;
        default:
            codeArray = nullptr;
            lineCount = 0;
            title = "TRIE";
            break;
        }
    }

    int pickCodeOperationForTimeline(int operationMenuIndex, const std::vector<TrieInstruction>& steps, int displayStepIndex) {
        if (operationMenuIndex != 4) {
            return operationMenuIndex;
        }

        const int currentInstructionIndex = displayStepIndex - 1;
        if (currentInstructionIndex < 0) {
            return 3;
        }

        int insertStartIndex = static_cast<int>(steps.size());
        bool oldWordWasDeleted = false;
        for (int i = 0; i < static_cast<int>(steps.size()); ++i) {
            const TrieOp op = steps[static_cast<std::size_t>(i)].trie_op;
            if (!oldWordWasDeleted) {
                if (op == TrieOp::UNMARK_END) {
                    oldWordWasDeleted = true;
                }
                continue;
            }

            if (op == TrieOp::DELETE_PHYSICAL || op == TrieOp::RETURN_NODE || op == TrieOp::RETURN_NULL) {
                continue;
            }

            if (op == TrieOp::CREATE_NODE || op == TrieOp::MOVE_TO_NODE || op == TrieOp::MARK_END) {
                insertStartIndex = i;
                break;
            }
        }

        return currentInstructionIndex >= insertStartIndex ? 2 : 3;
    }

    int countVisibleLeafSlots(TrieNode* node, const std::string& prefix, const std::unordered_set<std::string>* visiblePrefixes) {
        if (node == nullptr) {
            return 0;
        }

        int total = 0;
        for (int i = 0; i < 26; ++i) {
            TrieNode* child = node->children[i];
            if (child == nullptr) {
                continue;
            }

            const std::string childPrefix = prefix + static_cast<char>('a' + i);
            if (visiblePrefixes != nullptr && visiblePrefixes->find(childPrefix) == visiblePrefixes->end()) {
                continue;
            }

            total += countVisibleLeafSlots(child, childPrefix, visiblePrefixes);
        }

        return std::max(1, total);
    }
}

TrieUI::TrieUI() {}

TrieUI::~TrieUI() {
    trie.clearHelper(animationBaseRoot_);
    hasAnimationBase_ = false;
}

void TrieUI::startStepTransition(int targetStep) {
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

std::string TrieUI::buildActivePath(int appliedCount) const {
    std::string activePath;
    const int count = std::clamp(appliedCount, 0, static_cast<int>(currentSteps_.size()));
    for (int i = 0; i < count; ++i) {
        const TrieInstruction& step = currentSteps_[i];
        if (step.trie_op == TrieOp::MOVE_TO_NODE && step.character >= 'a' && step.character <= 'z') {
            activePath.push_back(step.character);
        }
        else if (step.trie_op == TrieOp::MARK_END || step.trie_op == TrieOp::FOUND_WORD || step.trie_op == TrieOp::NOT_FOUND) {
            // A word-level operation is finished; start tracking the next word from root.
            activePath.clear();
        }
        else if (step.trie_op == TrieOp::DELETE_PHYSICAL && !activePath.empty()) {
            activePath.pop_back();
        }
        else if (step.trie_op == TrieOp::RETURN_NODE && step.character >= 'a' && step.character <= 'z' && !activePath.empty()) {
            activePath.pop_back();
        }
    }
    return activePath;
}

void TrieUI::draw() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 vpPos = viewport->Pos;
    const ImVec2 vpSize = viewport->Size;
    const float dt = ImGui::GetIO().DeltaTime;
    const float foldLerp = 1.0f - std::exp(-14.0f * dt);

    operationPanelOpenT_ = std::clamp(lerp(operationPanelOpenT_, operationPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
    commentPanelOpenT_ = std::clamp(lerp(commentPanelOpenT_, commentPanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);
    codePanelOpenT_ = std::clamp(lerp(codePanelOpenT_, codePanelCollapsed_ ? 0.0f : 1.0f, foldLerp), 0.0f, 1.0f);

    ImGui::SetNextWindowPos(vpPos);
    ImGui::SetNextWindowSize(ImVec2(vpSize.x, 44.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.06f, 0.98f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
    if (ImGui::Begin("##TrieTopBar", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
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
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
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

    auto startTimeline = [&](std::vector<TrieInstruction>&& steps, int sourceMenuIndex, const std::string& fallbackMessage) {
        currentSteps_ = std::move(steps);
        currentStepIndex_ = 0;
        transitionFromStep_ = 0;
        transitionToStep_ = 0;
        stepTransitioning_ = false;
        stepTransitionProgress_ = 1.0f;
        lastOperationMenuIndex_ = sourceMenuIndex;
        autoplayAccumulator_ = 0.0f;
        operationResult_ = fallbackMessage;
        autoplay_ = true;
        playbackMode_ = TriePlaybackMode::RunAtOnce;
    };

    auto captureAnimationBase = [&]() {
        trie.clearHelper(animationBaseRoot_);
        animationBaseRoot_ = trie.cloneTrieNode(trie.getRoot());
        hasAnimationBase_ = (animationBaseRoot_ != nullptr);
    };

    auto captureEmptyAnimationBase = [&]() {
        trie.clearHelper(animationBaseRoot_);
        animationBaseRoot_ = new TrieNode();
        hasAnimationBase_ = true;
    };

    const float drawerBottomY = vpPos.y + vpSize.y - 300.0f;
    ImGui::SetNextWindowPos(ImVec2(vpPos.x, drawerBottomY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(52.0f, 200.0f), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
    if (ImGui::Begin("Operation Toggle##Trie", nullptr,
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
        if (ImGui::Begin("Operations##TrieOperations", nullptr,
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
        const float inputPanelHeight = 200.0f;
        const float inputPanelX = vpPos.x + 52.0f + operationPanelWidth + 2.0f;
        ImGui::SetNextWindowPos(ImVec2(inputPanelX, drawerBottomY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(inputPanelWidth, inputPanelHeight), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.34f, 0.72f, 0.42f, 0.96f));
        if (ImGui::Begin("Operation Inputs##TrieInputs", nullptr,
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
                randomWordCount_ = std::max(1, randomWordCount_);
                randomMinLength_ = std::max(1, randomMinLength_);
                randomMaxLength_ = std::max(randomMinLength_, randomMaxLength_);

                ImGui::TextUnformatted("Initialize Trie");

                if (ImGui::Button("Empty", ImVec2(90.0f, 0.0f))) {
                    captureEmptyAnimationBase();
                    trie.clear();
                    startTimeline({}, 0, "Initialized empty trie");
                }
                ImGui::SameLine();
                if (ImGui::Button("Random", ImVec2(90.0f, 0.0f))) {
                    std::vector<std::string> words = trie.generateRandomWords(randomWordCount_, randomMinLength_, randomMaxLength_);
                    if (words.empty()) {
                        operationResult_ = "Random initialize failed";
                    }
                    else {
                        captureEmptyAnimationBase();
                        std::vector<TrieInstruction> steps = trie.initFromListStep(words);
                        startTimeline(std::move(steps), 0, "Initialized from random words");
                    }
                }

                ImGui::Spacing();
                ImGui::TextUnformatted("Random settings");

                ImGui::TextUnformatted("N =");
                ImGui::SameLine();
                ImGui::PushItemWidth(90.0f);
                ImGui::InputInt("##TrieRandomCount", &randomWordCount_);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::TextUnformatted("Min Len =");
                ImGui::SameLine();
                ImGui::PushItemWidth(90.0f);
                ImGui::InputInt("##TrieRandomMinLen", &randomMinLength_);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::TextUnformatted("Max Len =");
                ImGui::SameLine();
                ImGui::PushItemWidth(90.0f);
                ImGui::InputInt("##TrieRandomMaxLen", &randomMaxLength_);
                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::TextUnformatted("Custom words (comma-separated)");
                const float createRowButtonW = 72.0f;
                const float createRowInputW = std::max(180.0f, ImGui::GetContentRegionAvail().x - createRowButtonW - 8.0f);
                ImGui::PushItemWidth(createRowInputW);
                ImGui::InputText("##TrieCreateWords", createWords_.data(), createWords_.size());
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Go", ImVec2(createRowButtonW, 0.0f))) {
                    std::vector<std::string> words = trie.parseWordList(createWords_.data());
                    if (words.empty()) {
                        operationResult_ = "Create failed: enter comma-separated words (letters only)";
                    }
                    else {
                        captureEmptyAnimationBase();
                        std::vector<TrieInstruction> steps = trie.initFromListStep(words);
                        startTimeline(std::move(steps), 0, "Create from word list");
                    }
                }

                ImGui::Spacing();
                ImGui::TextUnformatted("Load from .txt");

                const float browseButtonW = 110.0f;
                const float loadButtonW = 100.0f;
                const float spacingW = 16.0f;
                const float loadInputW = std::max(
                    180.0f,
                    ImGui::GetContentRegionAvail().x - browseButtonW - loadButtonW - spacingW
                );

                ImGui::PushItemWidth(loadInputW);
                ImGui::InputText("##TrieLoadTxt", txtPath_.data(), txtPath_.size());
                ImGui::PopItemWidth();

                ImGui::SameLine();
                if (ImGui::Button("Browse File", ImVec2(browseButtonW, 0.0f))) {
                    std::string selectedPath = cr::utils::SimpleFileDialog::dialog();
                    if (!selectedPath.empty()) {
                        std::snprintf(txtPath_.data(), txtPath_.size(), "%s", selectedPath.c_str());
                    }
                }

                ImGui::SameLine();
                if (ImGui::Button("Load txt", ImVec2(loadButtonW, 0.0f))) {
                    const std::filesystem::path filePath(txtPath_.data());
                    if (txtPath_[0] == '\0' || !std::filesystem::exists(filePath)) {
                        operationResult_ = "Load failed: file not found";
                    }
                    else {
                        captureEmptyAnimationBase();
                        std::vector<TrieInstruction> steps = trie.initFromFileStep(txtPath_.data());
                        if (steps.empty()) {
                            operationResult_ = "Load failed: no valid words in file";
                        }
                        else {
                            startTimeline(std::move(steps), 0, "Initialized from text file");
                        }
                    }
                }

                ImGui::Spacing();
            }
            else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 1) {
                ImGui::TextUnformatted("Word:");
                ImGui::SameLine();
                ImGui::PushItemWidth(180.0f);
                ImGui::InputText("##TrieSearchWord", searchWord_.data(), searchWord_.size());
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Search", ImVec2(95.0f, 0.0f))) {
                    const std::string word = trie.sanitizeWord(searchWord_.data());
                    if (word.empty()) {
                        operationResult_ = "Search failed: enter a valid word";
                    }
                    else {
                        captureAnimationBase();
                        std::vector<TrieInstruction> steps = trie.searchWordStep(word);
                        const bool found = !steps.empty() && steps.back().trie_op == TrieOp::FOUND_WORD;
                        startTimeline(std::move(steps), 1, found ? "Found" : "Not Found");
                    }
                }
            }
            else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 2) {
                ImGui::TextUnformatted("Word:");
                ImGui::SameLine();
                ImGui::PushItemWidth(180.0f);
                ImGui::InputText("##TrieInsertWord", insertWord_.data(), insertWord_.size());
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Insert", ImVec2(95.0f, 0.0f))) {
                    const std::string word = trie.sanitizeWord(insertWord_.data());
                    if (word.empty()) {
                        operationResult_ = "Insert failed: enter a valid word";
                    }
                    else {
                        captureAnimationBase();
                        std::vector<TrieInstruction> steps = trie.insertWordStep(word);
                        startTimeline(std::move(steps), 2, "Inserted");
                    }
                }
            }
            else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 3) {
                ImGui::TextUnformatted("Word:");
                ImGui::SameLine();
                ImGui::PushItemWidth(180.0f);
                ImGui::InputText("##TrieDeleteWord", deleteWord_.data(), deleteWord_.size());
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Remove", ImVec2(95.0f, 0.0f))) {
                    const std::string word = trie.sanitizeWord(deleteWord_.data());
                    if (word.empty()) {
                        operationResult_ = "Remove failed: enter a valid word";
                    }
                    else {
                        captureAnimationBase();
                        std::vector<TrieInstruction> steps = trie.deleteWordStep(word);
                        startTimeline(std::move(steps), 3, "Remove operation" );
                    }
                }
            }
            else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 4) {
                ImGui::TextUnformatted("Old:");
                ImGui::SameLine();
                ImGui::PushItemWidth(140.0f);
                ImGui::InputText("##TrieUpdateOld", updateOldWord_.data(), updateOldWord_.size());
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::TextUnformatted("New:");
                ImGui::SameLine();
                ImGui::PushItemWidth(140.0f);
                ImGui::InputText("##TrieUpdateNew", updateNewWord_.data(), updateNewWord_.size());
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Update", ImVec2(95.0f, 0.0f))) {
                    const std::string oldWord = trie.sanitizeWord(updateOldWord_.data());
                    const std::string newWord = trie.sanitizeWord(updateNewWord_.data());
                    if (oldWord.empty() || newWord.empty()) {
                        operationResult_ = "Update failed: old/new words must be valid";
                    }
                    else {
                        captureAnimationBase();
                        std::vector<TrieInstruction> steps = trie.updateWordStep(oldWord, newWord);
                        startTimeline(std::move(steps), 4, "Update operation");
                    }
                }
            }
            else if (operationPanelOpenT_ > 0.65f && operationMenuIndex_ == 5) {
                ImGui::PushItemWidth(220.0f);
                ImGui::SliderFloat("Node Radius", &nodeRadius_, 12.0f, 44.0f, "%.1f");
                ImGui::SliderFloat("Edge Thickness", &edgeThickness_, 1.0f, 6.0f, "%.1f");
                ImGui::SliderFloat("Font Scale", &fontScale_, 0.7f, 1.8f, "%.2f");
                ImGui::PopItemWidth();
                ImGui::Checkbox("Code Overlay", &showCodeOverlay_);
                if (ImGui::Button("Reset Visuals", ImVec2(124.0f, 0.0f))) {
                    nodeRadius_ = 28.0f;
                    edgeThickness_ = 3.0f;
                    fontScale_ = 1.0f;
                    zoomScale_ = 1.0f;
                    scrollOffsetX_ = 0.0f;
                    scrollOffsetY_ = 0.0f;
                    canvasBgColor_ = sf::Color(255, 255, 255, 255);
                    nodeBaseColor_ = sf::Color(72, 149, 239, 255);
                    activeNodeColor_ = sf::Color(96, 165, 250, 255);
                    secondaryNodeColor_ = sf::Color(245, 204, 77, 255);
                    deleteNodeColor_ = sf::Color(239, 68, 68, 255);
                    edgeColor_ = sf::Color(70, 70, 70, 255);
                    valueTextColor_ = sf::Color(42, 42, 42, 255);
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

    int displayStepIndex = currentStepIndex_;
    if (stepTransitioning_ && stepTransitionProgress_ > 0.5f) {
        displayStepIndex = transitionToStep_;
    }
    displayStepIndex = std::clamp(displayStepIndex, 0, static_cast<int>(currentSteps_.size()));

    const TrieInstruction* activeInstruction = nullptr;
    if (!currentSteps_.empty() && displayStepIndex > 0) {
        activeInstruction = &currentSteps_[displayStepIndex - 1];
    }
    const std::string currentComment = (activeInstruction != nullptr)
        ? instructionToComment(activeInstruction)
        : (operationResult_.empty() ? "Ready" : operationResult_);

    const float rightTabWidth = 26.0f;
    const float rightPanelWidth = 550.0f;
    const float commentY = vpPos.y + vpSize.y - 450.0f;
    const float commentH = 115.0f;
    const float codeY = vpPos.y + vpSize.y - 330.0f;
    const float codeH = 270.0f;
    const float rightTabX = vpPos.x + vpSize.x - rightTabWidth;

    const float animatedCommentWidth = rightPanelWidth * commentPanelOpenT_;
    if (animatedCommentWidth > 6.0f) {
        ImGui::SetNextWindowPos(ImVec2(rightTabX - animatedCommentWidth, commentY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(animatedCommentWidth, commentH), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.98f, 0.55f, 0.16f, 0.96f));
        if (ImGui::Begin("Traversal Comment##TrieComment", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar)) {
            if (commentPanelOpenT_ > 0.55f) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.14f, 0.10f, 0.07f, 1.0f));
                const bool usingCommentFont = (menuCardDescFont != nullptr);
                if (usingCommentFont) {
                    ImGui::PushFont(menuCardDescFont);
                }
                ImGui::TextWrapped("%s", currentComment.c_str());
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
    if (ImGui::Begin("Traversal Comment Toggle##TrieCommentToggle", nullptr,
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
        if (ImGui::Begin("Source Code##TrieCode", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoScrollbar)) {
            if (codePanelOpenT_ > 0.55f) {
                //ImGui::SetWindowFontScale(0.82f);
                const char** codeArray = nullptr;
                int lineCount = 0;
                const char* opTitle = "TRIE";
                const int codeOperationIndex = pickCodeOperationForTimeline(lastOperationMenuIndex_, currentSteps_, displayStepIndex);
                pickCodeBlock(codeOperationIndex, codeArray, lineCount, opTitle);
                if (codeArray != nullptr && lineCount > 0) {
                    const std::vector<int> highlightedLines = mapInstructionToCodeLines(activeInstruction, codeOperationIndex);
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
    if (ImGui::Begin("Source Code Toggle##TrieCodeToggle", nullptr,
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

    ImGui::SetNextWindowPos(ImVec2(vpPos.x, vpPos.y + vpSize.y - 48.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(vpSize.x, 48.0f), ImGuiCond_Always);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.03f, 0.03f, 0.98f));
    if (ImGui::Begin("Playback##TrieBottomPlayback", nullptr,
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar)) {
        ImGui::PushItemWidth(140.0f);
        ImGui::SliderFloat("##TrieBottomPlaybackSpeed", &playbackSpeed_, 0.25f, 5.0f, "");
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text("%.2gx", playbackSpeed_);

        ImGui::SameLine(vpSize.x * 0.43f);
        if (ImGui::Button("|<")) {
            autoplay_ = false;
            currentStepIndex_ = 0;
            autoplayAccumulator_ = 0.0f;
            stepTransitioning_ = false;
            stepTransitionProgress_ = 1.0f;
        }
        ImGui::SameLine();
        if (ImGui::Button("<")) {
            autoplay_ = false;
            playbackMode_ = TriePlaybackMode::StepByStep;
            startStepTransition(currentStepIndex_ - 1);
        }
        ImGui::SameLine();
        if (ImGui::Button(autoplay_ ? "[]" : "|>")) {
            autoplay_ = !autoplay_;
            if (autoplay_) {
                playbackMode_ = TriePlaybackMode::RunAtOnce;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(">")) {
            autoplay_ = false;
            playbackMode_ = TriePlaybackMode::StepByStep;
            startStepTransition(currentStepIndex_ + 1);
        }
        ImGui::SameLine();
        if (ImGui::Button(">|")) {
            autoplay_ = false;
            currentStepIndex_ = static_cast<int>(currentSteps_.size());
            stepTransitioning_ = false;
            stepTransitionProgress_ = 1.0f;
        }

        if (!currentSteps_.empty()) {
            int frameIndex = currentStepIndex_;
            const int maxFrame = static_cast<int>(currentSteps_.size());
            ImGui::SameLine(vpSize.x * 0.58f);
            ImGui::PushItemWidth(vpSize.x * 0.36f);
            if (ImGui::SliderInt("##TrieBottomTimeline", &frameIndex, 0, maxFrame, "")) {
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

    if (autoplay_ && playbackMode_ == TriePlaybackMode::RunAtOnce && currentStepIndex_ < static_cast<int>(currentSteps_.size()) && !stepTransitioning_) {
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

void TrieUI::drawSfml(sf::RenderWindow& window) {
    const sf::Vector2u size = window.getSize();
    const sf::Font* font = getTrieFont();

    sf::RectangleShape background(sf::Vector2f(static_cast<float>(size.x), static_cast<float>(size.y)));
    background.setFillColor(canvasBgColor_);
    window.draw(background);

    TrieNode* previewRoot = nullptr;
    const int displayAppliedCount = std::clamp(
        stepTransitioning_
            ? (stepTransitionProgress_ < 0.5f ? transitionFromStep_ : transitionToStep_)
            : currentStepIndex_,
        0,
        static_cast<int>(currentSteps_.size())
    );
    if (hasAnimationBase_ && animationBaseRoot_ != nullptr && !currentSteps_.empty()) {
        previewRoot = trie.cloneTrieNode(animationBaseRoot_);
        trie.applyStepsToPreviewTrie(previewRoot, currentSteps_, displayAppliedCount);
    }

    TrieNode* root = (previewRoot != nullptr) ? previewRoot : trie.getRoot();
    if (root == nullptr) {
        if (previewRoot != nullptr) {
            trie.clearHelper(previewRoot);
        }
        return;
    }

    const sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    const bool mouseInsideCanvas =
        mousePos.x >= 0 && mousePos.y >= 0 &&
        mousePos.x < static_cast<int>(size.x) && mousePos.y < static_cast<int>(size.y);
    const bool canDragCanvas = mouseInsideCanvas && !ImGui::GetIO().WantCaptureMouse;

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

    const int fromStep = stepTransitioning_ ? transitionFromStep_ : currentStepIndex_;
    const int toStep = stepTransitioning_ ? transitionToStep_ : currentStepIndex_;
    const float transitionT = stepTransitioning_ ? easeInOut(stepTransitionProgress_) : 1.0f;
    const std::string fromPath = buildActivePath(fromStep);
    const std::string toPath = buildActivePath(toStep);
    const std::string activePath = stepTransitioning_ ? fromPath : toPath;

    const TrieInstruction* activeInstruction = nullptr;
    const int instructionIndex = stepTransitioning_ ? fromStep : toStep;
    if (!currentSteps_.empty() && instructionIndex > 0) {
        activeInstruction = &currentSteps_[instructionIndex - 1];
    }

    const float radius = std::clamp(nodeRadius_ * zoomScale_, 10.0f, 56.0f);
    const float startX = static_cast<float>(size.x) * 0.5f + scrollOffsetX_;
    const float startY = 90.0f + scrollOffsetY_;
    const float initialGap = std::max(90.0f, static_cast<float>(size.x) * 0.34f * zoomScale_);
    const float verticalGap = 95.0f * zoomScale_;

    const bool isCreateAnimation = (lastOperationMenuIndex_ == 0 && !currentSteps_.empty());
    const int revealStepIndex = stepTransitioning_ && stepTransitionProgress_ < 0.5f ? fromStep : toStep;
    std::unordered_set<std::string> visiblePrefixes;
    visiblePrefixes.insert("");
    if (isCreateAnimation) {
        std::string currentPrefix;
        const int applied = std::clamp(revealStepIndex, 0, static_cast<int>(currentSteps_.size()));
        for (int i = 0; i < applied; ++i) {
            const TrieInstruction& step = currentSteps_[i];
            if (step.trie_op == TrieOp::CREATE_NODE && step.character >= 'a' && step.character <= 'z') {
                visiblePrefixes.insert(currentPrefix + step.character);
            }
            else if (step.trie_op == TrieOp::MOVE_TO_NODE && step.character >= 'a' && step.character <= 'z') {
                currentPrefix.push_back(step.character);
                visiblePrefixes.insert(currentPrefix);
            }
            else if (step.trie_op == TrieOp::MARK_END) {
                currentPrefix.clear();
            }
        }
    }

    nodePositions_.clear();
    drawTrieNode(
        window,
        root,
        startX,
        startY,
        initialGap,
        font,
        "",
        activePath,
        activeInstruction,
        radius,
        verticalGap,
        isCreateAnimation ? &visiblePrefixes : nullptr
    );

    const auto findNodePosition = [&](std::string prefix) -> sf::Vector2f {
        while (true) {
            const auto it = nodePositions_.find(prefix);
            if (it != nodePositions_.end()) {
                return it->second;
            }
            if (prefix.empty()) {
                break;
            }
            prefix.pop_back();
        }
        return sf::Vector2f(startX, startY);
    };

    const sf::Vector2f fromPos = findNodePosition(fromPath);
    const sf::Vector2f toPos = findNodePosition(toPath);
    const sf::Vector2f markerPos(
        lerp(fromPos.x, toPos.x, transitionT),
        lerp(fromPos.y, toPos.y, transitionT)
    );

    sf::CircleShape ring(radius + 9.0f);
    ring.setOrigin(sf::Vector2f(radius + 9.0f, radius + 9.0f));
    ring.setPosition(markerPos);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineThickness(3.0f);
    ring.setOutlineColor(highlightRingColor_);
    window.draw(ring);

    if (previewRoot != nullptr) {
        trie.clearHelper(previewRoot);
    }
}

void TrieUI::drawTrieNode(
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
) {
    if (node == nullptr) {
        return;
    }

    nodePositions_[prefix] = sf::Vector2f(x, y);

    const bool isRoot = prefix.empty();
    const bool isOnPath = !activePath.empty() && activePath.rfind(prefix, 0) == 0;
    const bool isPathEnd = !activePath.empty() && prefix == activePath;

    sf::Color fill = nodeBaseColor_;
    if (node->is_end_of_word && !isRoot) {
        fill = secondaryNodeColor_;
    }
    if (isOnPath && !isRoot) {
        fill = sf::Color(96, 165, 250, 255);
    }
    if (isPathEnd && activeInstruction != nullptr) {
        switch (activeInstruction->trie_op) {
        case TrieOp::FOUND_WORD:
        case TrieOp::MARK_END:
            fill = secondaryNodeColor_;
            break;
        case TrieOp::NOT_FOUND:
        case TrieOp::RETURN_NULL:
            fill = deleteNodeColor_;
            break;
        case TrieOp::UNMARK_END:
            fill = deleteNodeColor_;
            break;
        case TrieOp::DELETE_PHYSICAL:
            fill = deleteNodeColor_;
            break;
        case TrieOp::RETURN_NODE:
            break;
        case TrieOp::CREATE_NODE:
            fill = sf::Color(96, 165, 250, 255);
            break;
        case TrieOp::MOVE_TO_NODE:
            fill = sf::Color(96, 165, 250, 255);
            break;
        }
    }

    sf::CircleShape circle(radius);
    circle.setOrigin(sf::Vector2f(radius, radius));
    circle.setPosition(sf::Vector2f(x, y));
    circle.setOutlineThickness(edgeThickness_ * zoomScale_);
    circle.setOutlineColor(isOnPath ? highlightRingColor_ : edgeColor_);
    circle.setFillColor(fill);
    window.draw(circle);

    if (!isRoot && font != nullptr && !prefix.empty()) {
        const char letter = prefix.back();
        sf::Text letterText(*font, std::string(1, letter), static_cast<unsigned int>(radius * 0.9f * fontScale_));
        letterText.setFillColor(valueTextColor_);
        const sf::FloatRect bounds = letterText.getLocalBounds();
        letterText.setOrigin(sf::Vector2f(bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f));
        letterText.setPosition(sf::Vector2f(x, y));
        window.draw(letterText);
    }

    struct VisibleChild {
        TrieNode* node = nullptr;
        std::string prefix;
        int leafSlots = 1;
    };

    std::vector<VisibleChild> visibleChildren;
    visibleChildren.reserve(26);
    int totalLeafSlots = 0;
    for (int i = 0; i < 26; ++i) {
        TrieNode* child = node->children[i];
        if (child == nullptr) {
            continue;
        }

        const std::string childPrefix = prefix + static_cast<char>('a' + i);
        if (visiblePrefixes != nullptr && visiblePrefixes->find(childPrefix) == visiblePrefixes->end()) {
            continue;
        }

        const int slots = countVisibleLeafSlots(child, childPrefix, visiblePrefixes);
        visibleChildren.push_back(VisibleChild{ child, childPrefix, std::max(1, slots) });
        totalLeafSlots += std::max(1, slots);
    }

    if (visibleChildren.empty()) {
        return;
    }

    const float nextY = y + verticalGap;
    const float slotWidth = std::max(radius * 2.2f, horizontalGap);
    float currentX = x - (slotWidth * static_cast<float>(std::max(0, totalLeafSlots - 1))) * 0.5f;

    for (const VisibleChild& childInfo : visibleChildren) {
        const float childCenterX = currentX + (slotWidth * static_cast<float>(childInfo.leafSlots - 1)) * 0.5f;
        const bool edgeOnPath = !activePath.empty() && activePath.rfind(childInfo.prefix, 0) == 0;
        sf::Vertex line[] = {
            sf::Vertex{sf::Vector2f(x, y + radius), edgeOnPath ? highlightRingColor_ : edgeColor_},
            sf::Vertex{sf::Vector2f(childCenterX, nextY - radius), edgeOnPath ? highlightRingColor_ : edgeColor_}
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);

        drawTrieNode(
            window,
            childInfo.node,
            childCenterX,
            nextY,
            std::max(radius * 2.0f, horizontalGap / 1.35f),
            font,
            childInfo.prefix,
            activePath,
            activeInstruction,
            radius,
            verticalGap,
            visiblePrefixes
        );

        currentX += slotWidth * static_cast<float>(childInfo.leafSlots);
    }
}

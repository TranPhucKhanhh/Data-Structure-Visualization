#pragma once

#include<algorithm>
#include<iostream>
#include<vector>
#include<string>
#include<chrono>
#include<random>

//Serve for UI
enum class TrieOp
{
    MOVE_TO_NODE,   //Move to the existing node
    CREATE_NODE,    //Create a new node for new character
    MARK_END,       //
    FOUND_WORD,     //
    NOT_FOUND,      //
    UNMARK_END,     //
    DELETE_PHYSICAL //Delete the node from the memory
};

struct TrieInstruction {
    TrieOp trie_op;
	char character; // The character involved in the operation, if applicable

	TrieInstruction(TrieOp _trie_op, char _char) : trie_op(_trie_op), character(_char) {}
	TrieInstruction(TrieOp _trie_op) : trie_op(_trie_op), character('\0') {} // For operations that don't involve a character
};

struct TrieNode
{
    //Lowercase English characters only
    TrieNode *children[26];
    bool is_end_of_word = false;

    TrieNode() : is_end_of_word(false)
    {
        for (int _i = 0; _i < 26; ++_i) children[_i] = nullptr;
    }
};

struct Trie {
private:
    TrieNode *root_node;

    //Helper functions
    bool isEmpty(TrieNode *_node);
//    bool deleteHelper(TrieNode *current, std::string word, int index);
    bool deleteHelperStep(TrieNode *current, std::string word, int index, std::vector<TrieInstruction> &step);

public:
    Trie();
    ~Trie();

    void clearHelper(TrieNode *&root_node);
    void clear();

    void initFromKeyboard(); // Use for debug

	TrieNode* getRoot() { return root_node; }

    //The Do All At Once functions
//    void initFromList(std::vector<std::string> &_word_list);
//    void initFromFile(const std::string &_file_path);
//    void insertWord(const std::string& _word);
//    bool searchWord(const std::string& _word);
//    void deleteWord(const std::string& _word);
//    void updateWord(const std::string& _old_word, const std::string& _new_word);

    //The Step by Step functions
    std::vector<TrieInstruction> initFromListStep(std::vector<std::string> &_word_list);
    std::vector<TrieInstruction> initFromFileStep(const std::string &_file_path);
    std::vector<TrieInstruction> insertWordStep(const std::string& _word);
    std::vector<TrieInstruction> searchWordStep(const std::string& _word);
    std::vector<TrieInstruction> deleteWordStep(const std::string& _word);
    std::vector<TrieInstruction> updateWordStep(const std::string& _old_word, const std::string& _new_word);

    //Logic functions transferred from UI file
    std::vector<std::string> generateRandomWords(int count, int minLength, int maxLength);
    TrieNode* cloneTrieNode(const TrieNode* source);
    std::string sanitizeWord(const std::string& raw);
    std::vector<std::string> parseWordList(const std::string& raw);
    void applyStepsToPreviewTrie(TrieNode* root, const std::vector<TrieInstruction>& steps, int appliedCount);
};

inline Trie trie;

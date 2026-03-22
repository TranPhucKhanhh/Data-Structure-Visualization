#pragma once

#include<iostream>
#include<vector>
#include<string>

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
    bool _isEmpty(TrieNode *_node);
    void _clear(TrieNode *&root_node);
    bool _deleteHelper(TrieNode *_current, std::string _word, int _index);
    bool _deleteHelperStep(TrieNode *_current, std::string _word, int _index, std::vector<TrieInstruction> &_step);

public:
    Trie();
    ~Trie();

    void initFromKeyboard(); // Use for debug

	TrieNode* getRoot() { return root_node; }

    //The Do All At Once functions
    void initFromList(std::vector<std::string> &word_list);
	void initFromFile(std::string file_path); // Implement latter
    void insertWord(const std::string& word);
    bool searchWord(const std::string& word);
    void deleteWord(const std::string& word);
    void updateWord(const std::string& old_word, const std::string& new_word);

    //The Step by Step functions
    std::vector<TrieInstruction> initFromListStep(std::vector<std::string> &word_list);
    std::vector<TrieInstruction> initFromFileStep(std::string file_path); // Implement latter
    std::vector<TrieInstruction> insertWordStep(const std::string& word);
    std::vector<TrieInstruction> searchWordStep(const std::string& word);
    std::vector<TrieInstruction> deleteWordStep(const std::string& word);
    std::vector<TrieInstruction> updateWordStep(const std::string& old_word, const std::string& new_word);
};

inline Trie trie;

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

    //The Do All At Once functions
    void initFromList(std::vector<std::string> word_list);
	void initFromFile(std::string file_path); // Implement latter
    void insertWord(std::string word);
    bool searchWord(std::string word);
    void deleteWord(std::string word);
    void updateWord(std::string old_word, std::string new_word);
	void clearTrie();

    //The Step by Step functions
    std::vector<TrieInstruction> initFromListStep(std::vector<std::string> word_list);
    std::vector<TrieInstruction> initFromFileStep(std::string file_path); // Implement latter
    std::vector<TrieInstruction> insertWordStep(std::string word);
    std::vector<TrieInstruction> searchWordStep(std::string word);
    std::vector<TrieInstruction> deleteWordStep(std::string word);
    std::vector<TrieInstruction> updateWordStep(std::string old_word, std::string new_word);
};

inline Trie trie;

#pragma once

#include<vector>
#include<string>

//Serve for UI
enum class TrieOp
{
    MOVE_TO_NODE,   //Move to the existing node
    CREATE_NODE,    //Create a new node for new character
    MARK_END,       //
    VISIT_NODE,     //Traverse through node
    FOUND_WORD,     //
    NOT_FOUND,      //
    UNMARK_END,     //
    DELETE_PHYSICAL //Delete the node from the memory
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
    bool _deleteHelperStep(TrieNode *_current, std::string _word, int _index, std::vector<TrieOp> &_step);

public:
    Trie();
    ~Trie();

    void initFromKeyboard();

    //The Do All At Once functions
    void insertWord(std::string word);
    bool searchWord(std::string word);
    void deleteWord(std::string word);
    void updateWord(std::string old_word, std::string new_word);

    //The Step by Step functions
    std::vector<TrieOp> insertWordStep(std::string word);
    std::vector<TrieOp> searchWordStep(std::string word);
    std::vector<TrieOp> deleteWordStep(std::string word);
    std::vector<TrieOp> updateWordStep(std::string old_word, std::string new_word);
};

inline Trie trie;

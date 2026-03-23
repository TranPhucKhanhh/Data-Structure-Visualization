#include <logic/trie.h>

///-----------------------------------
///CONSTRUCTOR AND DESTRUCTOR
///-----------------------------------

Trie::Trie()
{
    root_node = new TrieNode();
}

Trie::~Trie()
{
    _clear(root_node);
}

//Helper function to clear the trie

void Trie::_clear(TrieNode*& root_node)
{
    if (root_node == nullptr) return;

    for (int i = 0; i < 26; ++i) _clear(root_node->children[i]);

    delete root_node;

    root_node = nullptr;
}

///-----------------------------------
///INITIALIZER
///-----------------------------------

//The instruction line is //-ed
void Trie::initFromKeyboard()
{
    int _num_words;
    //    std::cout << "Enter the number of words: ";
    std::cin >> _num_words;
    for (int _i = 0; _i < _num_words; ++_i)
    {
        std::string _temp_word;
        std::cout << "Enter the " << _i << "-th word: ";
        std::cin >> _temp_word;
        insertWord(_temp_word);
    }
}

void Trie::initFromList(std::vector<std::string>& word_list)
{
    for (const std::string& _word : word_list) insertWord(_word);
}

void Trie::initFromFile(const std::string& file_path)
{
    std::ifstream _inp(file_path);

    if (!_inp.is_open())
    {
        std::cerr << "Error: Cannot open the file at directory " << file_path << "!\n";
        return;
    }

    std::string _word;
    while (_inp >> _word) insertWord(_word);

    _inp.close();
}

// Step by step initialization
std::vector<TrieInstruction> Trie::initFromListStep(std::vector<std::string>& word_list)
{
    std::vector<TrieInstruction> _steps;
    for (const std::string& _word : word_list) {
        std::vector<TrieInstruction> _step_for_word = insertWordStep(_word);
        _steps.insert(_steps.end(), _step_for_word.begin(), _step_for_word.end());
    }
    return _steps;
}

std::vector<TrieInstruction> Trie::initFromFileStep(const std::string& file_path)
{
    std::vector<TrieInstruction> _total_steps;
    std::ifstream _inp(file_path);

    if (!_inp.is_open())
    {
        std::cerr << "Error: Cannot open the file to insert step by step!\n";
        return _total_steps;
    }

    std::string _word;
    while (_inp >> _word)
    {
        std::vector<TrieInstruction> _word_steps = insertWordStep(_word);

        _total_steps.insert(_total_steps.end(), _word_steps.begin(), _word_steps.end());
    }

    _inp.close();
    return _total_steps;
}



///-----------------------------------
///WORD INSERTION
///-----------------------------------

//DAAT insertion
void Trie::insertWord(const std::string& word)
{
    TrieNode* _current_node = root_node;

    for (char _c : word)
    {
        int _idx = _c - 'a';
        if (_current_node->children[_idx] == nullptr)
        {
            _current_node->children[_idx] = new TrieNode();
        }

        _current_node = _current_node->children[_idx];
    }

    _current_node->is_end_of_word = true;
}

//Step-by-step insertion

std::vector<TrieInstruction> Trie::insertWordStep(const std::string& word)
{
    std::vector<TrieInstruction> _steps;
    TrieNode* _current_node = root_node;

    for (char _c : word)
    {
        int _idx = _c - 'a';
        if (_current_node->children[_idx] == nullptr)
        {
            _current_node->children[_idx] = new TrieNode();
            _steps.push_back(TrieInstruction(TrieOp::CREATE_NODE, _c));
        }
        else _steps.push_back(TrieInstruction(TrieOp::MOVE_TO_NODE, _c));

        _current_node = _current_node->children[_idx];
    }

    _current_node->is_end_of_word = true;
    _steps.push_back(TrieInstruction(TrieOp::MARK_END));

    return _steps;
}

///-----------------------------------
///WORD SEARCHING
///-----------------------------------

//DAAT search
bool Trie::searchWord(const std::string& word)
{
    TrieNode* _current_node = root_node;

    for (char _c : word)
    {
        int _idx = _c - 'a';
        if (_current_node->children[_idx] == nullptr)
        {
            //Word not found
            return false;
        }
        _current_node = _current_node->children[_idx];
    }

    return _current_node->is_end_of_word;
}

//Step-by-step search
std::vector<TrieInstruction> Trie::searchWordStep(const std::string& word)
{
    std::vector<TrieInstruction> _steps;
    TrieNode* _current_node = root_node;

    for (char _c : word)
    {
        int _idx = _c - 'a';
        if (_current_node->children[_idx] == nullptr)
        {
            //Word not found
            _steps.push_back(TrieInstruction(TrieOp::NOT_FOUND));
            return _steps;
        }
        else _steps.push_back(TrieInstruction(TrieOp::MOVE_TO_NODE, _c));
        _current_node = _current_node->children[_idx];
    }

    if (_current_node->is_end_of_word) _steps.push_back(TrieInstruction(TrieOp::FOUND_WORD));
    else _steps.push_back(TrieInstruction(TrieOp::NOT_FOUND));

    return _steps;
}

///-----------------------------------
///WORD DELETION
///-----------------------------------

//DAAT deletion
void Trie::deleteWord(const std::string& word)
{
    if (root_node == nullptr || word.empty()) return;
    _deleteHelper(root_node, word, 0);
}

bool Trie::_isEmpty(TrieNode* _node)
{
    for (int _i = 0; _i < 26; ++_i)
    {
        if (_node->children[_i] != nullptr) return false;
    }
    return true;
}

/*
Case 1: The word is fully traversed
Case 2: The path does not exist
Case 3: The path does exist -> traverse down
*/

bool Trie::_deleteHelper(TrieNode* _current, std::string _word, int _index)
{
    if (_index == _word.size())
    {
        _current->is_end_of_word = false;
        return _isEmpty(_current);
    }

    char _char_to_find = _word[_index];
    int _idx = _char_to_find - 'a';
    if (_current->children[_idx] == nullptr)
    {
        //Word not found
        return false;
    }

    bool _can_delete = _deleteHelper(_current->children[_idx], _word, _index + 1);

    if (_can_delete)
    {
        delete _current->children[_idx];
        _current->children[_idx] = nullptr;

        return !_current->is_end_of_word && _isEmpty(_current);
    }

    return false;
}

//Step-by-step deletion

std::vector<TrieInstruction> Trie::deleteWordStep(const std::string& word)
{
    std::vector<TrieInstruction> _steps;
    if (root_node == nullptr || word.empty())
    {
        _steps.push_back(TrieInstruction(TrieOp::NOT_FOUND));
        return _steps;
    }

    _deleteHelperStep(root_node, word, 0, _steps);
    return _steps;
}

bool Trie::_deleteHelperStep(TrieNode* _current, std::string _word, int _index, std::vector<TrieInstruction>& _steps)
{
    if (_index == _word.size())
    {
        if (_current->is_end_of_word)
        {
            _current->is_end_of_word = false;
            _steps.push_back(TrieInstruction(TrieOp::UNMARK_END));
        }
        else {
            _steps.push_back(TrieInstruction(TrieOp::NOT_FOUND));
            return false;
        }
        return _isEmpty(_current);
    }

    char _char_to_find = _word[_index];
    int _idx = _char_to_find - 'a';
    if (_current->children[_idx] == nullptr)
    {
        //Word not found
        _steps.push_back(TrieInstruction(TrieOp::NOT_FOUND));
        return false;
    }

    _steps.push_back(TrieInstruction(TrieOp::MOVE_TO_NODE, _char_to_find));
    bool _can_delete = _deleteHelperStep(_current->children[_idx], _word, _index + 1, _steps);

    if (_can_delete)
    {
        _steps.push_back(TrieInstruction(TrieOp::DELETE_PHYSICAL));

        delete _current->children[_idx];
        _current->children[_idx] = nullptr;

        return !_current->is_end_of_word && _isEmpty(_current);
    }

    return false;
}

///-----------------------------------
///WORD UPDATE
///-----------------------------------

//DAAT update
void Trie::updateWord(const std::string& old_word, const std::string& new_word)
{
    deleteWord(old_word);
    insertWord(new_word);
}

//Step-by-step update
std::vector<TrieInstruction> Trie::updateWordStep(const std::string& old_word, const std::string& new_word)
{
    std::vector<TrieInstruction> _all_steps;

    std::vector<TrieInstruction> _delete_steps = deleteWordStep(old_word);
    std::vector<TrieInstruction> _insert_steps = insertWordStep(new_word);

    _all_steps.insert(_all_steps.end(), _delete_steps.begin(), _delete_steps.end());
    _all_steps.insert(_all_steps.end(), _insert_steps.begin(), _insert_steps.end());

    return _all_steps;
}

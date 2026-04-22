#include <logic/trie.h>

#include <cctype>
#include <fstream>
#include <sstream>

namespace {
    std::vector<std::string> extractWords(const std::string& _content)
    {
        std::vector<std::string> _words;
        std::string _token;

        for (char _c: _content) {
            if (std::isalpha(static_cast<unsigned char>(_c)) != 0) {
                _token.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(_c))));
            }
            else if (!_token.empty()) {
                _words.push_back(_token);
                _token.clear();
            }
        }

        if (!_token.empty()) {
            _words.push_back(_token);
        }

        return _words;
    }
}

///-----------------------------------
///CONSTRUCTOR AND DESTRUCTOR
///-----------------------------------

Trie::Trie()
{
    root_node = new TrieNode();
}

Trie::~Trie()
{
    clearHelper(root_node);
}

void Trie::clearHelper(TrieNode *&root_node)
{
    if (root_node == nullptr) return;

    for (int _i = 0; _i < 26; ++_i) clearHelper(root_node->children[_i]);
    delete root_node;

    root_node = nullptr;
}

void Trie::clear()
{
    clearHelper(root_node);
    root_node = new TrieNode();
}

///-----------------------------------
///INITIALIZER
///-----------------------------------

//The instruction line is //-ed
void Trie::initFromKeyboard()
{
    int _num_words;
    //std::cout << "Enter the number of words: ";
    std::cin >> _num_words;
    for (int _i = 0; _i < _num_words; ++_i)
    {
        std::string _temp_word;
        std::cout << "Enter the " << _i << "-th word: ";
        std::cin >> _temp_word;
        insertWordStep(_temp_word);
    }
}

//void Trie::initFromList(std::vector<std::string>& word_list)
//{
//    clear();
//    for (const std::string& _word : word_list) insertWord(_word);
//}
//
//void Trie::initFromFile(const std::string& file_path)
//{
//    std::ifstream file(file_path);
//    if (!file.is_open()) {
//        return;
//    }
//
//    std::ostringstream buffer;
//    buffer << file.rdbuf();
//    std::vector<std::string> words = extractWords(buffer.str());
//    initFromList(words);
//}

// Step by step initialization
std::vector<TrieInstruction> Trie::initFromListStep(std::vector<std::string>& word_list)
{
    std::vector<TrieInstruction> _steps;
    clear();
    for (const std::string& _word : word_list) {
        std::vector<TrieInstruction> _step_for_word = insertWordStep(_word);
        _steps.insert(_steps.end(), _step_for_word.begin(), _step_for_word.end());
    }
    return _steps;
}

std::vector<TrieInstruction> Trie::initFromFileStep(const std::string& file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return {};
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::vector<std::string> words = extractWords(buffer.str());
    return initFromListStep(words);
}

///-----------------------------------
///WORD INSERTION
///-----------------------------------

//DAAT insertion
//void Trie::insertWord(const std::string& _word)
//{
//    TrieNode* _current_node = root_node;
//
//    for (char _c : _word)
//    {
//        int _idx = _c - 'a';
//        if (_current_node->children[_idx] == nullptr)
//        {
//            _current_node->children[_idx] = new TrieNode();
//        }
//
//        _current_node = _current_node->children[_idx];
//    }
//
//    _current_node->is_end_of_word = true;
//}

//Step-by-step insertion

std::vector<TrieInstruction> Trie::insertWordStep(const std::string& _word)
{
    std::vector<TrieInstruction> _steps;
    TrieNode* _current_node = root_node;

    for (char _c : _word)
    {
        int _idx = _c - 'a';
        if (_current_node->children[_idx] == nullptr)
        {
            _current_node->children[_idx] = new TrieNode();
            _steps.push_back(TrieInstruction(TrieOp::CREATE_NODE, _c));
        }

        _steps.push_back(TrieInstruction(TrieOp::MOVE_TO_NODE, _c));

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
//bool Trie::searchWord(const std::string& _word)
//{
//    TrieNode* _current_node = root_node;
//
//    for (char _c : _word)
//    {
//        int _idx = _c - 'a';
//        if (_current_node->children[_idx] == nullptr)
//        {
//            //Word not found
//            return false;
//        }
//        _current_node = _current_node->children[_idx];
//    }
//
//    return _current_node->is_end_of_word;
//}

//Step-by-step search
std::vector<TrieInstruction> Trie::searchWordStep(const std::string& _word)
{
    std::vector<TrieInstruction> _steps;
    TrieNode* _current_node = root_node;

    for (char _c : _word)
    {
        int _idx = _c - 'a';
        if (_current_node->children[_idx] == nullptr)
        {
            //Word not found
            _steps.push_back(TrieInstruction(TrieOp::NOT_FOUND));
            return _steps;
        }

        _steps.push_back(TrieInstruction(TrieOp::MOVE_TO_NODE, _c));
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
//void Trie::deleteWord(const std::string& _word)
//{
//    if (root_node == nullptr || _word.empty()) return;
//    _deleteHelper(root_node, _word, 0);
//}

bool Trie::isEmpty(TrieNode* _node)
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

//bool Trie::deleteHelper(TrieNode* _current, std::string _word, int _index)
//{
//    if (_index == _word.size())
//    {
//        _current->is_end_of_word = false;
//        return _isEmpty(_current);
//    }
//
//    char _char_to_find = _word[_index];
//    int _idx = _char_to_find - 'a';
//    if (_current->children[_idx] == nullptr)
//    {
//        //Word not found
//        return false;
//    }
//
//    bool _can_delete = _deleteHelper(_current->children[_idx], _word, _index + 1);
//
//    if (_can_delete)
//    {
//        delete _current->children[_idx];
//        _current->children[_idx] = nullptr;
//
//        return !_current->is_end_of_word && _isEmpty(_current);
//    }
//
//    return false;
//}

//Step-by-step deletion

std::vector<TrieInstruction> Trie::deleteWordStep(const std::string& _word)
{
    std::vector<TrieInstruction> _steps;
    if (root_node == nullptr || _word.empty())
    {
        _steps.push_back(TrieInstruction(TrieOp::NOT_FOUND));
        return _steps;
    }

    deleteHelperStep(root_node, _word, 0, _steps);
    return _steps;
}

bool Trie::deleteHelperStep(TrieNode* _current, std::string _word, int _index, std::vector<TrieInstruction>& _steps)
{
    if (_index == _word.size())
    {
        if (_current->is_end_of_word)
        {
            _current->is_end_of_word = false;
            _steps.push_back(TrieInstruction(TrieOp::UNMARK_END));
        }
        else {
            _steps.push_back(TrieInstruction(TrieOp::RETURN_NODE));
            return false;
        }
        const bool can_delete_current = isEmpty(_current);
        if (!can_delete_current) {
            _steps.push_back(TrieInstruction(TrieOp::RETURN_NODE));
        }
        return can_delete_current;
    }

    char _char_to_find = _word[_index];
    int _idx = _char_to_find - 'a';
    if (_current->children[_idx] == nullptr)
    {
        //Word not found
        _steps.push_back(TrieInstruction(TrieOp::RETURN_NULL));
        return false;
    }

    _steps.push_back(TrieInstruction(TrieOp::MOVE_TO_NODE, _char_to_find));
    bool _can_delete = deleteHelperStep(_current->children[_idx], _word, _index + 1, _steps);

    if (_can_delete)
    {
        _steps.push_back(TrieInstruction(TrieOp::DELETE_PHYSICAL));

        delete _current->children[_idx];
        _current->children[_idx] = nullptr;

        const bool can_delete_current = !_current->is_end_of_word && isEmpty(_current);
        if (!can_delete_current) {
            _steps.push_back(TrieInstruction(TrieOp::RETURN_NODE, _char_to_find));
        }
        return can_delete_current;
    }

    _steps.push_back(TrieInstruction(TrieOp::RETURN_NODE, _char_to_find));
    return false;
}

///-----------------------------------
///WORD UPDATE
///-----------------------------------

//DAAT update
//void Trie::updateWord(const std::string& _old_word, const std::string& _new_word)
//{
//    deleteWord(_old_word);
//    insertWord(_new_word);
//}

//Step-by-step update
std::vector<TrieInstruction> Trie::updateWordStep(const std::string& _old_word, const std::string& _new_word)
{
    std::vector<TrieInstruction> _all_steps;

    std::vector<TrieInstruction> _delete_steps = deleteWordStep(_old_word);
    _all_steps.insert(_all_steps.end(), _delete_steps.begin(), _delete_steps.end());

    bool old_word_was_found = false;
    for (const TrieInstruction& _step : _delete_steps) {
        if (_step.trie_op == TrieOp::UNMARK_END) {
            old_word_was_found = true;
            break;
        }
    }

    if (!old_word_was_found) {
        return _all_steps;
    }

    std::vector<TrieInstruction> _insert_steps = insertWordStep(_new_word);
    _all_steps.insert(_all_steps.end(), _insert_steps.begin(), _insert_steps.end());

    return _all_steps;
}

//Logic functions transferred from UI file

std::vector<std::string> Trie::generateRandomWords(int count, int minLength, int maxLength) {
    std::vector<std::string> words;
    count = std::max(0, count);
    minLength = std::max(1, minLength);
    maxLength = std::max(minLength, maxLength);

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> lenDist(minLength, maxLength);
    std::uniform_int_distribution<int> charDist(0, 25);

    words.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const int len = lenDist(rng);
        std::string word;
        word.reserve(static_cast<std::size_t>(len));
        for (int j = 0; j < len; ++j) {
            word.push_back(static_cast<char>('a' + charDist(rng)));
        }
        words.push_back(std::move(word));
    }

    return words;
}

TrieNode* Trie::cloneTrieNode(const TrieNode* source)
{
    if (source == nullptr) {
        return nullptr;
    }
    TrieNode* node = new TrieNode();
    node->is_end_of_word = source->is_end_of_word;
    for (int i = 0; i < 26; ++i) {
        node->children[i] = cloneTrieNode(source->children[i]);
    }
    return node;
}

std::string Trie::sanitizeWord(const std::string& raw) {
    std::string out;
    out.reserve(raw.size());
    for (char c : raw) {
        if (std::isalpha(static_cast<unsigned char>(c)) != 0) {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    return out;
}

std::vector<std::string> Trie::parseWordList(const std::string& raw) {
    std::vector<std::string> words;
    std::string token;
    std::stringstream ss(raw);
    while (std::getline(ss, token, ',')) {
        std::string clean = sanitizeWord(token);
        if (!clean.empty()) {
            words.push_back(clean);
        }
    }
    return words;
}

void Trie::applyStepsToPreviewTrie(TrieNode* root, const std::vector<TrieInstruction>& steps, int appliedCount) {
    if (root == nullptr) {
        return;
    }

    TrieNode* current = root;
    std::string path;
    bool resetBeforeNextTraversal = false;
    const int count = std::clamp(appliedCount, 0, static_cast<int>(steps.size()));

    for (int i = 0; i < count; ++i) {
        const TrieInstruction& step = steps[i];
        switch (step.trie_op) {
        case TrieOp::CREATE_NODE: {
            if (step.character < 'a' || step.character > 'z') {
                continue;
            }
            if (resetBeforeNextTraversal) {
                current = root;
                path.clear();
                resetBeforeNextTraversal = false;
            }
            const int idx = step.character - 'a';
            if (current->children[idx] == nullptr) {
                current->children[idx] = new TrieNode();
            }
            break;
        }
        case TrieOp::MOVE_TO_NODE: {
            if (step.character < 'a' || step.character > 'z') {
                continue;
            }
            if (resetBeforeNextTraversal) {
                current = root;
                path.clear();
                resetBeforeNextTraversal = false;
            }

            const int idx = step.character - 'a';
            if (current->children[idx] != nullptr) {
                current = current->children[idx];
                path.push_back(step.character);
            }
            else if (root->children[idx] != nullptr) {
                current = root->children[idx];
                path.clear();
                path.push_back(step.character);
            }
            break;
        }
        case TrieOp::MARK_END:
            current->is_end_of_word = true;
            current = root;
            path.clear();
            break;
        case TrieOp::UNMARK_END:
            current->is_end_of_word = false;
            resetBeforeNextTraversal = true;
            break;
        case TrieOp::DELETE_PHYSICAL:
            resetBeforeNextTraversal = true;
            if (!path.empty()) {
                const char erased = path.back();
                path.pop_back();

                TrieNode* parent = root;
                bool validParent = true;
                for (char c : path) {
                    const int idx = c - 'a';
                    if (idx < 0 || idx >= 26 || parent->children[idx] == nullptr) {
                        validParent = false;
                        break;
                    }
                    parent = parent->children[idx];
                }

                if (validParent) {
                    const int erasedIdx = erased - 'a';
                    if (erasedIdx >= 0 && erasedIdx < 26 && parent->children[erasedIdx] != nullptr) {
                        clearHelper(parent->children[erasedIdx]);
                        parent->children[erasedIdx] = nullptr;
                    }
                    current = parent;
                }
            }
            break;
        case TrieOp::FOUND_WORD:
        case TrieOp::NOT_FOUND:
            current = root;
            path.clear();
            break;
        case TrieOp::RETURN_NULL:
            break;
        case TrieOp::RETURN_NODE:
            break;
        }
    }
}

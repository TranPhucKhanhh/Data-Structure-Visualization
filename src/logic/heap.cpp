#include <logic/heap.h>
#include <fstream>
#include <cstdlib>
#include <ctime>

///-----------------------------------
/// Heap helper functions
///-----------------------------------
bool Heap::compareIndex(const int & a, const int& b) const {
	if (heap_type == HeapType::MinHeap) {
		return heap[a] <= heap[b];
	}
	else {
		return heap[a] >= heap[b];
	}
}

void Heap::heapifyUp(int id) {
	while (id != 0 && !compareIndex(getParent(id), id)) {
		std::swap(heap[id], heap[getParent(id)]);
		id = getParent(id);
	}
}

void Heap::heapifyDown(int id) {
	while (true) {
		int left = 2 * id + 1;
		int right = 2 * id + 2;
		int target = id;
		if (left < heap.size() && !compareIndex(target, left)) {
			target = left;
		}
		if (right < heap.size() && !compareIndex(target, right)) {
			target = right;
		}
		if (target == id) {
			break;
		}
		std::swap(heap[id], heap[target]);
		id = target;
	}
}

std::vector<HeapInstruction> Heap::heapifyUpStep(int id) {
	std::vector<HeapInstruction> instructions;
	while (id != 0 && !compareIndex(getParent(id), id)) {
		instructions.push_back(HeapInstruction(HeapOp::SwapParent, getParent(id)));
		std::swap(heap[id], heap[getParent(id)]);
		id = getParent(id);
	}
	return instructions;
}

std::vector<HeapInstruction> Heap::heapifyDownStep(int id) {
	std::vector<HeapInstruction> instructions;
	while (true) {
		int left = 2 * id + 1;
		int right = 2 * id + 2;
		int target = id;
		if (left < heap.size() && !compareIndex(target, left)) {
			target = left;
		}
		if (right < heap.size() && !compareIndex(target, right)) {
			target = right;
		}
		if (target == id) {
			instructions.push_back(HeapInstruction(HeapOp::HeapifyDownDone, id));
			break;
		}
		instructions.push_back(HeapInstruction(target == left ? HeapOp::SwapLeftChild : HeapOp::SwapRightChild, target));
		std::swap(heap[id], heap[target]);
		id = target;
	}
	return instructions;
}

///-----------------------------------
///CONSTRUCTOR AND DESTRUCTOR
///-----------------------------------

Heap::Heap()
{
	// The default heap type is MaxHeap
	heap_type = HeapType::MaxHeap;
}

Heap::~Heap()
{
	heap.clear();
}

///-----------------------------------
///Do all at once functions
///-----------------------------------

void Heap::initFromList(const std::vector<int>& list)
{
	heap = list;
	for (int i = ((int) heap.size() / 2) - 1; i >= 0; --i) {
		heapifyDown(i);
	}
}

void Heap::initFromFile(const std::string& file_path)
{
	std::ifstream file(file_path);
	if (!file) {
		throw std::runtime_error("Cannot open file");
	}

	std::vector<int> values;
	int x;

	while (file >> x) {
		values.push_back(x);
	}

	initFromList(values);
}

void Heap::initRandom(const int &num) {
    std::vector<int> vc;
    int max_value = 100;
    int min_value = 0;

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    for (int i = 0; i < num; i++) {
        vc.push_back(min_value + (std::rand() % (max_value - min_value + 1)));
    }

    initFromList(vc);
}

void Heap::insertValue(const int& val) {
	heap.push_back(val);
	heapifyUp(((int) heap.size()) - 1);
}

bool Heap::searchValue(const int& val) {
	for (const int& x : heap) {
		if (x == val) {
			return true;
		}
	}
	return false;
}

void Heap::deleteTop() {
	if (heap.empty()) {
		return;
	}
	heap[0] = heap.back();
	heap.pop_back();
	if (!heap.empty()) {
		heapifyDown(0);
	}
}

void Heap::updateValue(const int& old_value, const int& new_value) {
	for (int i = 0; i < heap.size(); ++i) {
		if (heap[i] == old_value) {
			heap[i] = new_value;
			int parent = getParent(i);

			if (i > 0 && !compareIndex(parent, i)) {
				heapifyUp(i);
			}
			else {
				heapifyDown(i);
			}

			return;
		}
	}
}

///-----------------------------------
/// Step by step functions
///-----------------------------------

std::vector<HeapInstruction> Heap::initFromListStep(const std::vector<int>& list)
{
	std::vector<HeapInstruction> instructions;
	heap = list;
	for (int i = ((int) heap.size() / 2) - 1; i >= 0; --i) {
		instructions.push_back(HeapInstruction(HeapOp::VisitStraight, i));
		std::vector<HeapInstruction> tmp = heapifyDownStep(i);
		instructions.insert(std::end(instructions), std::begin(tmp), std::end(tmp));
	}
	instructions.push_back(HeapInstruction(HeapOp::ReturnHeap));
	return instructions;
}

std::vector<HeapInstruction> Heap::initFromFileStep(const std::string& file_path)
{
	std::ifstream file(file_path);
	if (!file) {
		throw std::runtime_error("Cannot open file");
	}
	std::vector<int> values;
	int x;
	while (file >> x) {
		values.push_back(x);
	}
	return initFromListStep(values);
}

std::vector<HeapInstruction> Heap::initRandomStep(const int &num)
{
    std::vector<int> values;

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> valueDist(0, 99);

    for (int i = 0; i < num; ++i) values.push_back(valueDist(rng));

    return initFromListStep(values);
}

std::vector<HeapInstruction> Heap::insertValueStep(const int& val)
{
	std::vector<HeapInstruction> instructions;
	heap.push_back(val);
	instructions.push_back(HeapInstruction(HeapOp::AddBackValue, val));
	std::vector<HeapInstruction> tmp = heapifyUpStep(((int) heap.size()) - 1);
	instructions.insert(std::end(instructions), std::begin(tmp), std::end(tmp));
	instructions.push_back(HeapInstruction(HeapOp::ReturnHeap));
	return instructions;
}

std::vector<HeapInstruction> Heap::deleteTopStep()
{
	std::vector<HeapInstruction> instructions;
	if (heap.empty()) {
		return instructions;
	}
	heap[0] = heap.back();
	heap.pop_back();
	instructions.push_back(HeapInstruction(HeapOp::MoveBackToTop));
	if (!heap.empty()) {
		std::vector<HeapInstruction> tmp = heapifyDownStep(0);
		instructions.insert(std::end(instructions), std::begin(tmp), std::end(tmp));
	}
	instructions.push_back(HeapInstruction(HeapOp::ReturnHeap));
	return instructions;
}

std::vector<HeapInstruction> Heap::updateValueStep(const int& old_value, const int& new_value)
{
	std::vector<HeapInstruction> instructions;
	for (int i = 0; i < heap.size(); ++i) {
		instructions.push_back(HeapInstruction(HeapOp::VisitStraight, i));
		if (heap[i] == old_value) {
			heap[i] = new_value;
			instructions.push_back(HeapInstruction(HeapOp::UpdateValue, new_value));
			std::vector<HeapInstruction> tmp;

			int parent = getParent(i);

			if (i > 0 && !compareIndex(parent, i)) {
				tmp = heapifyUpStep(i);
			}
			else {
				tmp = heapifyDownStep(i);
			}

			instructions.insert(std::end(instructions), std::begin(tmp), std::end(tmp));
			instructions.push_back(HeapInstruction(HeapOp::ReturnHeap));
			return instructions;
		}
	}
	instructions.push_back(HeapInstruction(HeapOp::NotFound));
	return instructions;
}

std::vector<HeapInstruction> Heap::searchValueStep(const int& val)
{
	std::vector<HeapInstruction> instructions;
	for (int i = 0; i < heap.size(); ++i) {
		const int& x = heap[i];
		instructions.push_back(HeapInstruction(HeapOp::VisitStraight, i));
		if (x == val) {
			instructions.push_back(HeapInstruction(HeapOp::FoundValue));
			return instructions;
		}
	}
	instructions.push_back(HeapInstruction(HeapOp::NotFound));
	return instructions;
}

//Split function from the old rebuildViewFromStep function in the UI file, to pull the logic feature from the old function to the logic file.
void Heap::applyInstructions(std::vector<int> &state, const HeapInstruction &instruction, int &cursor_idx)
{
    switch(instruction.heap_op)
    {
    case HeapOp::AddBackValue:
        {
            state.push_back(instruction.data);
            cursor_idx = (int)state.size() - 1;
            break;
        }
    case HeapOp::SwapParent:
    case HeapOp::SwapLeftChild:
    case HeapOp::SwapRightChild:
        {
            int _target_index = instruction.data;
            if (cursor_idx >= 0 && cursor_idx < state.size() &&
                _target_index >= 0 && _target_index < state.size())
            {
                std::swap(state[cursor_idx], state[_target_index]);
                cursor_idx = _target_index;
            }
            break;
        }
    case HeapOp::MoveBackToTop:
        {
            if (!state.empty())
            {
                state[0] = state.back();
                state.pop_back();
                cursor_idx = (state.empty()) ? -1 : 0;
            }
            break;
        }
    case HeapOp::VisitStraight:
        {
            cursor_idx = instruction.data;
            break;
        }
    case HeapOp::HeapifyDownDone:
        {
            cursor_idx = instruction.data;
            break;
        }
    case HeapOp::ReturnHeap:
        {
            break;
        }
    case HeapOp::UpdateValue:
        {
            if (cursor_idx >= 0 && cursor_idx < state.size())
            {
                state[cursor_idx] = instruction.data;
            }
            break;
        }
    default: break;
    }
}

std::vector<int> Heap::parseIntegers(const std::string& raw) const {
	std::vector<int> _values;
	std::stringstream _ss(raw);
	std::string _token;
	while (std::getline(_ss, _token, ',')) {
		if (_token.empty()) {
			continue;
		}
		try {
			_values.push_back(std::stoi(_token));
		}
		catch (...) {
		}
	}
	return _values;
}


#include <logic/heap.h>
#include <fstream>

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
		std::vector<HeapInstruction> tmp = heapifyDownStep(i);
		instructions.insert(std::end(instructions), std::begin(tmp), std::end(tmp));
	}
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

std::vector<HeapInstruction> Heap::insertValueStep(const int& val)
{
	std::vector<HeapInstruction> instructions;
	heap.push_back(val);
	instructions.push_back(HeapInstruction(HeapOp::AddBackValue, val));
	std::vector<HeapInstruction> tmp = heapifyUpStep(((int) heap.size()) - 1);
	instructions.insert(std::end(instructions), std::begin(tmp), std::end(tmp));
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
	return instructions;
}

std::vector<HeapInstruction> Heap::updateValueStep(const int& old_value, const int& new_value)
{
	std::vector<HeapInstruction> instructions;
	for (int i = 0; i < heap.size(); ++i) {
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
			return instructions;
		}
		instructions.push_back(HeapInstruction(HeapOp::VisitStraight));
	}
	return instructions;
}

std::vector<HeapInstruction> Heap::searchValueStep(const int& val)
{
	std::vector<HeapInstruction> instructions;
	for (const int& x : heap) {
		if (x == val) {
			instructions.push_back(HeapInstruction(HeapOp::FoundValue));
			return instructions;
		}
		instructions.push_back(HeapInstruction(HeapOp::VisitStraight));
	}
	instructions.push_back(HeapInstruction(HeapOp::NotFound));
	return instructions;
}
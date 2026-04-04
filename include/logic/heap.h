#pragma once

#include <vector>
#include <string>

enum class HeapType {
	MinHeap,
	MaxHeap
};

enum class HeapOp {
	SwapLeftChild, // Swap the current node with its left child and go to left child, the data also shows the left child pos
	SwapRightChild, // Swap the current node with its right child and go to right child, the data also shows the right child pos
	SwapParent, // Swap the current node with its parent and go to parent
	VisitStraight, // Visit the next index node in the heap vector without swapping
	UpdateValue, // Update the value of the current node to 'data'
	AddBackValue, // Add a new node with value 'data' to the back of the heap vecctor 
	MoveBackToTop, // Move the back value to the top of the heap and delete the back value

	FoundValue, // Search found the value
	NotFound, // Search did not find the value
};

struct HeapInstruction {
	HeapOp heap_op;
	int data;

	HeapInstruction(HeapOp _heap_op, int _data) : heap_op(_heap_op), data(_data) {}
	HeapInstruction(HeapOp _heap_op) : heap_op(_heap_op), data(0) {}
};

class Heap {
private:
	// Heap data
	std::vector<int> heap;
	HeapType heap_type;

	// Helper functions for heap operations
	bool compareIndex(const int& ida, const int& idb) const; // Compare always return true if the heap property is satisfied between ida (parent) and idb (child)
	int getParent(const int &id) const { return (id - 1) / 2; }
	void heapifyUp(int id);
	void heapifyDown(int id);

	std::vector<HeapInstruction> heapifyUpStep(int id);
	std::vector<HeapInstruction> heapifyDownStep(int id);
public:

	Heap();
	~Heap();

	void clear() { heap.clear(); }
	void swapType() { heap_type = (heap_type == HeapType::MinHeap) ? HeapType::MaxHeap : HeapType::MinHeap; }
	const std::vector<int>& getData() const { return heap; }
	HeapType getType() const { return heap_type; }

	// Do All At Once functions
	void initFromList(const std::vector<int>&  list);
	void initFromFile(const std::string& file_path);
	void initRandom(const int &num);
    void insertValue(const int& val);
	bool searchValue(const int& val);
	void deleteTop();
	void updateValue(const int& old_value, const int &new_value);


	// Step by step functions
	std::vector<HeapInstruction> initFromListStep(const std::vector<int>& list);
	std::vector<HeapInstruction> initFromFileStep(const std::string& file_path);
	std::vector<HeapInstruction> insertValueStep(const int& val);
	std::vector<HeapInstruction> searchValueStep(const int& val);
	std::vector<HeapInstruction> deleteTopStep();
	std::vector<HeapInstruction> updateValueStep(const int& old_value, const int& new_value);

};

inline Heap heap;

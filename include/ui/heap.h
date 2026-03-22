#pragma once

class HeapUI {
public:
	HeapUI();

	void draw();
private:
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
};

inline HeapUI heap_ui;
#pragma once

class ShortestPathUI {
public:
	ShortestPathUI();

	void draw();
private:
	float nodeRadius_ = 28.0f;
	float edgeThickness_ = 3.0f;
	float fontScale_ = 1.0f;
};

inline ShortestPathUI shortest_path_ui;	
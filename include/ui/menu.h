#pragma once

#include <imgui.h>
#include "ui/common.h"

class MenuUI {
public:
	MenuUI();
	void draw(); // Hàm vẽ giao diện Menu
};

// Tạo sẵn một đối tượng menu_ui để gọi trong hàm main
inline MenuUI menu_ui;
#pragma once

// Định nghĩa các trạng thái màn hình của ứng dụng
enum class UIState {
	Menu,
	Trie,
	Heap,
	SinglyLinkedList,
	ShortestPath,
};

// Cấu trúc lưu trữ trạng thái hiện tại
struct UIConfig {
	UIState state = UIState::Menu; // Mặc định khi mở app lên sẽ ở Menu
};

// Biến toàn cục để các file khác đều có thể truy cập và thay đổi trạng thái
inline UIConfig uiConfig;
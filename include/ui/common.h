#pragma once

struct ImFont;

enum class WindowMode {
	Windowed,
	Fullscreen,
};

struct GraphicsSettings {
	WindowMode windowMode = WindowMode::Fullscreen;
	int resolutionWidth = 1920;
	int resolutionHeight = 1080;
	bool fxaaEnabled = false;
	bool antialiasingEnabled = true;
	int antialiasingLevel = 8;
	bool vsyncEnabled = true;
	int fpsLimit = 60;
};

struct AudioSettings {
	bool musicEnabled = true;
	float musicVolume = 40.0f;
	bool sfxEnabled = true;
	float sfxVolume = 70.0f;
};

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
	bool requestAppQuit = false;
	bool requestGraphicsApply = false;
	GraphicsSettings graphicsSettings;
	AudioSettings audioSettings;
	int monitorWidth = 1920;
	int monitorHeight = 1080;
};

// Biến toàn cục để các file khác đều có thể truy cập và thay đổi trạng thái
inline UIConfig uiConfig;

// Shared ImGui fonts for high-DPI menu text rendering.
inline ImFont* menuTitleFont = nullptr;
inline ImFont* menuSubtitleFont = nullptr;
inline ImFont* menuCardTitleFont = nullptr;
inline ImFont* menuCardDescFont = nullptr;
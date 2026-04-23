#pragma once

#include <imgui.h>

#include "utils/audio_manager.h"

inline bool AudioButton(const char* label, const ImVec2& size = ImVec2(0.0f, 0.0f)) {
	if (ImGui::Button(label, size)) {
		audioManager.playClick();
		return true;
	}
	return false;
}

inline bool AudioInvisibleButton(const char* strId, const ImVec2& size, ImGuiButtonFlags flags = 0) {
	if (ImGui::InvisibleButton(strId, size, flags)) {
		audioManager.playClick();
		return true;
	}
	return false;
}

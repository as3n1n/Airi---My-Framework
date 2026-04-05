#pragma once
#include <windows.h>

namespace ImGui
{
	void BeginGroupPanel(const char* label, const ImVec2& size = ImVec2(-FLT_MIN, 0.0f));
	void EndGroupPanel();
	void Hotkey(const char* label, unsigned char* p_key, bool* p_held = NULL);
	void SmallHotkey(const char* label, unsigned char* p_key, bool* p_held = NULL);
	bool Toggle(const char* label, bool* v);
	void DrawKeyboardOverlay();
}
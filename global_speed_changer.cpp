#include "pch.h"
#include "global_speed_changer.h"
#include "cheat.h"
#include "engine.h"
#include "global.h"
#include "imgui.h"
#include "imgui_user.h"

// Time.set_timeScale RVA: 0x15AD910
// Time.get_timeScale RVA: 0x15AD810

namespace GlobalSpeedChanger
{
	static float s_Orig = 1.0f;
	static bool  s_Saved = false;

	static void Set(float v)
	{
		if (!hIl2Cpp) return;
		((void(*)(float))((PBYTE)hIl2Cpp + 0x15AD910))(v);
	}
	static float Get()
	{
		if (!hIl2Cpp) return 1.0f;
		return ((float(*)())((PBYTE)hIl2Cpp + 0x15AD810))();
	}

	void Menu()
	{
		ImGui::BeginGroupPanel("Speed Changer", ImVec2(-FLT_MIN, 0));
		bool prev = Options.GlobalSpeedChanger;
		ImGui::Toggle("Enable", &Options.GlobalSpeedChanger);
		if (Options.GlobalSpeedChanger)
		{
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1));
			ImGui::Text("Speed"); ImGui::PopStyleColor(); ImGui::SameLine();
			ImGui::SetNextItemWidth(140);
			ImGui::SliderFloat("##GS", &Options.GlobalSpeedChangerValue, 0.1f, 10, "x%.2f");
			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1));
			ImGui::Text("Hotkey"); ImGui::PopStyleColor(); ImGui::SameLine();
			ImGui::SmallHotkey("##GSK", &Options.GlobalSpeedChangerKey);
		}
		else if (prev && !Options.GlobalSpeedChanger)
		{
			Set(s_Orig); s_Saved = false;
		}
		ImGui::EndGroupPanel();
	}

	void BeforeFrame() {}

	void OnFrame()
	{
		if (!hIl2Cpp) return;
		if (Options.GlobalSpeedChangerKey)
		{
			bool d = (GetAsyncKeyState(Options.GlobalSpeedChangerKey) & 0x8000) != 0;
			if (d && !Options.GlobalSpeedChangerKeyHeld)
			{
				Options.GlobalSpeedChangerKeyHeld = true;
				Options.GlobalSpeedChanger = !Options.GlobalSpeedChanger;
                Cheat::PushHotkeyMessage("SPEED", Options.GlobalSpeedChanger, Input::GetKeyName(Options.GlobalSpeedChangerKey));
				if (!Options.GlobalSpeedChanger) { Set(s_Orig);s_Saved = false; }
			}
			else if (!d) Options.GlobalSpeedChangerKeyHeld = false;
		}
		if (!Options.GlobalSpeedChanger) return;
		if (!s_Saved) { s_Orig = Get();s_Saved = true; }
		Set(Options.GlobalSpeedChangerValue);
	}

	bool Setup() { return TRUE; }
}

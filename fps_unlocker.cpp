#include "pch.h"
#include "fps_unlocker.h"
#include "global.h"
#include "imgui.h"
#include "imgui_user.h"

namespace FpsUnlocker
{
    static void SetFPS(int fps)
    {
        if (!hIl2Cpp) return;
        ((void(*)(int))((PBYTE)hIl2Cpp + 0x175A5B0))(fps);
    }

    void Menu()
    {
        ImGui::BeginGroupPanel("FPS Unlocker", ImVec2(-FLT_MIN, 0));
        ImGui::Toggle("Enable", &Options.FpsUnlocker);
        if (Options.FpsUnlocker)
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1));
            ImGui::Text("Target FPS"); ImGui::PopStyleColor(); ImGui::SameLine();
            ImGui::SetNextItemWidth(120);
            ImGui::SliderInt("##FPS", &Options.FpsUnlockerValue, 30, 360, "%d fps");
        }
        ImGui::EndGroupPanel();
    }

    void BeforeFrame() {}

    void OnFrame()
    {
        if (!hIl2Cpp) return;
        if (Options.FpsUnlocker)
            SetFPS(Options.FpsUnlockerValue);
    }

    bool Setup() { return TRUE; }
}
#include "pch.h"
#include "fov_changer.h"
#include "cheat.h"
#include "phasmo_helpers.h"
#include "global.h"
#include "imgui.h"
#include "imgui_user.h"

// ══════════════════════════════════════════════════════════
//  FOV_CHANGER.CPP — Phasmophobia
//  Field of View modification
// ══════════════════════════════════════════════════════════

namespace FovChanger
{
    static float GetFOV()
    {
        if (!hIl2Cpp) return 60.f;

        uintptr_t base = Phasmo_Base();
        if (!base) return 60.f;

        using GetMainFn = void* (*)(const void*);
        using GetFovFn = float (*)(void*, const void*);

        auto getMain = reinterpret_cast<GetMainFn>(base + Phasmo::RVA_Camera_get_main);
        auto getFov = reinterpret_cast<GetFovFn>(base + Phasmo::RVA_Camera_get_fieldOfView);

        void* cam = getMain(nullptr);
        if (!cam) return 60.f;

        return getFov(cam, nullptr);
    }

    static void SetFOV(float fov)
    {
        if (!hIl2Cpp) return;

        uintptr_t base = Phasmo_Base();
        if (!base) return;

        using GetMainFn = void* (*)(const void*);
        using SetFovFn = void (*)(void*, float, const void*);

        auto getMain = reinterpret_cast<GetMainFn>(base + Phasmo::RVA_Camera_get_main);
        auto setFov = reinterpret_cast<SetFovFn>(base + Phasmo::RVA_Camera_set_fieldOfView);

        void* cam = getMain(nullptr);
        if (!cam) return;

        setFov(cam, fov, nullptr);
    }

    void Menu()
    {
        ImGui::BeginGroupPanel("FOV Changer", ImVec2(-FLT_MIN, 0));
        bool prev = Cheat::PLAYER_FovEditor;
        ImGui::Toggle("Enable", &Cheat::PLAYER_FovEditor);
        Options.FovChanger = Cheat::PLAYER_FovEditor; // keep legacy flag in sync
        if (Cheat::PLAYER_FovEditor)
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1));
            ImGui::Text("FOV"); ImGui::PopStyleColor(); ImGui::SameLine();
            ImGui::SetNextItemWidth(140);
            ImGui::SliderFloat("##FOV", &Cheat::PLAYER_FovValue, 30.f, 120.f, "%.0f");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1));
            ImGui::Text("Hotkey"); ImGui::PopStyleColor(); ImGui::SameLine();
            ImGui::SmallHotkey("##FOVK", &Options.FovChangerKey);
            Options.FovChangerValue = Cheat::PLAYER_FovValue;
        }
        else if (prev && !Cheat::PLAYER_FovEditor)
        {
            SetFOV(60.f);
        }
        ImGui::EndGroupPanel();
    }

    void BeforeFrame() {}

    void OnFrame()
    {
        if (!hIl2Cpp) return;
        if (Options.FovChangerKey)
        {
            bool d = (GetAsyncKeyState(Options.FovChangerKey) & 0x8000) != 0;
            if (d && !Options.FovChangerKeyHeld)
            {
                Options.FovChangerKeyHeld = true;
                Cheat::PLAYER_FovEditor = !Cheat::PLAYER_FovEditor;
                Options.FovChanger = Cheat::PLAYER_FovEditor;
                Cheat::PushHotkeyMessage("FOV", Cheat::PLAYER_FovEditor, Input::GetKeyName(Options.FovChangerKey));
                if (!Cheat::PLAYER_FovEditor) SetFOV(60.f);
            }
            else if (!d) Options.FovChangerKeyHeld = false;
        }
        if (Cheat::PLAYER_FovEditor) SetFOV(Cheat::PLAYER_FovValue);
    }

    bool Setup() { return TRUE; }
}

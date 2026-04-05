#include "pch.h"
#include "cheat.h"
#include "phasmo_helpers.h"
#include "global.h"
#include "imgui.h"
#include <algorithm>
#include <vector>
#include <deque>
#include <string>

// ══════════════════════════════════════════════════════════
//  VISUALS.CPP — Phasmophobia
//  HUD overlays and visual indicators
// ══════════════════════════════════════════════════════════

namespace Cheat
{
    struct HotkeyToast
    {
        std::string title;
        std::string detail;
        float time = 0.0f;
    };

    static std::deque<HotkeyToast> s_hotkeyToasts;

    void PushNotification(const char* title, const char* detail)
    {
        if ((!title || !*title) && (!detail || !*detail))
            return;

        const float now = ImGui::GetCurrentContext() ? static_cast<float>(ImGui::GetTime()) : 0.0f;
        const std::string t = title ? title : "";
        const std::string d = detail ? detail : "";

        // Dedup: if the same title+detail was pushed < 1s ago, skip it.
        for (const auto& existing : s_hotkeyToasts) {
            if (existing.title == t && existing.detail == d) {
                float age = (now > 0.0f && existing.time > 0.0f) ? (now - existing.time) : 0.0f;
                if (age < 1.0f)
                    return;
            }
        }

        HotkeyToast toast;
        toast.title = std::move(t);
        toast.detail = std::move(d);
        toast.time = now;

        s_hotkeyToasts.push_back(std::move(toast));
        if (s_hotkeyToasts.size() > 8)
            s_hotkeyToasts.pop_front();
    }

    void PushHotkeyMessage(const char* feature, bool enabled, const char* keyName)
    {
        if (!feature || !*feature)
            return;

        const char* state = enabled ? "ON" : "OFF";
        std::string msg = feature;
        msg += " ";
        msg += state;
        if (keyName && *keyName)
        {
            msg += " (";
            msg += keyName;
            msg += ")";
        }

        PushNotification("Hotkey", msg.c_str());
    }

    // ══════════════════════════════════════════════════════
    //  HOTKEY INDICATOR
    // ══════════════════════════════════════════════════════
    static void DrawHotkeyIndicator()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* dl = ImGui::GetForegroundDrawList();

        std::vector<const char*> activeFeatures;

        if (PLAYER_GodMode) activeFeatures.push_back("GODMODE");
        if (MOV_NoClip) activeFeatures.push_back("NOCLIP");
        if (MOV_InfinityStamina) activeFeatures.push_back("INF STAMINA");
        if (MOV_CustomSpeed) activeFeatures.push_back("SPEED");
        if (VIS_Fullbright) activeFeatures.push_back("FULLBRIGHT");
        if (VIS_GhostESP) activeFeatures.push_back("GHOST ESP");
        if (VIS_PlayerESP) activeFeatures.push_back("PLAYER ESP");
        if (MORE_AntiKick) activeFeatures.push_back("ANTI-KICK");

        if (activeFeatures.empty()) return;

        float x = 20.f;
        float y = io.DisplaySize.y - 80.f;

        for (const char* feature : activeFeatures)
        {
            ImVec2 textSize = ImGui::CalcTextSize(feature);
            ImVec2 boxMin{ x - 4.f, y - 2.f };
            ImVec2 boxMax{ x + textSize.x + 4.f, y + textSize.y + 2.f };

            dl->AddRectFilled(boxMin, boxMax, IM_COL32(20, 20, 25, 200), 2.f);
            dl->AddRect(boxMin, boxMax, IM_COL32(180, 80, 255, 150), 2.f, 0, 1.f);
            dl->AddText({ x, y }, IM_COL32(180, 80, 255, 255), feature);

            y -= textSize.y + 6.f;
        }
    }

    static void DrawHotkeyToasts()
    {
        if (s_hotkeyToasts.empty())
            return;

        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        const float now = static_cast<float>(ImGui::GetTime());
        const float life = 3.8f;

        const float rightMargin = 24.0f;
        float y = 92.0f;

        for (auto it = s_hotkeyToasts.begin(); it != s_hotkeyToasts.end(); )
        {
            if (it->time <= 0.0f)
                it->time = now;

            float age = now - it->time;
            if (age > life)
            {
                it = s_hotkeyToasts.erase(it);
                continue;
            }

            const float fadeIn = 0.18f;
            const float fadeOut = 0.35f;
            float alpha = 1.0f;
            if (age < fadeIn)
                alpha = age / fadeIn;
            else if (age > (life - fadeOut))
                alpha = std::max(0.0f, (life - age) / fadeOut);

            const float slideT = std::clamp(age / 0.28f, 0.0f, 1.0f);
            const float slide = (1.0f - slideT) * 48.0f;
            const float progress = std::clamp(1.0f - (age / life), 0.0f, 1.0f);

            const char* detail = it->detail.empty() ? nullptr : it->detail.c_str();
            ImVec2 titleSize = ImGui::CalcTextSize(it->title.c_str());
            ImVec2 detailSize = detail ? ImGui::CalcTextSize(detail, nullptr, false, 320.0f) : ImVec2(0.0f, 0.0f);
            const float boxWidth = std::max(titleSize.x, detailSize.x) + 48.0f;
            const float boxHeight = 18.0f + titleSize.y + (detail ? (detailSize.y + 8.0f) : 0.0f) + 12.0f;

            ImVec2 boxMax(io.DisplaySize.x - rightMargin - slide, y + boxHeight);
            ImVec2 boxMin(boxMax.x - boxWidth, y);

            dl->AddRectFilled(ImVec2(boxMin.x + 4.0f, boxMin.y + 6.0f), ImVec2(boxMax.x + 4.0f, boxMax.y + 6.0f),
                IM_COL32(0, 0, 0, static_cast<int>(70 * alpha)), 10.0f);
            dl->AddRectFilled(boxMin, boxMax, IM_COL32(15, 16, 22, static_cast<int>(230 * alpha)), 10.0f);
            dl->AddRect(boxMin, boxMax, IM_COL32(190, 76, 86, static_cast<int>(170 * alpha)), 10.0f, 0, 1.2f);
            dl->AddRectFilled(ImVec2(boxMin.x, boxMin.y), ImVec2(boxMin.x + 4.0f, boxMax.y),
                IM_COL32(230, 78, 88, static_cast<int>(255 * alpha)), 10.0f, ImDrawFlags_RoundCornersLeft);
            dl->AddRectFilled(ImVec2(boxMin.x + 10.0f, boxMax.y - 3.0f), ImVec2(boxMin.x + 10.0f + (boxWidth - 20.0f) * progress, boxMax.y - 1.0f),
                IM_COL32(255, 173, 83, static_cast<int>(225 * alpha)), 1.0f);

            dl->AddText(ImVec2(boxMin.x + 18.0f, boxMin.y + 10.0f), IM_COL32(245, 236, 236, static_cast<int>(255 * alpha)), it->title.c_str());
            if (detail) {
                dl->AddText(ImVec2(boxMin.x + 18.0f, boxMin.y + 14.0f + titleSize.y),
                    IM_COL32(188, 174, 176, static_cast<int>(255 * alpha)), detail);
            }

            y += boxHeight + 12.0f;
            ++it;
        }
    }

    // ══════════════════════════════════════════════════════
    //  MAIN VISUALS OnFrame
    // ══════════════════════════════════════════════════════
    void Visuals_OnFrame()
    {
        __try { DrawHotkeyIndicator(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        __try { DrawHotkeyToasts(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

} // namespace Cheat

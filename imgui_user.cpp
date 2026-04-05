#include "pch.h"
#include "imgui_user.h"
#include "global.h"
#include "imgui_internal.h"

namespace ImGui
{
    struct GroupPanelHeaderBounds { ImRect Left; ImRect Right; };
    static ImVector<GroupPanelHeaderBounds> GroupPanelStack;

    static float Clamp01(float value) { return value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value); }
    static ImU32 AccentStrong(int alpha = 255) { return IM_COL32((int)(Options.AccentR * 255.0f), (int)(Options.AccentG * 255.0f), (int)(Options.AccentB * 255.0f), alpha); }
    static ImU32 AccentBright(int alpha = 255) { return IM_COL32((int)(Clamp01(Options.AccentR + 0.12f) * 255.0f), (int)(Clamp01(Options.AccentG + 0.12f) * 255.0f), (int)(Clamp01(Options.AccentB + 0.12f) * 255.0f), alpha); }
    static ImU32 AccentDeep(int alpha = 255) { return IM_COL32((int)(Options.AccentR * 160.0f), (int)(Options.AccentG * 160.0f), (int)(Options.AccentB * 160.0f), alpha); }
    static ImU32 PanelDark(int alpha = 255) { return IM_COL32((int)(Options.BgR * 255.0f), (int)(Options.BgG * 255.0f), (int)(Options.BgB * 255.0f), alpha); }
    static ImU32 PanelAlt(int alpha = 255) { return IM_COL32((int)(Clamp01(Options.SidebarR + 0.04f) * 255.0f), (int)(Clamp01(Options.SidebarG + 0.03f) * 255.0f), (int)(Clamp01(Options.SidebarB + 0.03f) * 255.0f), alpha); }
    static ImU32 TextMuted(int alpha = 255) { return IM_COL32((int)(Options.TextR * 190.0f), (int)(Options.TextG * 185.0f), (int)(Options.TextB * 190.0f), alpha); }

    void BeginGroupPanel(const char* label, const ImVec2& size)
    {
        ImGuiContext& G = *GImGui;
        ImGuiWindow* Window = G.CurrentWindow;
        const ImGuiID Id = Window->GetID(label);
        ImGui::PushID(Id);

        ImVec2 groupPanelPos = Window->DC.CursorPos;
        ImVec2 itemSpacing = ImGui::GetStyle().ItemSpacing;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::BeginGroup();

        ImVec2 effectiveSize = size;
        effectiveSize.x = (size.x < 0.0f) ? ImGui::GetContentRegionAvail().x : size.x;

        ImGui::Dummy(ImVec2(effectiveSize.x, 0.0f));
        float frameHeight = ImGui::GetFrameHeight();
        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::SameLine(0.0f, 0.0f);

        ImGui::TextUnformatted(label);

        ImRect leftRect = { ImGui::GetItemRectMin(), ImGui::GetItemRectMax() };
        ImVec2 rightMax = ImVec2(groupPanelPos.x + effectiveSize.x - frameHeight, leftRect.Max.y);
        ImRect rightRect = { { rightMax.x, leftRect.Min.y }, rightMax };

        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Dummy(ImVec2(0.0f, frameHeight + itemSpacing.y));
        ImGui::PopStyleVar(2);

        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x -= frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->WorkRect.Max.x -= frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->InnerRect.Max.x -= frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->Size.x -= frameHeight;

        ImGui::PushItemWidth(ImMax(0.0f, ImGui::CalcItemWidth() - frameHeight));
        GroupPanelStack.push_back({ leftRect, rightRect });
        ImGui::Indent(7.0f * ImGui::GetStyle().FontScaleDpi);
    }

    void EndGroupPanel()
    {
        ImGui::Unindent(7.0f * ImGui::GetStyle().FontScaleDpi);

        GroupPanelHeaderBounds& info = GroupPanelStack.back();
        GroupPanelStack.pop_back();
        ImGui::PopItemWidth();

        ImVec2& itemSpacing = ImGui::GetStyle().ItemSpacing;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::EndGroup();

        float frameHeight = ImGui::GetFrameHeight();
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::Dummy(ImVec2(frameHeight * 0.5f, 0.0f));
        ImGui::Dummy(ImVec2(0.0f, frameHeight - frameHeight * 0.5f - itemSpacing.y));
        ImGui::EndGroup();

        ImVec2 itemMin = ImGui::GetItemRectMin();
        ImVec2 itemMax = ImGui::GetItemRectMax();
        ImVec2 halfFrame = ImVec2(frameHeight * 0.25f, frameHeight) * 0.5f;
        ImRect frameRect = ImRect(itemMin + halfFrame, itemMax - ImVec2(halfFrame.x, 0.0f));

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRect(frameRect.Min, frameRect.Max, IM_COL32(92, 58, 72, 225), 10.0f, 0, 1.0f);

        ImRect& leftRect = info.Left;
        leftRect.Min.x -= itemSpacing.x;
        leftRect.Max.x += itemSpacing.x;

        bool hasRight = info.Right.Min.x != info.Right.Max.x;
        ImRect& rightRect = info.Right;
        if (hasRight) { rightRect.Min.x -= itemSpacing.x; rightRect.Max.x += itemSpacing.x; }

        for (int i = 0; i < (hasRight ? 5 : 3); ++i)
        {
            switch (i)
            {
            case 0: ImGui::PushClipRect(ImVec2(-FLT_MAX, -FLT_MAX), ImVec2(leftRect.Min.x, FLT_MAX), TRUE); break;
            case 1: ImGui::PushClipRect(ImVec2(leftRect.Max.x, -FLT_MAX), ImVec2(hasRight ? rightRect.Min.x : FLT_MAX, FLT_MAX), TRUE); break;
            case 2: ImGui::PushClipRect(ImVec2(leftRect.Min.x, leftRect.Max.y), ImVec2(leftRect.Max.x, FLT_MAX), TRUE); break;
            case 3: ImGui::PushClipRect(ImVec2(rightRect.Min.x, rightRect.Max.y), ImVec2(rightRect.Max.x, FLT_MAX), TRUE); break;
            case 4: ImGui::PushClipRect(ImVec2(rightRect.Max.x, -FLT_MAX), ImVec2(FLT_MAX, FLT_MAX), TRUE); break;
            }
            dl->AddRect(frameRect.Min, frameRect.Max, IM_COL32(92, 58, 72, 225), 10.0f);
            ImGui::PopClipRect();
        }

        ImGui::PopStyleVar(2);
        ImGui::GetCurrentWindow()->ContentRegionRect.Max.x += frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->WorkRect.Max.x += frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->InnerRect.Max.x += frameHeight * 0.5f;
        ImGui::GetCurrentWindow()->Size.x += frameHeight;
        ImGui::Dummy(ImVec2(0.0f, 0.0f));
        ImGui::PopID();
    }

    bool Toggle(const char* label, bool* v)
    {
        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems) return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);

        const float height = 22.0f;
        const float width = 44.0f;
        const float radius = height * 0.50f;
        const ImVec2 labelSize = CalcTextSize(label, nullptr, true);
        const float fullWidth = ImMax(width + style.ItemInnerSpacing.x + labelSize.x, ImGui::GetContentRegionAvail().x);
        const float totalHeight = ImMax(height, labelSize.y + style.FramePadding.y * 2.0f);

        const ImVec2 pos = window->DC.CursorPos;
        const ImRect bb(pos, ImVec2(pos.x + fullWidth, pos.y + totalHeight));

        ItemSize(bb, style.FramePadding.y);
        if (!ItemAdd(bb, id)) return false;

        bool hovered = false;
        bool held = false;
        const bool clicked = ButtonBehavior(bb, id, &hovered, &held);
        if (clicked)
        {
            *v = !*v;
            MarkItemEdited(id);
        }

        ImDrawList* dl = GetWindowDrawList();
        const ImU32 rowColor = hovered ? PanelAlt(205) : PanelDark(182);
        dl->AddRectFilled(bb.Min, bb.Max, rowColor, 6.0f);
        if (*v)
        {
            dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(35, 22, 29, 220), 6.0f);
        }

        const ImVec2 pillMin = ImVec2(bb.Max.x - width - 8.0f, pos.y + (totalHeight - height) * 0.5f);
        const ImVec2 pillMax = ImVec2(pillMin.x + width, pillMin.y + height);

        dl->AddRectFilled(pillMin, pillMax, *v ? AccentDeep(255) : PanelAlt(255), radius);
        dl->AddRect(pillMin, pillMax, *v ? AccentStrong(255) : TextMuted(170), radius, 0, 1.2f);

        const float knobRadius = radius - 3.0f;
        const float knobX = *v ? pillMax.x - radius : pillMin.x + radius;
        const float knobY = pillMin.y + radius;

        if (*v)
        {
            dl->AddCircleFilled(ImVec2(knobX, knobY), knobRadius + 2.0f, AccentBright(90));
        }
        dl->AddCircleFilled(ImVec2(knobX, knobY), knobRadius, IM_COL32(255, 248, 250, 255));

        const ImVec2 textPos = ImVec2(pos.x + 10.0f, pos.y + (totalHeight - labelSize.y) * 0.5f);
        dl->AddText(textPos, *v ? IM_COL32(255, 244, 248, 255) : TextMuted(235), label);

        return clicked;
    }

    void Hotkey(const char* label, unsigned char* p_key, bool* p_held)
    {
        ImGui::PushID(label);
        unsigned char key = *p_key;
        if (key == 0xFF)
        {
            unsigned char lk = Input::GetLastKeyDown();
            if (lk == VK_ESCAPE) *p_key = 0x00;
            else if (lk != 0x00) *p_key = lk;
        }
        const char* bl = (key == 0x00) ? "NONE" : (key == 0xFF) ? "..." :
            (Input::GetKeyName(key) ? Input::GetKeyName(key) : "UNKNOWN");
        if (Button(bl)) { *p_key = 0xFF; Input::ClearLastKeyDown(); }
        if (key != 0x00 && key != 0xFF && p_held)
        {
            ImGui::SameLine();
            if (Button(*p_held ? "HOLD" : "TOGGLE")) *p_held = !*p_held;
        }
        ImGui::PopID();
    }

    void SmallHotkey(const char* label, unsigned char* p_key, bool* p_held)
    {
        ImGui::PushID(label);
        unsigned char key = *p_key;
        if (key == 0xFF)
        {
            unsigned char lk = Input::GetLastKeyDown();
            if (lk == VK_ESCAPE) *p_key = 0x00;
            else if (lk != 0x00) *p_key = lk;
        }
        const char* bl = (key == 0x00) ? "NONE" : (key == 0xFF) ? "..." :
            (Input::GetKeyName(key) ? Input::GetKeyName(key) : "UNKNOWN");
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.88f, 0.56f, 0.72f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.93f, 0.66f, 0.79f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.80f, 0.45f, 0.63f, 1.00f));
        if (SmallButton(bl)) { *p_key = 0xFF; Input::ClearLastKeyDown(); }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar();
        if (key != 0x00 && key != 0xFF && p_held)
        {
            ImGui::SameLine();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 10.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.16f, 0.22f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.20f, 0.27f, 0.96f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.42f, 0.23f, 0.31f, 1.00f));
            if (SmallButton(*p_held ? "HOLD" : "TOGGLE")) *p_held = !*p_held;
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }
        ImGui::PopID();
    }

    static bool IsAzerty()
    {
        HKL layout = GetKeyboardLayout(0);
        LANGID lang = LOWORD((DWORD_PTR)layout);
        return (lang == 0x040C || lang == 0x080C);
    }

    void DrawKeyboardOverlay()
    {
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* dl = ImGui::GetForegroundDrawList();

        const float keyW = 32.0f;
        const float keyH = 32.0f;
        const float gap = 5.0f;
        const float rounding = 5.0f;

        const bool azerty = IsAzerty();

        struct RowKey
        {
            const char* qw;
            const char* az;
            int vk;
            float x;
            float y;
            float w;
            float h;
        };

        const RowKey keys[] =
        {
            {"TAB", "TAB", VK_TAB, 0.0f, keyH + gap, keyW * 1.5f, keyH},
            {"Q", "A", azerty ? 'A' : 'Q', keyW * 1.5f + gap, keyH + gap, keyW, keyH},
            {"W", "Z", azerty ? 'Z' : 'W', keyW * 2.5f + gap * 2, keyH + gap, keyW, keyH},
            {"E", "E", 'E', keyW * 3.5f + gap * 3, keyH + gap, keyW, keyH},
            {"R", "R", 'R', keyW * 4.5f + gap * 4, keyH + gap, keyW, keyH},
            {"CAPS", "CAPS", VK_CAPITAL, 0.0f, keyH * 2 + gap * 2, keyW * 1.75f, keyH},
            {"A", "Q", azerty ? 'Q' : 'A', keyW * 1.75f + gap, keyH * 2 + gap * 2, keyW, keyH},
            {"S", "S", 'S', keyW * 2.75f + gap * 2, keyH * 2 + gap * 2, keyW, keyH},
            {"D", "D", 'D', keyW * 3.75f + gap * 3, keyH * 2 + gap * 2, keyW, keyH},
            {"F", "F", 'F', keyW * 4.75f + gap * 4, keyH * 2 + gap * 2, keyW, keyH},
            {"SHIFT", "SHIFT", VK_SHIFT, 0.0f, keyH * 3 + gap * 3, keyW * 2.25f, keyH},
            {"Z", "W", azerty ? 'W' : 'Z', keyW * 2.25f + gap, keyH * 3 + gap * 3, keyW, keyH},
            {"X", "X", 'X', keyW * 3.25f + gap * 2, keyH * 3 + gap * 3, keyW, keyH},
            {"C", "C", 'C', keyW * 4.25f + gap * 3, keyH * 3 + gap * 3, keyW, keyH},
            {"CTRL", "CTRL", VK_CONTROL, 0.0f, keyH * 4 + gap * 4, keyW * 1.5f, keyH},
            {"ALT", "ALT", VK_MENU, keyW * 1.5f + gap, keyH * 4 + gap * 4, keyW * 1.25f, keyH},
            {"SPACE", "SPACE", VK_SPACE, keyW * 2.75f + gap * 2, keyH * 4 + gap * 4, keyW * 3.5f, keyH},
        };

        const float mouseW = 52.0f;
        const float mouseGap = 18.0f;
        const float panelW = keyW * 7.5f + gap * 7 + 16.0f;
        const float panelH = keyH * 5 + gap * 5 + 16.0f;

        const float startX = (io.DisplaySize.x - (panelW + mouseGap + mouseW)) * 0.5f;
        const float startY = io.DisplaySize.y - panelH - 20.0f;

        dl->AddRectFilled(ImVec2(startX - 8.0f, startY - 8.0f), ImVec2(startX + panelW, startY + panelH), PanelDark(230), 6.0f);
        dl->AddRect(ImVec2(startX - 8.0f, startY - 8.0f), ImVec2(startX + panelW, startY + panelH), AccentStrong(225), 6.0f, 0, 1.4f);

        for (const RowKey& k : keys)
        {
            const bool pressed = (GetAsyncKeyState(k.vk) & 0x8000) != 0;

            const ImVec2 keyMin = ImVec2(startX + k.x, startY + k.y);
            const ImVec2 keyMax = ImVec2(keyMin.x + k.w, keyMin.y + k.h);

            if (pressed)
            {
                dl->AddRectFilled(ImVec2(keyMin.x - 2.0f, keyMin.y - 2.0f), ImVec2(keyMax.x + 2.0f, keyMax.y + 2.0f), AccentBright(95), rounding + 2.0f);
            }

            dl->AddRectFilled(keyMin, keyMax, pressed ? AccentDeep(255) : PanelAlt(245), rounding);
            dl->AddRect(keyMin, keyMax, pressed ? AccentBright(255) : TextMuted(185), rounding, 0, 1.4f);

            const char* lbl = azerty ? k.az : k.qw;
            const ImVec2 textSize = ImGui::CalcTextSize(lbl);
            dl->AddText(ImVec2(keyMin.x + (k.w - textSize.x) * 0.5f, keyMin.y + (k.h - textSize.y) * 0.5f),
                pressed ? IM_COL32(247, 241, 241, 255) : IM_COL32(196, 173, 174, 255),
                lbl);
        }

        const float mx = startX + panelW + mouseGap;
        const float my = startY + 12.0f;
        const float mw = mouseW;
        const float mh = 80.0f;

        const bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        const bool rmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

        dl->AddRectFilled(ImVec2(mx, my), ImVec2(mx + mw, my + mh), PanelDark(230), 10.0f);
        dl->AddRect(ImVec2(mx, my), ImVec2(mx + mw, my + mh), AccentStrong(225), 10.0f, 0, 1.4f);

        dl->AddLine(ImVec2(mx + mw * 0.5f, my), ImVec2(mx + mw * 0.5f, my + mh * 0.48f), TextMuted(180), 1.0f);
        dl->AddLine(ImVec2(mx, my + mh * 0.48f), ImVec2(mx + mw, my + mh * 0.48f), TextMuted(180), 1.0f);

        if (lmb)
        {
            dl->AddRectFilled(ImVec2(mx + 2.0f, my + 2.0f), ImVec2(mx + mw * 0.5f - 1.0f, my + mh * 0.48f - 1.0f), AccentDeep(255), 9.0f);
        }
        if (rmb)
        {
            dl->AddRectFilled(ImVec2(mx + mw * 0.5f + 1.0f, my + 2.0f), ImVec2(mx + mw - 2.0f, my + mh * 0.48f - 1.0f), AccentDeep(255), 9.0f);
        }

        const ImVec2 wheelMin = ImVec2(mx + mw * 0.5f - 5.0f, my + mh * 0.14f);
        const ImVec2 wheelMax = ImVec2(mx + mw * 0.5f + 5.0f, my + mh * 0.38f);
        dl->AddRectFilled(wheelMin, wheelMax, PanelAlt(255), 4.0f);
        dl->AddRect(wheelMin, wheelMax, AccentStrong(210), 4.0f, 0, 1.2f);
    }
}

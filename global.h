#pragma once
#include <windows.h>
#include <string>
#include "engine.h"
#include "input.h"
#include "imgui.h"

struct OPTIONS
{
    // ── Menu ─────────────────────────────────────────────────────────────
    unsigned char MenuKey = VK_INSERT;
    bool          MenuCustomScale = FALSE;
    float         MenuCustomScaleValue = 2.0f;
    char          MenuBackgroundPath[MAX_PATH] = "";
    float         MenuBackgroundOpacity = 0.32f;
    char          MenuFontPath[MAX_PATH] = "";
    bool          MenuLanguageFrench = false;

    // ── FPS ──────────────────────────────────────────────────────────────
    bool FpsIndicator = TRUE;
    bool FpsUnlocker = FALSE;
    int  FpsUnlockerValue = 120;

    // ── FOV ──────────────────────────────────────────────────────────────
    bool          FovChanger = FALSE;
    unsigned char FovChangerKey = 0;
    bool          FovChangerKeyHeld = FALSE;
    float         FovChangerValue = 90.0f;

    // ── Speed ─────────────────────────────────────────────────────────────
    bool          GlobalSpeedChanger = FALSE;
    unsigned char GlobalSpeedChangerKey = 0;
    bool          GlobalSpeedChangerKeyHeld = FALSE;
    float         GlobalSpeedChangerValue = 2.0f;

    // ── Theme ────────────────────────────────────────────────────────────
    int   ThemePreset = 0;       // 0=Airi, 1=Unicore, 2=Refund, 3=Custom
    float AccentR = 0.95f;
    float AccentG = 0.69f;
    float AccentB = 0.81f;

    // Custom theme colors (only used when ThemePreset == 3)
    float BgR = 0.08f, BgG = 0.08f, BgB = 0.10f;             // Window background
    float SidebarR = 0.05f, SidebarG = 0.05f, SidebarB = 0.063f; // Sidebar background
    float TextR = 0.80f, TextG = 0.80f, TextB = 0.84f;        // Primary text
};

extern OPTIONS Options;

inline const char* UI_Lang(const char* en, const char* fr)
{
    return Options.MenuLanguageFrench ? fr : en;
}

std::wstring GetAiriDocumentsDirW();
std::string GetAiriDocumentsDirA();
std::wstring GetAiriSubdirW(const wchar_t* subdir);
std::string GetAiriSubdirA(const char* subdir);

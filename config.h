#pragma once
#include <windows.h>

namespace Config
{
    void Menu();
    void MenuConfigProfiles();
    void MenuThemes();
    void BeforeFrame();
    void OnFrame();
    bool Setup();
    void Save(int slot = 0);
    void Load(int slot = 0);
    void SaveTheme(const char* name);
    void LoadTheme(const char* name);
    void Reset();
}

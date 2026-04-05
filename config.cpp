#include "pch.h"
#include "config.h"
#include "global.h"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace
{
    std::string Trim(const std::string& value)
    {
        size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
            ++start;

        size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
            --end;

        return value.substr(start, end - start);
    }

    std::string GetLegacyConfigPath(int slot)
    {
        char path[MAX_PATH];
        GetModuleFileNameA(nullptr, path, MAX_PATH);
        std::string dir(path);
        dir = dir.substr(0, dir.find_last_of("\\/"));
        if (slot == 0)
            return dir + "\\chino-heart_config.bin";
        return dir + "\\chino-heart_config_" + std::to_string(slot) + ".bin";
    }
}

namespace Config
{
    static std::string GetConfigDir()
    {
        return GetAiriSubdirA("Configs");
    }

    static std::string GetThemeDir()
    {
        return GetAiriSubdirA("Themes");
    }

    static std::string GetConfigPath(int slot = 0)
    {
        const std::string dir = GetConfigDir();
        if (slot == 0)
            return dir + "\\airi_config.bin";
        return dir + "\\airi_config_" + std::to_string(slot) + ".bin";
    }

    static std::string GetThemePath(const char* name)
    {
        std::string themeName = Trim(name ? name : "");
        if (themeName.empty())
            themeName = "default";

        for (char& ch : themeName) {
            if (ch == '\\' || ch == '/' || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>' || ch == '|')
                ch = '_';
        }

        return GetThemeDir() + "\\" + themeName + ".theme";
    }

    static void ApplyThemePresetValues(int preset)
    {
        switch (preset)
        {
        case 0: // Airi
            Options.ThemePreset = 0;
            Options.AccentR = 0.95f; Options.AccentG = 0.69f; Options.AccentB = 0.81f;
            Options.BgR = 0.10f; Options.BgG = 0.07f; Options.BgB = 0.09f;
            Options.SidebarR = 0.15f; Options.SidebarG = 0.10f; Options.SidebarB = 0.13f;
            Options.TextR = 0.99f; Options.TextG = 0.95f; Options.TextB = 0.97f;
            break;
        case 1: // Unicore
            Options.ThemePreset = 1;
            Options.AccentR = 0.20f; Options.AccentG = 0.46f; Options.AccentB = 0.96f;
            Options.BgR = 0.05f; Options.BgG = 0.07f; Options.BgB = 0.14f;
            Options.SidebarR = 0.07f; Options.SidebarG = 0.09f; Options.SidebarB = 0.18f;
            Options.TextR = 0.92f; Options.TextG = 0.95f; Options.TextB = 1.00f;
            break;
        case 2: // Refund / iRucifix
            Options.ThemePreset = 2;
            Options.AccentR = 0.90f; Options.AccentG = 0.30f; Options.AccentB = 0.30f;
            Options.BgR = 0.14f; Options.BgG = 0.05f; Options.BgB = 0.05f;
            Options.SidebarR = 0.19f; Options.SidebarG = 0.08f; Options.SidebarB = 0.08f;
            Options.TextR = 0.98f; Options.TextG = 0.92f; Options.TextB = 0.92f;
            break;
        default:
            Options.ThemePreset = 3;
            break;
        }
    }

    static void WriteThemeFile(const std::string& path)
    {
        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out)
            return;

        out << "name=" << fs::path(path).stem().string() << "\n";
        out << "preset=" << Options.ThemePreset << "\n";
        out << "accent_r=" << Options.AccentR << "\n";
        out << "accent_g=" << Options.AccentG << "\n";
        out << "accent_b=" << Options.AccentB << "\n";
        out << "bg_r=" << Options.BgR << "\n";
        out << "bg_g=" << Options.BgG << "\n";
        out << "bg_b=" << Options.BgB << "\n";
        out << "sidebar_r=" << Options.SidebarR << "\n";
        out << "sidebar_g=" << Options.SidebarG << "\n";
        out << "sidebar_b=" << Options.SidebarB << "\n";
        out << "text_r=" << Options.TextR << "\n";
        out << "text_g=" << Options.TextG << "\n";
        out << "text_b=" << Options.TextB << "\n";
        out << "background_opacity=" << Options.MenuBackgroundOpacity << "\n";
        out << "background_path=" << Options.MenuBackgroundPath << "\n";
        out << "font_path=" << Options.MenuFontPath << "\n";
        out << "menu_scale_enabled=" << (Options.MenuCustomScale ? 1 : 0) << "\n";
        out << "menu_scale=" << Options.MenuCustomScaleValue << "\n";
        out << "menu_language_french=" << (Options.MenuLanguageFrench ? 1 : 0) << "\n";
    }

    static void ReadThemeFile(const std::string& path)
    {
        std::ifstream in(path);
        if (!in)
            return;

        std::string line;
        while (std::getline(in, line))
        {
            const size_t sep = line.find('=');
            if (sep == std::string::npos)
                continue;

            const std::string key = Trim(line.substr(0, sep));
            const std::string value = Trim(line.substr(sep + 1));

            try
            {
                if (key == "preset") Options.ThemePreset = std::stoi(value);
                else if (key == "accent_r") Options.AccentR = std::stof(value);
                else if (key == "accent_g") Options.AccentG = std::stof(value);
                else if (key == "accent_b") Options.AccentB = std::stof(value);
                else if (key == "bg_r") Options.BgR = std::stof(value);
                else if (key == "bg_g") Options.BgG = std::stof(value);
                else if (key == "bg_b") Options.BgB = std::stof(value);
                else if (key == "sidebar_r") Options.SidebarR = std::stof(value);
                else if (key == "sidebar_g") Options.SidebarG = std::stof(value);
                else if (key == "sidebar_b") Options.SidebarB = std::stof(value);
                else if (key == "text_r") Options.TextR = std::stof(value);
                else if (key == "text_g") Options.TextG = std::stof(value);
                else if (key == "text_b") Options.TextB = std::stof(value);
                else if (key == "background_opacity") Options.MenuBackgroundOpacity = std::stof(value);
                else if (key == "menu_scale") Options.MenuCustomScaleValue = std::stof(value);
                else if (key == "menu_scale_enabled") Options.MenuCustomScale = (std::stoi(value) != 0);
                else if (key == "menu_language_french") Options.MenuLanguageFrench = (std::stoi(value) != 0);
                else if (key == "background_path") strncpy_s(Options.MenuBackgroundPath, value.c_str(), _TRUNCATE);
                else if (key == "font_path") strncpy_s(Options.MenuFontPath, value.c_str(), _TRUNCATE);
            }
            catch (...) {}
        }

        Options.MenuBackgroundOpacity = std::clamp(Options.MenuBackgroundOpacity, 0.0f, 1.0f);
        Options.MenuCustomScaleValue = std::clamp(Options.MenuCustomScaleValue, 0.5f, 4.0f);
        if (Options.ThemePreset < 0 || Options.ThemePreset > 3)
            Options.ThemePreset = 0;
    }

    static std::vector<std::string> ListThemes()
    {
        std::vector<std::string> result;
        std::error_code ec;
        const fs::path dir = GetThemeDir();
        for (const auto& entry : fs::directory_iterator(dir, ec))
        {
            if (ec)
                break;
            if (!entry.is_regular_file())
                continue;
            if (entry.path().extension() == ".theme")
                result.push_back(entry.path().stem().string());
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    void Save(int slot)
    {
        std::string p = GetConfigPath(slot);
        HANDLE hf = CreateFileA(p.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hf == INVALID_HANDLE_VALUE) return;
        DWORD written = 0;
        WriteFile(hf, &Options, sizeof(OPTIONS), &written, nullptr);
        CloseHandle(hf);
    }

    void Load(int slot)
    {
        std::string p = GetConfigPath(slot);
        HANDLE hf = CreateFileA(p.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, 0, nullptr);
        if (hf == INVALID_HANDLE_VALUE)
        {
            const std::string legacy = GetLegacyConfigPath(slot);
            hf = CreateFileA(legacy.c_str(), GENERIC_READ, FILE_SHARE_READ,
                nullptr, OPEN_EXISTING, 0, nullptr);
            if (hf == INVALID_HANDLE_VALUE)
                return;
        }

        DWORD read = 0;
        ReadFile(hf, &Options, sizeof(OPTIONS), &read, nullptr);
        CloseHandle(hf);
    }

    void SaveTheme(const char* name)
    {
        WriteThemeFile(GetThemePath(name));
    }

    void LoadTheme(const char* name)
    {
        ReadThemeFile(GetThemePath(name));
    }

    void Reset()
    {
        Options = OPTIONS{};
        ApplyThemePresetValues(0);
    }

    void Menu()
    {
        ImGui::BeginGroupPanel("Config", ImVec2(-FLT_MIN, 0.0f));

        if (ImGui::Button("Save Config", ImVec2(-FLT_MIN, 0.0f)))
            Save();

        ImGui::Spacing();

        if (ImGui::Button("Load Config", ImVec2(-FLT_MIN, 0.0f)))
            Load();

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.25f, 0.25f, 1));
        ImGui::TextWrapped("Config is saved in Documents\\Airi\\Configs\\airi_config.bin");
        ImGui::PopStyleColor();

        ImGui::EndGroupPanel();
    }

    void MenuThemes()
    {
        static char s_themeName[64] = "airi";
        static int s_selectedTheme = -1;

        ImGui::BeginGroupPanel(UI_Lang("Theme Presets", "Presets Theme"), ImVec2(-FLT_MIN, 0.0f));
        if (ImGui::Button("Airi", ImVec2(-FLT_MIN, 0.0f)))
            ApplyThemePresetValues(0);
        if (ImGui::Button("Unicore", ImVec2(-FLT_MIN, 0.0f)))
            ApplyThemePresetValues(1);
        if (ImGui::Button("Refund", ImVec2(-FLT_MIN, 0.0f)))
            ApplyThemePresetValues(2);
        if (ImGui::Button(UI_Lang("Custom Theme", "Theme perso"), ImVec2(-FLT_MIN, 0.0f)))
            Options.ThemePreset = 3;
        ImGui::EndGroupPanel();

        ImGui::Spacing();
        ImGui::BeginGroupPanel(".theme", ImVec2(-FLT_MIN, 0.0f));
        ImGui::InputText("##ThemeName", s_themeName, IM_ARRAYSIZE(s_themeName));
        if (ImGui::Button(UI_Lang("Save Theme File", "Sauver theme"), ImVec2(-FLT_MIN, 0.0f)))
            SaveTheme(s_themeName);

        ImGui::Spacing();
        std::vector<std::string> themes = ListThemes();
        if (themes.empty())
        {
            ImGui::TextDisabled("No .theme file yet.");
        }
        else
        {
            for (int i = 0; i < static_cast<int>(themes.size()); ++i)
            {
                ImGui::PushID(i);
                const bool selected = (s_selectedTheme == i);
                if (ImGui::Selectable(themes[i].c_str(), selected))
                    s_selectedTheme = i;
                ImGui::SameLine();
                if (ImGui::SmallButton("Load"))
                {
                    LoadTheme(themes[i].c_str());
                    s_selectedTheme = i;
                }
                ImGui::PopID();
            }
        }

        ImGui::TextDisabled("Themes live in Documents\\Airi\\Themes\\*.theme");
        ImGui::EndGroupPanel();

        ImGui::Spacing();
        ImGui::BeginGroupPanel(UI_Lang("Theme Customization", "Personnalisation theme"), ImVec2(-FLT_MIN, 0.0f));
        ImGui::TextDisabled("Preset %d", Options.ThemePreset);
        if (ImGui::ColorEdit3("Accent", &Options.AccentR)) Options.ThemePreset = 3;
        if (ImGui::ColorEdit3("Background", &Options.BgR)) Options.ThemePreset = 3;
        if (ImGui::ColorEdit3("Sidebar", &Options.SidebarR)) Options.ThemePreset = 3;
        if (ImGui::ColorEdit3("Text", &Options.TextR)) Options.ThemePreset = 3;
        ImGui::TextDisabled("Background image and opacity are saved in the .theme file too.");
        ImGui::EndGroupPanel();
    }

    void MenuConfigProfiles()
    {
        static int s_Selected = 1;

        ImGui::BeginGroupPanel("Config Profiles", ImVec2(-FLT_MIN, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.92f, 1));
        ImGui::Text("Profile"); ImGui::PopStyleColor(); ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::SliderInt("##profslot", &s_Selected, 1, 5, "Slot %d");

        ImGui::Spacing();

        if (ImGui::Button("Save Profile", ImVec2(-FLT_MIN, 0.0f)))
            Save(s_Selected);

        ImGui::Spacing();

        if (ImGui::Button("Load Profile", ImVec2(-FLT_MIN, 0.0f)))
            Load(s_Selected);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.25f, 0.25f, 1));
        ImGui::TextWrapped("Profiles are saved in Documents\\Airi\\Configs\\airi_config_1.bin ... airi_config_5.bin");
        ImGui::PopStyleColor();

        ImGui::EndGroupPanel();
    }

    void BeforeFrame() {}
    void OnFrame() {}

    bool Setup()
    {
        Load();
        if (Options.ThemePreset < 0 || Options.ThemePreset > 3)
            ApplyThemePresetValues(0);
        return TRUE;
    }
}

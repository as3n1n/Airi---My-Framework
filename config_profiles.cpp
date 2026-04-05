#include "pch.h"
#include "cheat.h"
#include "engine.h"
#include "global.h"
#include "imgui.h"
#include "imgui_user.h"
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

namespace Cheat
{
	// ── Path ─────────────────────────────────────────────────────────────

	static std::string GetProfileDir()
	{
		return GetAiriSubdirA("Profiles") + "\\";
	}

	static std::string ProfilePath(const std::string& name)
	{
		return GetProfileDir() + name + ".json";
	}

	// ── Write JSON ────────────────────────────────────────────────────────

#define JP_BOOL(s,f)  (s) << "  \"" #f "\": " << (o.f ? "true" : "false") << ",\n"
#define JP_INT(s,f)   (s) << "  \"" #f "\": " << (int)(o.f) << ",\n"
#define JP_FLOAT(s,f) (s) << "  \"" #f "\": " << (o.f) << ",\n"
#define JP_LAST(s,f)  (s) << "  \"" #f "\": " << (o.f) << "\n"

	static void WriteJSON(const std::string& fp, const OPTIONS& o)
	{
		std::ofstream out(fp);
		if (!out) return;
		out << "{\n";
		JP_INT(out, MenuKey);
		JP_BOOL(out, MenuCustomScale);
		JP_FLOAT(out, MenuCustomScaleValue);
		JP_BOOL(out, FpsIndicator);
		JP_BOOL(out, FpsUnlocker);
		JP_INT(out, FpsUnlockerValue);
		JP_BOOL(out, FovChanger);
		JP_INT(out, FovChangerKey);
		JP_FLOAT(out, FovChangerValue);
		JP_BOOL(out, GlobalSpeedChanger);
		JP_INT(out, GlobalSpeedChangerKey);
		JP_LAST(out, GlobalSpeedChangerValue);
		out << "}\n";
	}

#undef JP_BOOL
#undef JP_INT
#undef JP_FLOAT
#undef JP_LAST

	// ── Read JSON ─────────────────────────────────────────────────────────

	static bool JR_Bool(const std::string& json, const char* key, bool& val)
	{
		std::string tok = std::string("\"") + key + "\": ";
		auto pos = json.find(tok);
		if (pos == std::string::npos) return false;
		pos += tok.size();
		val = (json.substr(pos, 4) == "true");
		return true;
	}

	static bool JR_Int(const std::string& json, const char* key, int& val)
	{
		std::string tok = std::string("\"") + key + "\": ";
		auto pos = json.find(tok);
		if (pos == std::string::npos) return false;
		pos += tok.size();
		try { val = std::stoi(json.substr(pos)); }
		catch (...) { return false; }
		return true;
	}

	static bool JR_Float(const std::string& json, const char* key, float& val)
	{
		std::string tok = std::string("\"") + key + "\": ";
		auto pos = json.find(tok);
		if (pos == std::string::npos) return false;
		pos += tok.size();
		try { val = std::stof(json.substr(pos)); }
		catch (...) { return false; }
		return true;
	}

	static void ReadJSON(const std::string& fp, OPTIONS& o)
	{
		std::ifstream in(fp);
		if (!in) return;
		std::ostringstream ss; ss << in.rdbuf();
		const std::string& j = ss.str();
		int iv = 0;

#define JR_B(f) JR_Bool(j, #f, o.f)
#define JR_I(f) if (JR_Int(j, #f, iv)) o.f = (decltype(o.f))iv
#define JR_F(f) JR_Float(j, #f, o.f)

		JR_I(MenuKey);
		JR_B(MenuCustomScale);   JR_F(MenuCustomScaleValue);
		JR_B(FpsIndicator);
		JR_B(FpsUnlocker);       JR_I(FpsUnlockerValue);
		JR_B(FovChanger);        JR_I(FovChangerKey);     JR_F(FovChangerValue);
		JR_B(GlobalSpeedChanger); JR_I(GlobalSpeedChangerKey); JR_F(GlobalSpeedChangerValue);

#undef JR_B
#undef JR_I
#undef JR_F
	}

	// ── Profile list ──────────────────────────────────────────────────────

	static std::vector<std::string> ListProfiles()
	{
		std::vector<std::string> r;
		std::string mask = GetProfileDir() + "*.json";
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA(mask.c_str(), &fd);
		if (h == INVALID_HANDLE_VALUE) return r;
		do {
			std::string n(fd.cFileName);
			if (n.size() > 5) r.push_back(n.substr(0, n.size() - 5));
		} while (FindNextFileA(h, &fd));
		FindClose(h);
		return r;
	}

	// ── State ─────────────────────────────────────────────────────────────

	static char  s_NewName[64] = "default";
	static int   s_Selected = -1;
	static char  s_Status[128] = {};
	static float s_StatusTimer = 0.0f;

	static void SetStatus(const char* msg)
	{
		strncpy_s(s_Status, msg, sizeof(s_Status) - 1);
		s_StatusTimer = 3.0f;
	}

	// ── Menu ──────────────────────────────────────────────────────────────

	void MenuConfigProfiles()
	{
		s_StatusTimer -= ImGui::GetIO().DeltaTime;

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.60f, 0.60f, 1));
		ImGui::TextWrapped("Profiles saved in Documents\\Airi\\Profiles\\");
		ImGui::PopStyleColor();
		ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();

		// ── Save ─────────────────────────────────────────────────────────
		ImGui::BeginGroupPanel("Save Profile", ImVec2(-FLT_MIN, 0));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1));
		ImGui::TextUnformatted("Name"); ImGui::PopStyleColor(); ImGui::SameLine();
		ImGui::SetNextItemWidth(160);
		ImGui::InputText("##PN", s_NewName, sizeof(s_NewName));
		ImGui::SameLine();
		if (ImGui::Button("Save##P") && s_NewName[0] != '\0')
		{
			WriteJSON(ProfilePath(s_NewName), Options);
			SetStatus("Profile saved.");
		}
		ImGui::EndGroupPanel();
		ImGui::Spacing();

		// ── Load ─────────────────────────────────────────────────────────
		ImGui::BeginGroupPanel("Load / Delete", ImVec2(-FLT_MIN, 0));
		auto profiles = ListProfiles();
		if (profiles.empty())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.25f, 0.25f, 1));
			ImGui::TextUnformatted("No profiles found."); ImGui::PopStyleColor();
		}
		else
		{
			for (int i = 0; i < (int)profiles.size(); i++)
			{
				ImGui::PushID(i);
				bool sel = (s_Selected == i);
				ImGui::PushStyleColor(ImGuiCol_Text,
					sel ? ImVec4(0.92f, 0.85f, 0.85f, 1) : ImVec4(0.65f, 0.45f, 0.45f, 1));
				if (ImGui::Selectable(profiles[i].c_str(), sel)) s_Selected = i;
				ImGui::PopStyleColor();
				ImGui::SameLine();
				if (ImGui::SmallButton("Load")) { ReadJSON(ProfilePath(profiles[i]), Options); s_Selected = i; SetStatus("Profile loaded."); }
				ImGui::SameLine();
				if (ImGui::SmallButton("X")) { DeleteFileA(ProfilePath(profiles[i]).c_str()); if (s_Selected == i)s_Selected = -1; SetStatus("Deleted."); }
				ImGui::PopID();
			}
		}
		ImGui::EndGroupPanel();
		ImGui::Spacing();

		// ── Quick presets ────────────────────────────────────────────────
		ImGui::BeginGroupPanel("Quick Presets", ImVec2(-FLT_MIN, 0));
		if (ImGui::Button("Performance Preset", ImVec2(-FLT_MIN, 0)))
		{
			Options.FpsUnlocker = true;
			Options.FpsUnlockerValue = 120;
			Options.GlobalSpeedChanger = false;
			SetStatus("Performance preset applied.");
		}
		ImGui::Spacing();
		if (ImGui::Button("Speed Preset", ImVec2(-FLT_MIN, 0)))
		{
			Options.GlobalSpeedChanger = true;
			Options.GlobalSpeedChangerValue = 3.0f;
			Options.FpsUnlocker = true;
			Options.FpsUnlockerValue = 120;
			SetStatus("Speed preset applied.");
		}
		ImGui::EndGroupPanel();

		// ── Status ───────────────────────────────────────────────────────
		if (s_StatusTimer > 0.0f)
		{
			ImGui::Spacing();
			float a = s_StatusTimer > 1.0f ? 1.0f : s_StatusTimer;
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.30f, 0.80f, 0.30f, a));
			ImGui::TextUnformatted(s_Status);
			ImGui::PopStyleColor();
		}
	}

} // namespace Cheat

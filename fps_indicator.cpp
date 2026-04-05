#include "pch.h"
#include "fps_indicator.h"
#include "global.h"
#include "imgui.h"

namespace FpsIndicator
{
	void Menu()
	{
		ImGui::BeginGroupPanel("FPS Indicator", ImVec2(-FLT_MIN, 0.0f));
		ImGui::Toggle("Enable FPS Indicator", &Options.FpsIndicator);
		ImGui::EndGroupPanel();
	}

	void BeforeFrame() {}

	void OnFrame()
	{
		if (!Options.FpsIndicator) return;

		ImGuiIO& io = ImGui::GetIO();
		float    fps = io.Framerate;
		float    ms = io.DeltaTime * 1000.0f;

		// Top-right corner
		ImVec2 pos = { io.DisplaySize.x - 8.0f, 8.0f };
		ImVec2 pivot = { 1.0f, 0.0f };

		ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pivot);
		ImGui::SetNextWindowBgAlpha(0.55f);
		ImGui::SetNextWindowSize(ImVec2(90.0f, 0.0f));

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoInputs |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImGui::PushStyleColor(ImGuiCol_WindowBg,
			ImVec4(0.04f, 0.01f, 0.01f, 0.55f));
		ImGui::PushStyleColor(ImGuiCol_Border,
			ImVec4(0.28f, 0.05f, 0.05f, 0.80f));

		if (ImGui::Begin("##FpsInd", nullptr, flags))
		{
			ImVec4 col = fps >= 55 ?
				ImVec4(0.30f, 0.85f, 0.30f, 1.0f) :
				fps >= 30 ?
				ImVec4(0.95f, 0.75f, 0.10f, 1.0f) :
				ImVec4(0.90f, 0.15f, 0.15f, 1.0f);

			ImGui::PushStyleColor(ImGuiCol_Text, col);
			ImGui::Text("%.0f FPS", fps);
			ImGui::PopStyleColor();

			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.55f, 0.55f, 1));
			ImGui::Text("%.2f ms", ms);
			ImGui::PopStyleColor();
		}
		ImGui::End();
		ImGui::PopStyleColor(2);
	}

	bool Setup() { return TRUE; }
}
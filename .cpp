#include "pch.h"
#include "cheat.h"
#include "engine.h"
#include "global.h"
#include "imgui.h"
#include "imgui_user.h"
#include "unitysdk/RPG/GameCore/AdventureStatic.h"
#include "unitysdk/RPG/Client/AdventurePhase.h"
#include "unitysdk/RPG/GameCore/GameWorld.h"
#include "unitysdk/RPG/GameCore/EntityManager.h"
#include "unitysdk/RPG/GameCore/GameEntity.h"
#include "unitysdk/RPG/GameCore/EntityType.h"
#include "unitysdk/RPG/GameCore/PropInteractMode.h"
#include "unitysdk/UnityEngine/GameObject.h"
#include "unitysdk/UnityEngine/Transform.h"
#include "unitysdk/System/Nullable_1.h"
#include <math.h>

namespace Cheat
{
	static bool  AutoLoot_Enable = false;
	static float AutoLoot_Range = 8.0f;
	static float AutoLoot_Interval = 0.5f;
	static float AutoLoot_Timer = 0.0f;
	static int   AutoLoot_Count = 0;

	static float Dist3D_AL(UnityEngine::Vector3 a, UnityEngine::Vector3 b)
	{
		float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
		return sqrtf(dx * dx + dy * dy + dz * dz);
	}

	static void AutoLoot_Sweep()
	{
		RPG::Client::AdventurePhase* phase =
			RPG::GameCore::AdventureStatic::GetAdventurePhase();
		if (!phase) return;

		RPG::GameCore::GameEntity* localEnt = phase->get_CurrentLocalPlayer();
		if (!localEnt) return;

		UnityEngine::GameObject* localGO = localEnt->get_UnityGO();
		if (!localGO) return;

		UnityEngine::Vector3 playerPos = localGO->get_transform()->get_position();

		RPG::GameCore::GameWorld* world = phase->GetMainWorld();
		if (!world) return;
		RPG::GameCore::EntityManager* em = world->get_EntityManagerRef();
		if (!em) return;
		auto* teamArray = em->_AllTeamEntityList;
		if (!teamArray) return;

		// Build a Nullable<PropInteractMode> with has_value = true
		// Field names from Nullable_1.h: value, has_value
		System::Nullable_1<RPG::GameCore::PropInteractMode> mode;
		mode.value = RPG::GameCore::PropInteractMode::FactToProp;
		mode.has_value = true;

		for (int t = 0; t < (int)teamArray->max_length; t++)
		{
			auto* teamList = teamArray->vector[t];
			if (!teamList) continue;

			for (int i = 0; i < teamList->_size; i++)
			{
				RPG::GameCore::GameEntity* ent = teamList->_items->vector[i];
				if (!ent || ent == localEnt) continue;

				RPG::GameCore::EntityType et = ent->get_EntityType();
				if (et != RPG::GameCore::EntityType::Prop &&
					et != RPG::GameCore::EntityType::LevelEntity) continue;

				UnityEngine::GameObject* go = ent->get_UnityGO();
				if (!go) continue;

				if (Dist3D_AL(playerPos, go->get_transform()->get_position())
			> AutoLoot_Range) continue;

			phase->SetCurrentInteractProp(ent, mode, nullptr);
			AutoLoot_Count++;

			if (Webhook_LogChestOpen())
				Webhook_Send("Chest Looted", "Auto Loot triggered an interaction.");
			}
		}
	}

	void AutoLoot_OnFrame()
	{
		if (!AutoLoot_Enable) return;
		AutoLoot_Timer += ImGui::GetIO().DeltaTime;
		if (AutoLoot_Timer < AutoLoot_Interval) return;
		AutoLoot_Timer = 0.0f;
		AutoLoot_Sweep();
	}

	void MenuAutoLoot()
	{
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.14f, 0.14f, 1.00f));
		ImGui::TextUnformatted("Auto Loot");
		ImGui::PopStyleColor();
		ImGui::Spacing();

		ImGui::Toggle("Enable Auto Loot", &AutoLoot_Enable);
		ImGui::Spacing();

		ImGui::BeginGroupPanel("Settings", ImVec2(-FLT_MIN, 0.0f));

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1.00f));
		ImGui::Text("Range");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150.0f);
		ImGui::SliderFloat("##ALRange", &AutoLoot_Range, 1.0f, 30.0f, "%.0f m");

		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.85f, 0.85f, 1.00f));
		ImGui::Text("Interval");
		ImGui::PopStyleColor();
		ImGui::SameLine();
		ImGui::SetNextItemWidth(150.0f);
		ImGui::SliderFloat("##ALInterval", &AutoLoot_Interval, 0.1f, 3.0f, "%.1f s");

		ImGui::EndGroupPanel();
		ImGui::Spacing();

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.60f, 0.60f, 1.00f));
		ImGui::Text("Looted this session: %d", AutoLoot_Count);
		ImGui::PopStyleColor();
	}

} // namespace Cheat
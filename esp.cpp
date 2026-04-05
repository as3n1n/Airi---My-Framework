#include "pch.h"
#include "cheat.h"
#include "phasmo_helpers.h"
#include "phasmo_resolve.h"
#include "phasmo_offsets.h"
#include "phasmo_structs.h"
#include "global.h"
#include "imgui.h"
#include "logger.h"
#include <cfloat>
#include <math.h>
#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>

// ══════════════════════════════════════════════════════════
//  ESP.CPP — Phasmophobia
//  ESP: Ghost, Player, Evidence, FuseBox, EMF
// ══════════════════════════════════════════════════════════

namespace Cheat
{
    static Phasmo::NetworkPlayerSpot* GetPlayerSpot(Phasmo::Player* player);
    static void DrawTextShadow(ImDrawList* dl, ImVec2 pos, ImU32 color, const char* text);
    static bool WorldToScreen(const Phasmo::Vec3& world, ImVec2& out);
    static bool TryGetPlayerPosition(Phasmo::Player* player, Phasmo::Vec3& out);
    static void DrawPlayersWindowUnsafe();
    static void DrawEspDebugOverlay(ImDrawList* dl);
    static bool TryBuildRendererBounds(void* renderer, ImVec2& outMin, ImVec2& outMax);
    static bool DrawRendererBox(ImDrawList* dl, void* renderer, const char* label, const char* footer, ImU32 color);
    static void DrawPlayerBox(ImDrawList* dl, const ImVec2& head, const ImVec2& feet, const char* label, const char* footer, ImU32 color);
    // ─── Colors ─────────────────────────────────────────
    static const ImU32 COL_SHADOW   = IM_COL32(0, 0, 0, 180);

    static ImU32 GetEspColor(const float color[4])
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
    }

    static std::string ToLowerCopy(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return value;
    }

    static std::string JsonEscape(const std::string& value)
    {
        std::string out;
        out.reserve(value.size() + 8);
        for (char ch : value) {
            switch (ch) {
            case '\\': out += "\\\\"; break;
            case '\"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += ch; break;
            }
        }
        return out;
    }

    static const char* EvidenceTypeName(int evidenceType)
    {
        switch (evidenceType)
        {
        case 0: return "EMF 5";
        case 1: return "Spirit Box";
        case 2: return "Fingerprints";
        case 3: return "Ghost Writing";
        case 4: return "Ghost Orbs";
        case 5: return "Freezing Temps";
        case 6: return "D.O.T.S";
        case 7: return "Ultraviolet";
        case 8: return "Cursed Possession";
        default: return nullptr;
        }
    }

    struct Il2CppListPlayer
        : Phasmo::Il2CppObject
    {
        void* _items;    // 0x10
        int32_t _size;   // 0x18
        int32_t _version;// 0x1C
        void* _syncRoot; // 0x20
    };

    static ImGuiTextFilter s_playersWindowFilter;
    static ULONGLONG s_ghostEspSuspendUntilMs = 0;
    static int s_ghostEspCrashCount = 0;
    static bool s_espLastInGameFrame = false;
    static ULONGLONG s_espInGameEnteredAtMs = 0;
    static bool s_espDebugCameraOk = false;
    static bool s_espDebugW2sOk = false;
    static int s_espDebugGhostFound = 0;
    static int s_espDebugPlayersFound = 0;
    static int s_espDebugObjectsFound = 0;
    static int s_espDebugObjectsDrawn = 0;
    struct CachedObjectEspEntry
    {
        Phasmo::PhotonObjectInteract* interact = nullptr;
        void* transform = nullptr;
        void* renderer = nullptr;
        void* photonView = nullptr;
        Phasmo::Vec3 pos{};
        std::string displayName;
        std::string loweredName;
        int viewId = 0;
        ULONGLONG lastPosRefreshMs = 0;
        bool isDoor = false;
        bool isItem = false;
        bool isProp = false;
        bool isUsable = false;
    };
    static std::vector<CachedObjectEspEntry> s_cachedObjectEspEntries;
    static std::vector<CachedObjectEspEntry> s_visibleObjectEspEntries;
    static ULONGLONG s_cachedObjectEspNextRefreshMs = 0;
    static ULONGLONG s_objectEspVisibleRefreshMs = 0;
    static std::vector<std::string> s_objectEspJsonFilters;
    static ULONGLONG s_objectEspJsonNextReloadMs = 0;
    static size_t s_objectEspUpdateCursor = 0;
    constexpr uintptr_t PLAYER_HEAD_TRANSFORM_OFFSET = 0x48;
    constexpr uintptr_t PLAYER_ANIMATOR_OFFSET = 0x168;
    constexpr uintptr_t GHOST_MODEL_ANIMATOR_OFFSET = 0x48;

    using CameraGetMainFn = void* (*)(const void*);
    using CameraWorldToScreenPointInjectedFn = void (*)(void*, const Phasmo::Vec3*, int32_t, Phasmo::Vec3*, const void*);
    struct ProjectedPoint
    {
        bool valid = false;
        ImVec2 screen{};
        Phasmo::Vec3 world{};
    };

    enum HumanBoneId : int32_t
    {
        HB_Hips = 0,
        HB_LeftUpperLeg = 1,
        HB_RightUpperLeg = 2,
        HB_LeftLowerLeg = 3,
        HB_RightLowerLeg = 4,
        HB_LeftFoot = 5,
        HB_RightFoot = 6,
        HB_Spine = 7,
        HB_Chest = 8,
        HB_UpperChest = 9,
        HB_Neck = 10,
        HB_Head = 11,
        HB_LeftShoulder = 12,
        HB_RightShoulder = 13,
        HB_LeftUpperArm = 14,
        HB_RightUpperArm = 15,
        HB_LeftLowerArm = 16,
        HB_RightLowerArm = 17,
        HB_LeftHand = 18,
        HB_RightHand = 19
    };

    struct EspCameraFns
    {
        CameraGetMainFn getMain = nullptr;
        CameraWorldToScreenPointInjectedFn worldToScreenInjected = nullptr;
        bool resolved = false;
    };

    static EspCameraFns s_espCameraFns;

    static bool IsInGame()
    {
        Phasmo::LevelValues* values = Phasmo_GetLevelValues();
        return values && Phasmo_Safe(values, sizeof(Phasmo::LevelValues)) && values->inGame;
    }

    static Phasmo::GhostAI* FindGhostAIForESP()
    {
        if (Phasmo::GhostAI* ghost = Phasmo_GetGhostAI())
            return ghost;

        const PhasmoResolve::ClassRef ghostClass = PhasmoResolve::GetAssembly("Assembly-CSharp").GetClass("", "GhostAI");
        auto ghosts = ghostClass.FindObjectsByType<Phasmo::GhostAI>();
        if (ghosts.empty())
            ghosts = ghostClass.FindObjectsOfTypeAll<Phasmo::GhostAI>();

        for (Phasmo::GhostAI* ghost : ghosts) {
            if (ghost && Phasmo_Safe(ghost, sizeof(Phasmo::GhostAI)))
                return ghost;
        }

        return nullptr;
    }

    static Il2CppListPlayer* GetMapPlayerList()
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
        return Phasmo_Safe(list->_items, arrayBytes) ? list : nullptr;
    }

    static bool EnsureEspCameraFns()
    {
        if (s_espCameraFns.resolved)
            return s_espCameraFns.getMain && s_espCameraFns.worldToScreenInjected;

        s_espCameraFns.getMain = PhasmoResolve::ResolveCameraGetMain();
        s_espCameraFns.worldToScreenInjected = PhasmoResolve::ResolveCameraWorldToScreenInjected();
        s_espCameraFns.resolved = true;
        return s_espCameraFns.getMain && s_espCameraFns.worldToScreenInjected;
    }

    static void* GetMainCamera()
    {
        if (!EnsureEspCameraFns())
            return nullptr;

        void* camera = s_espCameraFns.getMain(nullptr);
        return (camera && Phasmo_Safe(camera, 0x80)) ? camera : nullptr;
    }

    static void* GetUnityTypeObject(const char* namespaze, const char* name)
    {
        if (void* typeObject = PhasmoResolve::GetUnityTypeObject("UnityEngine.AnimationModule", namespaze, name))
            return typeObject;
        if (void* typeObject = PhasmoResolve::GetUnityTypeObject("UnityEngine.CoreModule", namespaze, name))
            return typeObject;
        return nullptr;
    }

    static void* GetAnimatorTypeObject()
    {
        static void* typeObject = nullptr;
        static bool resolved = false;
        if (!resolved) {
            typeObject = GetUnityTypeObject("UnityEngine", "Animator");
            resolved = true;
        }
        return typeObject;
    }

    static void* GetSkinnedMeshRendererTypeObject()
    {
        static void* typeObject = nullptr;
        static bool resolved = false;
        if (!resolved) {
            typeObject = GetUnityTypeObject("UnityEngine", "SkinnedMeshRenderer");
            resolved = true;
        }
        return typeObject;
    }

    static void* GetRendererTypeObject()
    {
        static void* typeObject = nullptr;
        static bool resolved = false;
        if (!resolved) {
            typeObject = GetUnityTypeObject("UnityEngine", "Renderer");
            resolved = true;
        }
        return typeObject;
    }

    static void* TryGetComponentByTypeRaw(void* component, void* typeObject)
    {
        if (!component || !typeObject || !Phasmo_Safe(component, 0x20))
            return nullptr;

        return PhasmoResolve::GetComponent(component, typeObject);
    }

    static void* TryGetComponentInChildrenByTypeRaw(void* component, void* typeObject, bool includeInactive)
    {
        if (!component || !typeObject || !Phasmo_Safe(component, 0x20))
            return nullptr;

        return PhasmoResolve::GetComponentInChildren(component, typeObject, includeInactive);
    }

    static void* TryGetAnimatorBoneTransformRaw(void* animator, int32_t boneId)
    {
        if (!animator || !Phasmo_Safe(animator, 0x40))
            return nullptr;

        return PhasmoResolve::GetAnimatorBoneTransform(animator, boneId);
    }

    static void* TryGetSkinnedMeshBonesRaw(void* renderer)
    {
        if (!renderer || !Phasmo_Safe(renderer, 0x40))
            return nullptr;

        return PhasmoResolve::GetSkinnedMeshBones(renderer);
    }

    static void* TryGetSkinnedMeshRootBoneRaw(void* renderer)
    {
        if (!renderer || !Phasmo_Safe(renderer, 0x40))
            return nullptr;

        return PhasmoResolve::GetSkinnedMeshRootBone(renderer);
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

    static bool IsLocalMapPlayer(Phasmo::Player* player)
    {
        if (!player)
            return false;

        if (Phasmo::Network* network = Phasmo_GetNetwork()) {
            if (Phasmo_Safe(network, sizeof(Phasmo::Network)) && network->localPlayer) {
                if (network->localPlayer == player)
                    return true;
            }
        }

        if (!player->photonView || !Phasmo_Safe(player->photonView, 0x40))
            return false;

        return Phasmo_Call<bool>(Phasmo::RVA_PV_get_IsMine, player->photonView, nullptr);
    }

    static int GetMapPlayerActorNumber(Phasmo::Player* player)
    {
        if (!player)
            return -1;

        if (Phasmo::NetworkPlayerSpot* spot = GetPlayerSpot(player)) {
            if (spot->photonPlayer && Phasmo_Safe(spot->photonPlayer, Phasmo::PP_ACTOR_NUMBER + sizeof(int32_t))) {
                return *reinterpret_cast<int32_t*>(
                    reinterpret_cast<uint8_t*>(spot->photonPlayer) + Phasmo::PP_ACTOR_NUMBER);
            }
        }

        if (!player->photonView || !Phasmo_Safe(player->photonView, 0x40))
            return -1;

        void* owner = Phasmo_Call<void*>(Phasmo::RVA_PV_get_Owner, player->photonView, nullptr);
        if (!owner || !Phasmo_Safe(owner, 0x20))
            return -1;

        return *reinterpret_cast<int32_t*>(
            reinterpret_cast<uint8_t*>(owner) + Phasmo::PP_ACTOR_NUMBER);
    }

    static Phasmo::Player* FindLocalPlayer()
    {
        Il2CppListPlayer* list = GetMapPlayerList();
        if (list) {
            for (int32_t i = 0; i < list->_size; ++i) {
                Phasmo::Player* player = GetMapPlayerAt(list, i);
                if (IsLocalMapPlayer(player))
                    return player;
            }
        }

        Phasmo::Network* network = Phasmo_GetNetwork();
        if (network && network->localPlayer && Phasmo_Safe(network->localPlayer, sizeof(Phasmo::Player)))
            return reinterpret_cast<Phasmo::Player*>(network->localPlayer);

        return nullptr;
    }

    static Il2CppListPlayer* GetNetworkPlayerSpotList()
    {
        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return nullptr;

        void* listPtr = network->playerSpots;
        if (!listPtr || !Phasmo_Safe(listPtr, sizeof(Il2CppListPlayer)))
            return nullptr;

        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(listPtr);
        if (!Phasmo_Safe(list, sizeof(Il2CppListPlayer)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return nullptr;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        return Phasmo_Safe(list->_items, arrayBytes) ? list : nullptr;
    }

    static const char* RoleName(Phasmo::PlayerRoleType role)
    {
        return Phasmo::GetRoleName(role);
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

    static Phasmo::NetworkPlayerSpot* GetNetworkSpotAt(int32_t index)
    {
        Il2CppListPlayer* list = GetNetworkPlayerSpotList();
        if (!list || index < 0 || index >= list->_size)
            return nullptr;

        auto** entries = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);
        Phasmo::NetworkPlayerSpot* spot = entries[index];
        return Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)) ? spot : nullptr;
    }

    static Phasmo::NetworkPlayerSpot* GetPlayerSpot(Phasmo::Player* player)
    {
        Il2CppListPlayer* list = GetNetworkPlayerSpotList();
        if (!list || !player)
            return nullptr;

        auto** entries = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::NetworkPlayerSpot* spot = entries[i];
            if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
                continue;
            if (spot->player == player)
                return spot;
        }

        return nullptr;
    }

    static std::string GetSpotName(Phasmo::NetworkPlayerSpot* spot)
    {
        if (!spot || !spot->accountName)
            return {};

        return Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(spot->accountName));
    }

    static std::string GetRoomName(void* roomPtr)
    {
        if (!roomPtr || !Phasmo_Safe(roomPtr, 0x68))
            return {};

        auto* room = reinterpret_cast<Phasmo::LevelRoom*>(roomPtr);
        if (!room->roomName)
            return {};

        return Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(room->roomName));
    }

    static std::string GetPlayerDisplayName(Phasmo::Player* player, int32_t fallbackIndex)
    {
        if (Phasmo::NetworkPlayerSpot* spot = GetPlayerSpot(player)) {
            std::string name = GetSpotName(spot);
            if (!name.empty())
                return name;
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

        return "Slot " + std::to_string(fallbackIndex);
    }

    static void* GetComponentTransform(void* component)
    {
        if (!component || !Phasmo_Safe(component, 0x20))
            return nullptr;

        void* transform = PhasmoResolve::GetTransform(component);
        return (transform && Phasmo_Safe(transform, 0xC0)) ? transform : nullptr;
    }

    static void* GetAnimatorFromComponent(void* component)
    {
        void* typeObject = GetAnimatorTypeObject();
        if (!typeObject)
            return nullptr;

        void* animator = PhasmoResolve::GetComponent(component, typeObject);
        if (!animator)
            animator = PhasmoResolve::GetComponentInChildren(component, typeObject, true);

        return (animator && Phasmo_Safe(animator, 0x40)) ? animator : nullptr;
    }

    static void* GetSkinnedMeshRendererFromComponent(void* component)
    {
        void* typeObject = GetSkinnedMeshRendererTypeObject();
        if (!typeObject)
            return nullptr;

        void* renderer = PhasmoResolve::GetComponent(component, typeObject);
        if (!renderer)
            renderer = PhasmoResolve::GetComponentInChildren(component, typeObject, true);

        return (renderer && Phasmo_Safe(renderer, 0x40)) ? renderer : nullptr;
    }

    static void* GetRendererFromComponent(void* component)
    {
        void* typeObject = GetRendererTypeObject();
        if (!typeObject)
            return nullptr;

        void* renderer = PhasmoResolve::GetComponent(component, typeObject);
        if (!renderer)
            renderer = PhasmoResolve::GetComponentInChildren(component, typeObject, true);

        return (renderer && Phasmo_Safe(renderer, 0x40)) ? renderer : nullptr;
    }

    static void* GetPlayerCharacterTransform(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            return nullptr;

        auto character = reinterpret_cast<Phasmo::PlayerCharacter*>(player->playerCharacter);
        if (!character || !Phasmo_Safe(character, sizeof(Phasmo::PlayerCharacter)))
            return nullptr;

        if (void* transform = GetComponentTransform(character))
            return transform;

        void* transform = character->transform;
        return (transform && Phasmo_Safe(transform, 0xC0)) ? transform : nullptr;
    }

    static void* GetPlayerHeadTransform(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, PLAYER_HEAD_TRANSFORM_OFFSET + sizeof(void*)))
            return nullptr;

        void* headTransform = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + PLAYER_HEAD_TRANSFORM_OFFSET);
        if (headTransform && Phasmo_Safe(headTransform, 0xC0))
            return headTransform;

        if (player->headObject && Phasmo_Safe(player->headObject, 0x20)) {
            if (void* transform = GetComponentTransform(player->headObject))
                return transform;
        }

        return nullptr;
    }

    static void* GetPlayerAnimator(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            return nullptr;

        if (Phasmo_Safe(player, PLAYER_ANIMATOR_OFFSET + sizeof(void*))) {
            void* animator = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + PLAYER_ANIMATOR_OFFSET);
            if (animator && Phasmo_Safe(animator, 0x40))
                return animator;
        }

        auto character = reinterpret_cast<Phasmo::PlayerCharacter*>(player->playerCharacter);
        if (!character || !Phasmo_Safe(character, sizeof(Phasmo::PlayerCharacter)))
            return nullptr;

        if (void* animator = GetAnimatorFromComponent(character))
            return animator;

        if (character->meshRenderer && Phasmo_Safe(character->meshRenderer, 0x40)) {
            if (void* animator = GetAnimatorFromComponent(character->meshRenderer))
                return animator;
        }

        return nullptr;
    }

    static void* GetPlayerSkinnedMeshRenderer(Phasmo::Player* player)
    {
        if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)))
            return nullptr;

        if (void* animator = GetPlayerAnimator(player)) {
            if (void* renderer = GetSkinnedMeshRendererFromComponent(animator))
                return renderer;
        }

        auto character = reinterpret_cast<Phasmo::PlayerCharacter*>(player->playerCharacter);
        if (!character || !Phasmo_Safe(character, sizeof(Phasmo::PlayerCharacter)))
            return nullptr;

        if (character->meshRenderer && Phasmo_Safe(character->meshRenderer, 0x40)) {
            if (void* renderer = GetSkinnedMeshRendererFromComponent(character->meshRenderer))
                return renderer;
            return character->meshRenderer;
        }

        return GetSkinnedMeshRendererFromComponent(character);
    }

    static bool TryGetPlayerPosition(Phasmo::Player* player, Phasmo::Vec3& out)
    {
        if (!player)
            return false;

        if (void* headTransform = GetPlayerHeadTransform(player)) {
            Phasmo::Vec3 headPos = Phasmo_GetPosition(headTransform);
            if (!(headPos.x == 0.0f && headPos.y == 0.0f && headPos.z == 0.0f)) {
                out = headPos;
                out.y -= 1.60f;
                return true;
            }
        }

        void* transform = GetPlayerCharacterTransform(player);
        if (!transform)
            return false;

        out = Phasmo_GetPosition(transform);
        return true;
    }

    static bool TryProjectTransform(void* transform, ProjectedPoint& out)
    {
        if (!transform || !Phasmo_Safe(transform, 0xC0))
            return false;

        out.world = Phasmo_GetPosition(transform);
        if (!WorldToScreen(out.world, out.screen))
            return false;

        out.valid = true;
        return true;
    }

    static bool WorldToScreen(const Phasmo::Vec3& world, ImVec2& out)
    {
        void* camera = GetMainCamera();
        s_espDebugCameraOk = (camera != nullptr);
        if (!camera)
            return false;

        Phasmo::Vec3 screen{};
        if (!PhasmoResolve::WorldToScreen(camera, world, screen))
            return false;

        ImGuiIO& io = ImGui::GetIO();
        const int32_t pixelWidth = PhasmoResolve::GetCameraPixelWidth(camera);
        const int32_t pixelHeight = PhasmoResolve::GetCameraPixelHeight(camera);

        float scaleX = 1.0f;
        float scaleY = 1.0f;
        if (pixelWidth > 0 && pixelHeight > 0) {
            scaleX = io.DisplaySize.x / static_cast<float>(pixelWidth);
            scaleY = io.DisplaySize.y / static_cast<float>(pixelHeight);
        }

        screen.x *= scaleX;
        screen.y *= scaleY;

        if (screen.x < -200.0f || screen.x > io.DisplaySize.x + 200.0f)
            return false;
        if (screen.y < -200.0f || screen.y > io.DisplaySize.y + 200.0f)
            return false;

        out.x = screen.x;
        out.y = io.DisplaySize.y - screen.y;
        s_espDebugW2sOk = true;
        return true;
    }

    static void ExpandScreenBounds(const ImVec2& point, ImVec2& min, ImVec2& max)
    {
        min.x = (point.x < min.x) ? point.x : min.x;
        min.y = (point.y < min.y) ? point.y : min.y;
        max.x = (point.x > max.x) ? point.x : max.x;
        max.y = (point.y > max.y) ? point.y : max.y;
    }

    static void DrawOutlinedLine(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 color, float thickness = 1.5f)
    {
        const float espThickness = std::clamp(Cheat::VIS_ESPLineThickness, 1.0f, 3.5f);
        const float finalThickness = (thickness > 0.0f ? thickness : espThickness);
        dl->AddLine(a, b, IM_COL32(0, 0, 0, 200), finalThickness + 2.0f);
        dl->AddLine(a, b, color, finalThickness);
    }

    static void DrawEspBounds(ImDrawList* dl, const ImVec2& min, const ImVec2& max, const char* label, const char* footer, ImU32 color)
    {
        const float width = max.x - min.x;
        const float height = max.y - min.y;
        if (width < 8.0f || height < 18.0f)
            return;

        const float cornerLen = std::clamp(width * 0.28f, 8.0f, 18.0f);
        const float lineThickness = std::clamp(Cheat::VIS_ESPLineThickness, 1.0f, 3.0f);
        const float glowThickness = lineThickness + 3.0f;
        const ImU32 shadowColor = IM_COL32(0, 0, 0, 185);
        dl->AddRectFilled(min, max, IM_COL32(8, 12, 18, 18), 2.0f);

        auto drawCorner = [&](const ImVec2& a, const ImVec2& b, const ImVec2& c)
        {
            dl->AddLine(a, b, shadowColor, glowThickness);
            dl->AddLine(a, c, shadowColor, glowThickness);
            dl->AddLine(a, b, color, lineThickness);
            dl->AddLine(a, c, color, lineThickness);
        };

        drawCorner(ImVec2(min.x, min.y), ImVec2(min.x + cornerLen, min.y), ImVec2(min.x, min.y + cornerLen));
        drawCorner(ImVec2(max.x, min.y), ImVec2(max.x - cornerLen, min.y), ImVec2(max.x, min.y + cornerLen));
        drawCorner(ImVec2(min.x, max.y), ImVec2(min.x + cornerLen, max.y), ImVec2(min.x, max.y - cornerLen));
        drawCorner(ImVec2(max.x, max.y), ImVec2(max.x - cornerLen, max.y), ImVec2(max.x, max.y - cornerLen));

        const float centerX = (min.x + max.x) * 0.5f;
        if (label && *label) {
            const ImVec2 size = ImGui::CalcTextSize(label);
            const ImVec2 textMin(centerX - size.x * 0.5f - 5.0f, min.y - size.y - 8.0f);
            const ImVec2 textMax(centerX + size.x * 0.5f + 5.0f, min.y - 2.0f);
            dl->AddRectFilled(textMin, textMax, IM_COL32(12, 10, 16, 170), 3.0f);
            DrawTextShadow(dl, ImVec2(centerX - size.x * 0.5f, min.y - size.y - 5.0f), color, label);
        }

        if (footer && *footer) {
            const ImVec2 size = ImGui::CalcTextSize(footer);
            const ImVec2 textMin(centerX - size.x * 0.5f - 5.0f, max.y + 2.0f);
            const ImVec2 textMax(centerX + size.x * 0.5f + 5.0f, max.y + size.y + 8.0f);
            dl->AddRectFilled(textMin, textMax, IM_COL32(12, 10, 16, 150), 3.0f);
            DrawTextShadow(dl, ImVec2(centerX - size.x * 0.5f, max.y + 4.0f), IM_COL32(235, 235, 235, 255), footer);
        }
    }

    static bool TryProjectAnimatorBone(void* animator, int32_t boneId, ProjectedPoint& out)
    {
        void* boneTransform = TryGetAnimatorBoneTransformRaw(animator, boneId);
        return TryProjectTransform(boneTransform, out);
    }

    static bool TryBuildSkinnedMeshBounds(void* renderer, ImVec2& outMin, ImVec2& outMax)
    {
        void* bonesRaw = TryGetSkinnedMeshBonesRaw(renderer);
        auto* bones = reinterpret_cast<Phasmo::Il2CppArray*>(bonesRaw);
        if (!bones || !Phasmo_Safe(bones, Phasmo::ARRAY_FIRST_ELEMENT + sizeof(void*)))
            return false;

        const int32_t count = bones->max_length;
        if (count <= 0 || count > 256) {
            void* rootBone = TryGetSkinnedMeshRootBoneRaw(renderer);
            if (!rootBone)
                return false;

            ProjectedPoint root{};
            if (!TryProjectTransform(rootBone, root))
                return false;

            outMin = ImVec2(root.screen.x - 24.0f, root.screen.y - 70.0f);
            outMax = ImVec2(root.screen.x + 24.0f, root.screen.y + 18.0f);
            return true;
        }

        auto** entries = reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(bones) + Phasmo::ARRAY_FIRST_ELEMENT);
        bool any = false;
        ImVec2 min(FLT_MAX, FLT_MAX);
        ImVec2 max(-FLT_MAX, -FLT_MAX);

        for (int32_t i = 0; i < count; ++i) {
            void* bone = entries[i];
            ProjectedPoint projected{};
            if (!TryProjectTransform(bone, projected))
                continue;

            ExpandScreenBounds(projected.screen, min, max);
            any = true;
        }

        if (!any)
            return false;

        outMin = ImVec2(min.x - 6.0f, min.y - 6.0f);
        outMax = ImVec2(max.x + 6.0f, max.y + 6.0f);
        return true;
    }

    static bool DrawHumanoidSkeleton(ImDrawList* dl, void* animator, const char* label, const char* footer, ImU32 color)
    {
        if (!animator || !Phasmo_Safe(animator, 0x40))
            return false;

        struct BoneEntry
        {
            int32_t id;
            ProjectedPoint point;
        };

        std::array<BoneEntry, 17> bones = { {
            { HB_Head, {} },
            { HB_Neck, {} },
            { HB_Chest, {} },
            { HB_Hips, {} },
            { HB_LeftShoulder, {} },
            { HB_LeftUpperArm, {} },
            { HB_LeftLowerArm, {} },
            { HB_LeftHand, {} },
            { HB_RightShoulder, {} },
            { HB_RightUpperArm, {} },
            { HB_RightLowerArm, {} },
            { HB_RightHand, {} },
            { HB_LeftUpperLeg, {} },
            { HB_LeftLowerLeg, {} },
            { HB_LeftFoot, {} },
            { HB_RightUpperLeg, {} },
            { HB_RightLowerLeg, {} }
        } };

        ProjectedPoint rightFoot{};
        bool rightFootValid = TryProjectAnimatorBone(animator, HB_RightFoot, rightFoot);

        int validCount = 0;
        ImVec2 min(FLT_MAX, FLT_MAX);
        ImVec2 max(-FLT_MAX, -FLT_MAX);

        for (auto& bone : bones) {
            bone.point.valid = TryProjectAnimatorBone(animator, bone.id, bone.point);
            if (!bone.point.valid && bone.id == HB_Chest)
                bone.point.valid = TryProjectAnimatorBone(animator, HB_UpperChest, bone.point);
            if (!bone.point.valid)
                continue;

            ExpandScreenBounds(bone.point.screen, min, max);
            ++validCount;
        }

        if (rightFootValid) {
            ExpandScreenBounds(rightFoot.screen, min, max);
            ++validCount;
        }

        if (validCount < 6)
            return false;

        auto findBone = [&](int32_t boneId) -> const ProjectedPoint* {
            for (const auto& bone : bones) {
                if (bone.id == boneId && bone.point.valid)
                    return &bone.point;
            }
            return nullptr;
        };

        auto drawLink = [&](int32_t a, int32_t b) {
            const ProjectedPoint* pa = findBone(a);
            const ProjectedPoint* pb = findBone(b);
            if (pa && pb)
                DrawOutlinedLine(dl, pa->screen, pb->screen, color);
        };

        drawLink(HB_Head, HB_Neck);
        drawLink(HB_Neck, HB_Chest);
        drawLink(HB_Chest, HB_Hips);
        drawLink(HB_Chest, HB_LeftShoulder);
        drawLink(HB_LeftShoulder, HB_LeftUpperArm);
        drawLink(HB_LeftUpperArm, HB_LeftLowerArm);
        drawLink(HB_LeftLowerArm, HB_LeftHand);
        drawLink(HB_Chest, HB_RightShoulder);
        drawLink(HB_RightShoulder, HB_RightUpperArm);
        drawLink(HB_RightUpperArm, HB_RightLowerArm);
        drawLink(HB_RightLowerArm, HB_RightHand);
        drawLink(HB_Hips, HB_LeftUpperLeg);
        drawLink(HB_LeftUpperLeg, HB_LeftLowerLeg);
        drawLink(HB_LeftLowerLeg, HB_LeftFoot);
        drawLink(HB_Hips, HB_RightUpperLeg);
        drawLink(HB_RightUpperLeg, HB_RightLowerLeg);
        if (const ProjectedPoint* knee = findBone(HB_RightLowerLeg)) {
            if (rightFootValid)
                DrawOutlinedLine(dl, knee->screen, rightFoot.screen, color);
        }

        DrawEspBounds(dl, ImVec2(min.x - 6.0f, min.y - 6.0f), ImVec2(max.x + 6.0f, max.y + 6.0f), label, footer, color);
        return true;
    }

    static float Vec3Distance(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return sqrtf(dx * dx + dy * dy + dz * dz);
    }

    static float Vec3DistanceSq(const Phasmo::Vec3& a, const Phasmo::Vec3& b)
    {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    // ─── Helper: Draw text with shadow ──────────────────
    static void DrawTextShadow(ImDrawList* dl, ImVec2 pos, ImU32 color, const char* text)
    {
        dl->AddText(ImVec2(pos.x + 1, pos.y + 1), COL_SHADOW, text);
        dl->AddText(pos, color, text);
    }

    static void* GetGhostModelTransform(void* ghostModel)
    {
        if (!ghostModel || !Phasmo_Safe(ghostModel, 0x80))
            return nullptr;

        constexpr uintptr_t GM_TRANSFORM_PRIMARY = 0x78;
        constexpr uintptr_t GM_TRANSFORM_ALT_A = 0xB8;
        constexpr uintptr_t GM_TRANSFORM_ALT_B = 0xC0;
        constexpr uintptr_t GM_TRANSFORM_ALT_C = 0xC8;

        const uintptr_t candidates[] = {
            GM_TRANSFORM_PRIMARY,
            GM_TRANSFORM_ALT_A,
            GM_TRANSFORM_ALT_B,
            GM_TRANSFORM_ALT_C
        };

        for (uintptr_t offset : candidates) {
            void* transform = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ghostModel) + offset);
            if (transform && Phasmo_Safe(transform, 0xC0))
                return transform;
        }

        void* transform = PhasmoResolve::GetTransform(ghostModel);
        if (transform && Phasmo_Safe(transform, 0xC0))
            return transform;

        return nullptr;
    }

    static void* GetGhostAnimator(void* ghostModel)
    {
        if (!ghostModel || !Phasmo_Safe(ghostModel, 0x50))
            return nullptr;

        void* animator = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(ghostModel) + GHOST_MODEL_ANIMATOR_OFFSET);
        if (animator && Phasmo_Safe(animator, 0x40))
            return animator;

        return GetAnimatorFromComponent(ghostModel);
    }

    static std::string GetGhostDisplayName(Phasmo::GhostAI* ghost)
    {
        if (!ghost || !ghost->ghostInfo || !Phasmo_Safe(ghost->ghostInfo, sizeof(Phasmo::GhostInfo)))
            return {};

        void* namePtr = ghost->ghostInfo->ghostTypeInfo.ghostName;
        if (!namePtr || !Phasmo_Safe(namePtr, sizeof(Il2CppString)))
            return {};

        return Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(namePtr));
    }

    static Il2CppString* TryGetUnityObjectNameRaw(void* object)
    {
        using Fn = Il2CppString * (*)(void*, const MethodInfo*);
        static Fn fn = nullptr;
        static bool resolved = false;
        if (!resolved) {
            fn = Phasmo_ResolveMethod<Fn>("UnityEngine", "Object", "get_name", 0, "UnityEngine.CoreModule");
            if (!fn && Phasmo_Base())
                fn = reinterpret_cast<Fn>(Phasmo_Base() + 0x4BC9190);
            resolved = true;
        }

        if (!fn || !object || !Phasmo_Safe(object, 0x20))
            return nullptr;

        __try {
            return fn(object, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return nullptr;
        }
    }

    static std::string GetUnityObjectName(void* object)
    {
        Il2CppString* raw = TryGetUnityObjectNameRaw(object);
        return raw ? Phasmo_StringToUtf8(raw) : std::string{};
    }

    static int GetPhotonViewId(void* photonView)
    {
        using Fn = int32_t(*)(void*, const MethodInfo*);
        static Fn fn = nullptr;
        static bool resolved = false;
        if (!resolved) {
            fn = Phasmo_ResolveMethod<Fn>("Photon.Pun", "PhotonView", "get_ViewID", 0, "PhotonUnityNetworking");
            if (!fn && Phasmo_Base())
                fn = reinterpret_cast<Fn>(Phasmo_Base() + 0x1A05DC0);
            resolved = true;
        }

        if (!fn || !photonView || !Phasmo_Safe(photonView, 0x20))
            return 0;

        __try {
            return fn(photonView, nullptr);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return 0;
        }
    }

    static std::string GetObjectEspName(Phasmo::PhotonObjectInteract* interact, void* transform, void* renderer)
    {
        if (std::string value = GetUnityObjectName(interact); !value.empty())
            return value;

        if (void* gameObject = PhasmoResolve::GetGameObject(interact)) {
            if (std::string value = GetUnityObjectName(gameObject); !value.empty())
                return value;
        }

        if (transform) {
            if (std::string value = GetUnityObjectName(transform); !value.empty())
                return value;
            if (void* gameObject = PhasmoResolve::GetGameObject(transform)) {
                if (std::string value = GetUnityObjectName(gameObject); !value.empty())
                    return value;
            }
        }

        if (renderer) {
            if (std::string value = GetUnityObjectName(renderer); !value.empty())
                return value;
            if (void* gameObject = PhasmoResolve::GetGameObject(renderer)) {
                if (std::string value = GetUnityObjectName(gameObject); !value.empty())
                    return value;
            }
        }

        return {};
    }

    static std::string GetObjectEspFilterPath()
    {
        if (Cheat::VIS_ObjectESPJsonPath[0] != '\0')
            return Cheat::VIS_ObjectESPJsonPath;
        return GetAiriSubdirA("ESP") + "\\object_esp_filter.json";
    }

    static std::string GetObjectEspIdsPath()
    {
        return GetAiriSubdirA("ESP") + "\\object_esp_ids.json";
    }

    static void EnsureObjectEspFilterFile()
    {
        const std::string path = GetObjectEspFilterPath();
        if (std::filesystem::exists(path))
            return;

        std::ofstream out(path, std::ios::out | std::ios::trunc);
        if (!out)
            return;

        out << "{\n";
        out << "  \"show_all_if_empty\": true,\n";
        out << "  \"items\": [\n";
        out << "    \"BasketBall\",\n";
        out << "    \"Crutch\"\n";
        out << "  ]\n";
        out << "}\n";
    }

    static void ReloadObjectEspJsonFilter()
    {
        s_objectEspJsonFilters.clear();
        EnsureObjectEspFilterFile();

        std::ifstream in(GetObjectEspFilterPath());
        if (!in)
            return;

        std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const size_t itemsPos = contents.find("\"items\"");
        if (itemsPos == std::string::npos)
            return;
        const size_t arrayStart = contents.find('[', itemsPos);
        const size_t arrayEnd = contents.find(']', arrayStart == std::string::npos ? itemsPos : arrayStart);
        if (arrayStart == std::string::npos || arrayEnd == std::string::npos || arrayEnd <= arrayStart)
            return;

        bool inString = false;
        std::string current;
        for (size_t i = arrayStart + 1; i < arrayEnd; ++i) {
            char ch = contents[i];
            if (inString) {
                if (ch == '\\' && (i + 1) < arrayEnd) {
                    current += contents[++i];
                    continue;
                }
                if (ch == '"') {
                    if (!current.empty())
                        s_objectEspJsonFilters.push_back(ToLowerCopy(current));
                    current.clear();
                    inString = false;
                    continue;
                }
                current += ch;
            }
            else if (ch == '"') {
                inString = true;
            }
        }
    }

    static void ExportObjectEspIdsJson(const std::vector<CachedObjectEspEntry>& entries)
    {
        std::ofstream out(GetObjectEspIdsPath(), std::ios::out | std::ios::trunc);
        if (!out)
            return;

        out << "{\n";
        out << "  \"count\": " << entries.size() << ",\n";
        out << "  \"entities\": [\n";
        for (size_t i = 0; i < entries.size(); ++i) {
            const CachedObjectEspEntry& entry = entries[i];
            out << "    {\n";
            out << "      \"name\": \"" << JsonEscape(entry.displayName) << "\",\n";
            out << "      \"view_id\": " << entry.viewId << ",\n";
            out << "      \"pointer\": \"" << entry.interact << "\",\n";
            out << "      \"is_door\": " << (entry.isDoor ? "true" : "false") << ",\n";
            out << "      \"is_item\": " << (entry.isItem ? "true" : "false") << ",\n";
            out << "      \"is_prop\": " << (entry.isProp ? "true" : "false") << ",\n";
            out << "      \"is_usable\": " << (entry.isUsable ? "true" : "false") << "\n";
            out << "    }" << (i + 1 < entries.size() ? "," : "") << "\n";
        }
        out << "  ]\n";
        out << "}\n";
    }

    // ─── Helper: Draw labeled position indicator ────────
    // ══════════════════════════════════════════════════════
    //  GHOST ESP
    // ══════════════════════════════════════════════════════
    static bool DrawWireBox3D(ImDrawList* dl, const Phasmo::Vec3& origin, float width, float height, float depth, ImU32 color);
    static void DrawFallbackEntityBox(ImDrawList* dl, const Phasmo::Vec3& origin, float distance, const char* label, const char* footer, ImU32 color);

    static void DrawGhostESP(ImDrawList* dl)
    {
        if (!VIS_GhostESP) return;
        if (GetTickCount64() < s_ghostEspSuspendUntilMs)
            return;
        const ImU32 ghostColor = GetEspColor(Cheat::VIS_ESPColorGhost);

        const ULONGLONG nowMs = GetTickCount64();
        const bool inGameWarmup = s_espInGameEnteredAtMs != 0 && (nowMs - s_espInGameEnteredAtMs) < 5000;
        if (inGameWarmup)
            return;

        Phasmo::GhostAI* ghost = FindGhostAIForESP();
        s_espDebugGhostFound = (ghost != nullptr) ? 1 : 0;
        if (!ghost || !Phasmo_Safe(ghost, sizeof(Phasmo::GhostAI))) return;

        Phasmo::Vec3 ghostPos = ghost->lastKnownPlayerPos;
        bool haveGhostPos = !(ghostPos.x == 0.0f && ghostPos.y == 0.0f && ghostPos.z == 0.0f);

        if (!haveGhostPos) {
            if (void* modelTransform = GetGhostModelTransform(ghost->ghostModel)) {
                ghostPos = Phasmo_GetPosition(modelTransform);
                haveGhostPos = !(ghostPos.x == 0.0f && ghostPos.y == 0.0f && ghostPos.z == 0.0f);
            }
        }

        if (!haveGhostPos && ghost->ghostInfo && Phasmo_Safe(ghost->ghostInfo, sizeof(Phasmo::GhostInfo))) {
            if (ghost->ghostInfo->levelRoom) {
                void* roomTransform = GetComponentTransform(ghost->ghostInfo->levelRoom);
                if (roomTransform) {
                    ghostPos = Phasmo_GetPosition(roomTransform);
                    haveGhostPos = !(ghostPos.x == 0.0f && ghostPos.y == 0.0f && ghostPos.z == 0.0f);
                }
            }
        }

        if (!haveGhostPos)
            return;

        Phasmo::Player* localPlayer = FindLocalPlayer();
        Phasmo::Vec3 localPos{};
        const bool haveLocal = TryGetPlayerPosition(localPlayer, localPos);
        const float distance = haveLocal ? Vec3Distance(localPos, ghostPos) : 0.0f;
        const std::string ghostName = GetGhostDisplayName(ghost);
        const float ghostSpeed = (ghost->ghostInfo && Phasmo_Safe(ghost->ghostInfo, sizeof(Phasmo::GhostInfo)))
            ? ghost->ghostInfo->ghostSpeed
            : 0.0f;

        char label[128];
        char footer[96];
        if (!ghostName.empty())
            snprintf(label, sizeof(label), "%s [%s]", ghostName.c_str(), GhostStateName(ghost->state));
        else
            snprintf(label, sizeof(label), "Ghost [%s]", GhostStateName(ghost->state));
        if (haveLocal)
            snprintf(footer, sizeof(footer), "Actor %d | %.0fm | %.2f m/s", ghost->targetPlayerId, distance, ghostSpeed);
        else
            snprintf(footer, sizeof(footer), "Actor %d | %.2f m/s", ghost->targetPlayerId, ghostSpeed);

        if (Cheat::VIS_GhostSkeletonESP) {
            if (void* animator = GetGhostAnimator(ghost->ghostModel)) {
                if (DrawHumanoidSkeleton(dl, animator, label, footer, ghostColor))
                    return;
            }
        }

        Phasmo::Vec3 ghostHead = ghostPos;
        ghostHead.y += 1.55f;
        Phasmo::Vec3 ghostFeet = ghostPos;
        ghostFeet.y -= 0.05f;
        ImVec2 headScreen{};
        ImVec2 feetScreen{};
        if (WorldToScreen(ghostHead, headScreen) && WorldToScreen(ghostFeet, feetScreen)) {
            DrawPlayerBox(dl, headScreen, feetScreen, label, footer, ghostColor);
            return;
        }

        DrawFallbackEntityBox(dl, ghostPos, distance, label, footer, ghostColor);
    }

    static void DrawObjectESP(ImDrawList* dl)
    {
        if (!VIS_ObjectESP)
            return;

        const ImU32 objectColor = GetEspColor(Cheat::VIS_ESPColorObject);
        constexpr ULONGLONG kObjectScanRefreshMs = 2000;
        const ULONGLONG nowMs = GetTickCount64();
        const PhasmoResolve::ClassRef interactClass = PhasmoResolve::GetAssembly("Assembly-CSharp").GetClass("", "PhotonObjectInteract");
        if (s_cachedObjectEspEntries.empty() || nowMs >= s_cachedObjectEspNextRefreshMs || Cheat::VIS_ObjectESPExportJsonNow) {
            s_cachedObjectEspEntries.clear();
            auto props = interactClass.FindObjectsByType<Phasmo::PhotonObjectInteract>();
            if (props.empty())
                props = interactClass.FindObjectsOfTypeAll<Phasmo::PhotonObjectInteract>();

            s_cachedObjectEspEntries.reserve(props.size());
            for (Phasmo::PhotonObjectInteract* interact : props) {
                if (!interact || !Phasmo_Safe(interact, sizeof(Phasmo::PhotonObjectInteract)))
                    continue;

                CachedObjectEspEntry entry{};
                entry.interact = interact;
                entry.isDoor = interactClass.GetValue<void*>(interact, "door") != nullptr;
                entry.isItem = interactClass.GetValue<bool>(interact, "isItem");
                entry.isProp = interactClass.GetValue<bool>(interact, "isProp");
                entry.isUsable = interactClass.GetValue<bool>(interact, "isUsable") || entry.isItem;
                entry.photonView = interactClass.GetValue<void*>(interact, "photonView");

                entry.transform = interactClass.GetValue<void*>(interact, "transform");
                if (!entry.transform || !Phasmo_Safe(entry.transform, 0x20))
                    entry.transform = GetComponentTransform(interact);
                if (!entry.transform)
                    continue;

                entry.renderer = GetRendererFromComponent(interact);
                entry.pos = Phasmo_GetPosition(entry.transform);
                if (entry.pos.x == 0.0f && entry.pos.y == 0.0f && entry.pos.z == 0.0f)
                    continue;

                entry.displayName = GetObjectEspName(interact, entry.transform, entry.renderer);
                if (entry.displayName.empty())
                    entry.displayName = entry.isDoor ? "Door" : (entry.isItem ? (entry.isProp ? "Item Prop" : "Item") : (entry.isProp ? "Prop" : "Interact"));
                entry.loweredName = ToLowerCopy(entry.displayName);
                entry.viewId = GetPhotonViewId(entry.photonView);
                s_cachedObjectEspEntries.push_back(entry);
            }

            EnsureObjectEspFilterFile();
            if (Cheat::VIS_ObjectESPUseJsonFilter || s_objectEspJsonFilters.empty() || nowMs >= s_objectEspJsonNextReloadMs) {
                ReloadObjectEspJsonFilter();
                s_objectEspJsonNextReloadMs = nowMs + 3000;
            }
            if (Cheat::VIS_ObjectESPExportJsonNow) {
                ExportObjectEspIdsJson(s_cachedObjectEspEntries);
                Cheat::VIS_ObjectESPExportJsonNow = false;
            }
            s_objectEspUpdateCursor = 0;
            s_objectEspVisibleRefreshMs = 0;
            s_cachedObjectEspNextRefreshMs = nowMs + kObjectScanRefreshMs;
        }

        s_espDebugObjectsFound = static_cast<int>(s_cachedObjectEspEntries.size());
        if (s_cachedObjectEspEntries.empty())
            return;

        Phasmo::Player* localPlayer = FindLocalPlayer();
        Phasmo::Vec3 localPos{};
        const bool haveLocal = TryGetPlayerPosition(localPlayer, localPos);
        const float maxDistanceSq = Cheat::VIS_ObjectESPMaxDistance * Cheat::VIS_ObjectESPMaxDistance;
        const std::string exactFilter = ToLowerCopy(Cheat::VIS_ObjectESPExactName);
        if (!s_cachedObjectEspEntries.empty()) {
            constexpr size_t kObjectUpdatesPerFrame = 10;
            const size_t updateCount = std::min(kObjectUpdatesPerFrame, s_cachedObjectEspEntries.size());
            for (size_t i = 0; i < updateCount; ++i) {
                CachedObjectEspEntry& entry = s_cachedObjectEspEntries[s_objectEspUpdateCursor];
                if (entry.transform && Phasmo_Safe(entry.transform, 0x20)) {
                    entry.pos = Phasmo_GetPosition(entry.transform);
                    entry.lastPosRefreshMs = nowMs;
                }
                s_objectEspUpdateCursor = (s_objectEspUpdateCursor + 1) % s_cachedObjectEspEntries.size();
            }
        }

        if (nowMs >= s_objectEspVisibleRefreshMs) {
            s_visibleObjectEspEntries.clear();
            s_visibleObjectEspEntries.reserve(std::min<size_t>(s_cachedObjectEspEntries.size(), 64));

            for (const CachedObjectEspEntry& entry : s_cachedObjectEspEntries) {
                if (!entry.transform || !Phasmo_Safe(entry.transform, 0x20))
                    continue;
                if (entry.pos.x == 0.0f && entry.pos.y == 0.0f && entry.pos.z == 0.0f)
                    continue;
                if (Cheat::VIS_ObjectESPOnlyUsable && !entry.isUsable)
                    continue;
                if (entry.isDoor && !Cheat::VIS_ObjectESPShowDoors)
                    continue;
                if (entry.isItem && !Cheat::VIS_ObjectESPShowItems)
                    continue;
                if (!entry.isDoor && !entry.isItem && entry.isProp && !Cheat::VIS_ObjectESPShowProps)
                    continue;
                if (haveLocal && Vec3DistanceSq(localPos, entry.pos) > maxDistanceSq)
                    continue;
                if (Cheat::VIS_ObjectESPUseNameFilter && !exactFilter.empty() && entry.loweredName.find(exactFilter) == std::string::npos)
                    continue;
                if (Cheat::VIS_ObjectESPUseJsonFilter && !s_objectEspJsonFilters.empty()) {
                    bool matched = false;
                    for (const std::string& token : s_objectEspJsonFilters) {
                        if (!token.empty() && entry.loweredName.find(token) != std::string::npos) {
                            matched = true;
                            break;
                        }
                    }
                    if (!matched)
                        continue;
                }
                s_visibleObjectEspEntries.push_back(entry);
            }

            if (haveLocal && s_visibleObjectEspEntries.size() > 1) {
                const size_t keepCount = std::min<size_t>(s_visibleObjectEspEntries.size(), static_cast<size_t>(std::max(1, Cheat::VIS_ObjectESPMaxDrawCount) * 3));
                std::partial_sort(
                    s_visibleObjectEspEntries.begin(),
                    s_visibleObjectEspEntries.begin() + keepCount,
                    s_visibleObjectEspEntries.end(),
                    [&localPos](const CachedObjectEspEntry& a, const CachedObjectEspEntry& b) {
                        return Vec3DistanceSq(localPos, a.pos) < Vec3DistanceSq(localPos, b.pos);
                    });
                s_visibleObjectEspEntries.resize(keepCount);
            }

            s_objectEspVisibleRefreshMs = nowMs + 120;
        }

        if (s_visibleObjectEspEntries.empty())
            return;

        if (haveLocal) {
            std::sort(s_visibleObjectEspEntries.begin(), s_visibleObjectEspEntries.end(),
                [&localPos](const CachedObjectEspEntry& a, const CachedObjectEspEntry& b) {
                    return Vec3DistanceSq(localPos, a.pos) < Vec3DistanceSq(localPos, b.pos);
                });
        }

        const int drawLimit = std::max(1, Cheat::VIS_ObjectESPMaxDrawCount);
        const int perNameLimit = std::max(1, Cheat::VIS_ObjectESPMaxPerName);
        int drawn = 0;
        std::unordered_map<std::string, int> drawnPerName;
        for (const CachedObjectEspEntry& entry : s_visibleObjectEspEntries) {
            const float distance = haveLocal ? Vec3Distance(localPos, entry.pos) : 0.0f;
            int& nameCount = drawnPerName[entry.loweredName];
            if (nameCount >= perNameLimit)
                continue;

            char footer[96];
            if (haveLocal)
                snprintf(footer, sizeof(footer), "ID %d | %.0fm%s", entry.viewId, distance, entry.isUsable ? " | usable" : "");
            else
                snprintf(footer, sizeof(footer), "ID %d%s", entry.viewId, entry.isUsable ? " | usable" : "");

            Phasmo::Vec3 headWorld = entry.pos;
            headWorld.y += entry.isDoor ? 1.10f : (entry.isItem ? 0.42f : 0.55f);
            Phasmo::Vec3 feetWorld = entry.pos;
            feetWorld.y -= entry.isDoor ? 0.02f : 0.08f;
            ImVec2 headScreen{};
            ImVec2 feetScreen{};
            if (WorldToScreen(headWorld, headScreen) && WorldToScreen(feetWorld, feetScreen))
                DrawPlayerBox(dl, headScreen, feetScreen, entry.displayName.c_str(), footer, objectColor);
            else
                DrawFallbackEntityBox(dl, entry.pos, distance, entry.displayName.c_str(), footer, objectColor);

            ++nameCount;
            if (++drawn >= drawLimit)
                break;
        }

        s_espDebugObjectsDrawn = drawn;
    }

    // ══════════════════════════════════════════════════════
    //  FUSE BOX ESP
    // ══════════════════════════════════════════════════════
    static void DrawFuseBoxESP(ImDrawList* dl)
    {
        if (!VIS_FuseBoxESP) return;
        const ImU32 fuseColor = GetEspColor(Cheat::VIS_ESPColorFuseBox);

        __try {
            Phasmo::FuseBox* fb = Phasmo_GetFuseBox();
            if (!fb) return;

            Phasmo::Player* localPlayer = FindLocalPlayer();
            Phasmo::Vec3 localPos{};
            const bool haveLocal = TryGetPlayerPosition(localPlayer, localPos);
            Phasmo::Vec3 fusePos{};
            void* transform = GetComponentTransform(fb);
            const bool haveFusePos = (transform != nullptr);
            if (haveFusePos)
                fusePos = Phasmo_GetPosition(transform);

            const float distance = (haveLocal && haveFusePos) ? Vec3Distance(localPos, fusePos) : -1.0f;
            const char* label = fb->isOn ? "Fuse Box [ON]" : "Fuse Box [OFF]";
            char footer[96];
            if (distance >= 0.0f)
                snprintf(footer, sizeof(footer), "Power %s | %.0fm", fb->isOn ? "On" : "Off", distance);
            else
                snprintf(footer, sizeof(footer), "Power %s", fb->isOn ? "On" : "Off");

            if (haveFusePos) {
                void* renderer = GetRendererFromComponent(fb);
                if (renderer && DrawRendererBox(dl, renderer, label, footer, fuseColor))
                    return;

                Phasmo::Vec3 headWorld = fusePos;
                headWorld.y += 0.72f;
                Phasmo::Vec3 feetWorld = fusePos;
                feetWorld.y -= 0.06f;
                ImVec2 headScreen{};
                ImVec2 feetScreen{};
                if (WorldToScreen(headWorld, headScreen) && WorldToScreen(feetWorld, feetScreen))
                    DrawPlayerBox(dl, headScreen, feetScreen, label, footer, fuseColor);
                else
                    DrawFallbackEntityBox(dl, fusePos, distance, label, footer, fuseColor);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  EMF ESP
    // ══════════════════════════════════════════════════════
    static void DrawEMFESP(ImDrawList* dl)
    {
        if (!VIS_EMFESP) return;
        const ImU32 emfColor = GetEspColor(Cheat::VIS_ESPColorEMF);

        __try {
            // EMF data is typically accessed through EvidenceController
            Phasmo::EvidenceController* ec = Phasmo_GetEvidenceController();
            if (!ec) return;

            // Display EMF status
            ImGuiIO& io = ImGui::GetIO();
            ImVec2 pos = { io.DisplaySize.x - 200.0f, 90.0f };

            DrawTextShadow(dl, pos, emfColor, "EMF: Monitoring...");
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    // ══════════════════════════════════════════════════════
    //  EVIDENCE ESP
    // ══════════════════════════════════════════════════════
    static void DrawEvidenceESP(ImDrawList* dl)
    {
        if (!VIS_EvidenceESP) return;

        Phasmo::EvidenceController* ec = Phasmo_GetEvidenceController();
        if (!ec) return;

        // Access evidence list
        void* evidenceList = ec->evidenceList;
        if (!Phasmo_Safe(evidenceList, 0x20)) return;

        Il2CppListPlayer* list = reinterpret_cast<Il2CppListPlayer*>(evidenceList);
        if (!Phasmo_Safe(list, sizeof(Il2CppListPlayer))) return;

        const int evidenceCount = list->_size;

        std::string evidenceNames;
        std::vector<int32_t> seenTypes;
        if (list->_items && evidenceCount > 0 && evidenceCount < 128) {
            auto** entries = reinterpret_cast<Phasmo::Evidence**>(
                reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);
            for (int i = 0; i < evidenceCount; ++i) {
                Phasmo::Evidence* ev = entries[i];
                if (!ev || !Phasmo_Safe(ev, sizeof(Phasmo::Evidence)))
                    continue;

                void* mv = ev->mediaValues;
                if (!mv || !Phasmo_Safe(mv, Phasmo::MV_EVIDENCE_TYPE + sizeof(int32_t)))
                    continue;

                int32_t type = *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(mv) + Phasmo::MV_EVIDENCE_TYPE);
                if (std::find(seenTypes.begin(), seenTypes.end(), type) != seenTypes.end())
                    continue;

                const char* name = EvidenceTypeName(type);
                char fallback[32]{};
                if (!name || !*name) {
                    snprintf(fallback, sizeof(fallback), "Type %d", type);
                    name = fallback;
                }

                if (!evidenceNames.empty())
                    evidenceNames += ", ";
                evidenceNames += name;
                seenTypes.push_back(type);
            }
        }

        // Draw evidence indicator
        ImGuiIO& io = ImGui::GetIO();
        ImVec2 pos = { io.DisplaySize.x - 200.0f, 120.0f };

        char buf[64];
        snprintf(buf, sizeof(buf), "Evidence tracked: %d", evidenceCount);
        DrawTextShadow(dl, pos, GetEspColor(Cheat::VIS_ESPColorEvidence), buf);

        if (!evidenceNames.empty()) {
            pos.y += 20.0f;
            DrawTextShadow(dl, pos, GetEspColor(Cheat::VIS_ESPColorEvidence), evidenceNames.c_str());
        }

        // Ghost orb position
        Phasmo::Transform* orbTf = ec->ghostOrbTransform;
        if (Phasmo_Safe(orbTf, 0xC0)) {
            Phasmo::Vec3 orbPos = Phasmo_GetPosition(orbTf);
            pos.y += 20.0f;
            snprintf(buf, sizeof(buf), "Orb: %.1f, %.1f, %.1f", orbPos.x, orbPos.y, orbPos.z);
            DrawTextShadow(dl, pos, GetEspColor(Cheat::VIS_ESPColorEvidence), buf);
        }
    }

    // ══════════════════════════════════════════════════════
    //  PLAYER ESP
    // ══════════════════════════════════════════════════════
    static void DrawPlayerBox(ImDrawList* dl, const ImVec2& head, const ImVec2& feet, const char* label, const char* footer, ImU32 color)
    {
        const float height = fabsf(feet.y - head.y);
        if (height < 18.0f)
            return;

        const float width = height * 0.42f;
        const float left = head.x - width * 0.5f;
        const float right = head.x + width * 0.5f;
        const float top = head.y;
        const float bottom = feet.y;
        DrawEspBounds(dl, ImVec2(left, top), ImVec2(right, bottom), label, footer, color);
    }

    static bool DrawWireBox3D(ImDrawList* dl, const Phasmo::Vec3& origin, float width, float height, float depth, ImU32 color)
    {
        Phasmo::Vec3 corners[8] = {
            { origin.x - width, origin.y,          origin.z - depth },
            { origin.x + width, origin.y,          origin.z - depth },
            { origin.x + width, origin.y,          origin.z + depth },
            { origin.x - width, origin.y,          origin.z + depth },
            { origin.x - width, origin.y + height, origin.z - depth },
            { origin.x + width, origin.y + height, origin.z - depth },
            { origin.x + width, origin.y + height, origin.z + depth },
            { origin.x - width, origin.y + height, origin.z + depth }
        };

        ImVec2 projected[8]{};
        for (int i = 0; i < 8; ++i) {
            if (!WorldToScreen(corners[i], projected[i]))
                return false;
        }

        const int edges[][2] = {
            { 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
            { 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
            { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
        };

        for (const auto& edge : edges)
            DrawOutlinedLine(dl, projected[edge[0]], projected[edge[1]], color, 1.2f);

        return true;
    }

    static bool TryBuildRendererBounds(void* renderer, ImVec2& outMin, ImVec2& outMax)
    {
        Phasmo::Bounds bounds{};
        if (!PhasmoResolve::GetRendererBounds(renderer, bounds))
            return false;

        Phasmo::Vec3 ext = bounds.extents;
        if (ext.x < 0.08f && ext.y < 0.08f && ext.z < 0.08f)
            ext = { 0.25f, 0.25f, 0.25f };

        const Phasmo::Vec3 c = bounds.center;
        Phasmo::Vec3 corners[8] = {
            { c.x - ext.x, c.y - ext.y, c.z - ext.z },
            { c.x + ext.x, c.y - ext.y, c.z - ext.z },
            { c.x + ext.x, c.y - ext.y, c.z + ext.z },
            { c.x - ext.x, c.y - ext.y, c.z + ext.z },
            { c.x - ext.x, c.y + ext.y, c.z - ext.z },
            { c.x + ext.x, c.y + ext.y, c.z - ext.z },
            { c.x + ext.x, c.y + ext.y, c.z + ext.z },
            { c.x - ext.x, c.y + ext.y, c.z + ext.z }
        };

        ImVec2 projected[8]{};
        for (int i = 0; i < 8; ++i) {
            if (!WorldToScreen(corners[i], projected[i]))
                return false;
        }

        ImVec2 min(FLT_MAX, FLT_MAX);
        ImVec2 max(-FLT_MAX, -FLT_MAX);
        for (const ImVec2& point : projected)
            ExpandScreenBounds(point, min, max);

        outMin = min;
        outMax = max;
        return true;
    }

    static bool DrawRendererBox(ImDrawList* dl, void* renderer, const char* label, const char* footer, ImU32 color)
    {
        ImVec2 min{};
        ImVec2 max{};
        if (!TryBuildRendererBounds(renderer, min, max))
            return false;

        DrawEspBounds(dl, min, max, label, footer, color);
        return true;
    }

    static void DrawFallbackEntityBox(ImDrawList* dl, const Phasmo::Vec3& origin, float distance, const char* label, const char* footer, ImU32 color)
    {
        Phasmo::Vec3 headWorld = origin;
        headWorld.y += 1.25f;
        Phasmo::Vec3 feetWorld = origin;
        feetWorld.y -= 0.15f;

        ImVec2 headScreen{};
        ImVec2 feetScreen{};
        if (WorldToScreen(headWorld, headScreen) && WorldToScreen(feetWorld, feetScreen)) {
            DrawPlayerBox(dl, headScreen, feetScreen, label, footer, color);
            return;
        }

        ImVec2 center{};
        if (!WorldToScreen(origin, center))
            return;

        const float dist = (distance > 0.0f) ? distance : 8.0f;
        const float boxH = std::clamp(2000.0f / (dist + 1.0f), 78.0f, 220.0f);
        const float boxW = boxH * 0.42f;
        DrawEspBounds(
            dl,
            ImVec2(center.x - boxW * 0.5f, center.y - boxH * 0.85f),
            ImVec2(center.x + boxW * 0.5f, center.y + boxH * 0.15f),
            label,
            footer,
            color);
    }

    static void DrawPlayerESP(ImDrawList* dl)
    {
        if (!VIS_PlayerESP) return;
        const ImU32 playerColor = GetEspColor(Cheat::VIS_ESPColorPlayer);

        Il2CppListPlayer* list = GetMapPlayerList();
        s_espDebugPlayersFound = list ? list->_size : 0;
        if (!list)
            return;

        Phasmo::Player* localPlayer = FindLocalPlayer();
        Phasmo::Vec3 localPos{};
        const bool haveLocal = TryGetPlayerPosition(localPlayer, localPos);

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::Player* player = GetMapPlayerAt(list, i);
            if (!player)
                continue;

            if (IsLocalMapPlayer(player))
                continue;

            Phasmo::Vec3 playerPos{};
            if (!TryGetPlayerPosition(player, playerPos))
                continue;

            Phasmo::Vec3 headWorld = playerPos;
            if (void* headTransform = GetPlayerHeadTransform(player))
                headWorld = Phasmo_GetPosition(headTransform);
            else
                headWorld.y += 1.72f;
            Phasmo::Vec3 feetWorld = playerPos;
            feetWorld.y -= 0.10f;

            ImVec2 headScreen{};
            ImVec2 feetScreen{};
            const bool hasProjectedBody = WorldToScreen(headWorld, headScreen) && WorldToScreen(feetWorld, feetScreen);

            Phasmo::NetworkPlayerSpot* spot = GetPlayerSpot(player);
            const std::string name = GetPlayerDisplayName(player, i);
            const char* roleName = spot ? RoleName(spot->role) : nullptr;
            const float distance = haveLocal ? Vec3Distance(localPos, playerPos) : -1.0f;

            char label[192];
            if (roleName && roleName[0] && strcmp(roleName, "None") != 0)
                snprintf(label, sizeof(label), "%s [%s]", name.c_str(), roleName);
            else
                snprintf(label, sizeof(label), "%s", name.c_str());

            char footer[96];
            if (distance >= 0.0f)
                snprintf(footer, sizeof(footer), "%.0fm", distance);
            else
                footer[0] = '\0';

            if (Cheat::VIS_PlayerSkeletonESP) {
                void* animator = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(player) + PLAYER_ANIMATOR_OFFSET);
                if (animator && DrawHumanoidSkeleton(dl, animator, label, footer, playerColor))
                    continue;
            }

            if (hasProjectedBody)
                DrawPlayerBox(dl, headScreen, feetScreen, label, footer, playerColor);
            else
                DrawFallbackEntityBox(dl, playerPos, distance, label, footer, playerColor);
        }
    }

    static void DrawPlayersWindowUnsafe()
    {
        const bool inGame = IsInGame();

        ImGui::SetNextItemWidth(-1.0f);
        s_playersWindowFilter.Draw("Search players##window", 0.0f);
        ImGui::Separator();

        Il2CppListPlayer* list = inGame ? GetMapPlayerList() : nullptr;
        Il2CppListPlayer* spotList = GetNetworkPlayerSpotList();

        if (!list && !spotList) {
            ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.45f, 1.0f), "Player data unavailable");
            ImGui::TextWrapped("Ni MapController ni Network.Instance n'ont encore expose la liste des joueurs.");
            return;
        }

        const int32_t count = list ? list->_size : spotList->_size;
        ImGui::Text("Connected Players: %d", count);
        if (!inGame)
            ImGui::TextDisabled("Lobby/menu safe mode: infos reseau seulement.");
        else if (!list)
            ImGui::TextDisabled("Lobby fallback: infos reseau seulement, actions gameplay desactivees.");
        ImGui::Separator();

        for (int32_t i = 0; i < count; ++i) {
            Phasmo::Player* player = list ? GetMapPlayerAt(list, i) : nullptr;
            Phasmo::NetworkPlayerSpot* spot = player ? GetPlayerSpot(player) : GetNetworkSpotAt(i);
            const bool isLocal = (inGame && player) ? IsLocalMapPlayer(player) : false;
            int actor = -1;
            if (player) {
                actor = GetMapPlayerActorNumber(player);
            } else if (spot && spot->photonPlayer && Phasmo_Safe(spot->photonPlayer, Phasmo::PP_ACTOR_NUMBER + sizeof(int32_t))) {
                actor = *reinterpret_cast<int32_t*>(
                    reinterpret_cast<uint8_t*>(spot->photonPlayer) + Phasmo::PP_ACTOR_NUMBER);
            }
            Phasmo::PlayerSanity* sanity = player ? Phasmo_GetPlayerSanity(player) : nullptr;
            const std::string name = (inGame && player) ? GetPlayerDisplayName(player, i) : GetSpotName(spot);
            const std::string room = (inGame && player) ? GetRoomName(player->levelRoom) : std::string{};
            const char* role = spot ? RoleName(spot->role) : "None";
            const bool ready = spot ? spot->playerReady : false;
            const std::string actorLabel = actor >= 0 ? ("Actor " + std::to_string(actor)) : "Actor ?";

            char label[256];
            snprintf(label, sizeof(label), "%s | Slot %d | %s | %s%s%s%s%s",
                name.empty() ? "Unknown" : name.c_str(),
                i,
                actorLabel.c_str(),
                role,
                ready ? " | READY" : " | NOT READY",
                isLocal ? " | YOU" : "",
                room.empty() ? "" : " | ",
                room.empty() ? "" : room.c_str());

            if (s_playersWindowFilter.IsActive() && !s_playersWindowFilter.PassFilter(label))
                continue;

            ImGui::TextUnformatted(name.empty() ? "Unknown" : name.c_str());
            ImGui::TextDisabled("Slot %d | %s | %s%s",
                i,
                actorLabel.c_str(),
                role,
                ready ? " | READY" : " | NOT READY");

            if (sanity)
                ImGui::Text("Sanity: %.0f%%", sanity->sanity);
            if (!room.empty())
                ImGui::TextDisabled("Room: %s", room.c_str());
            if (inGame && player) {
                Phasmo::Vec3 pos{};
                if (TryGetPlayerPosition(player, pos))
                    ImGui::TextDisabled("Pos: %.1f %.1f %.1f", pos.x, pos.y, pos.z);
            }

            ImGui::PushID(i);
            const bool canAct = (inGame && player != nullptr);
            if (!canAct) ImGui::BeginDisabled(true);
            if (ImGui::SmallButton("Teleport")) {
                Cheat::PLAYER_TeleportTarget = i;
                Cheat::PLAYER_RequestTeleport = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Kill")) {
                Cheat::PLAYER_KillTarget = i;
                Cheat::PLAYER_RequestKill = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Freeze")) {
                Cheat::PLAYER_FreezeTarget = i;
                Cheat::PLAYER_RequestFreeze = true;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Unfreeze")) {
                Cheat::PLAYER_FreezeTarget = i;
                Cheat::PLAYER_RequestUnfreeze = true;
            }
            if (!canAct) ImGui::EndDisabled();
            ImGui::PopID();

            if (i + 1 < count)
                ImGui::Separator();
        }
    }

    static void DrawPlayersWindow()
    {
        if (!VIS_PlayersWindow) return;

        ImGui::SetNextWindowSize(ImVec2(420, 320), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Players", &VIS_PlayersWindow)) {
            __try {
                DrawPlayersWindowUnsafe();
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                Logger::WriteLine("[ESP] Players window crashed while rendering");
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "Players window rendering failed");
                ImGui::TextWrapped("Une lecture ESP a plante dans cette frame, mais la fenetre a ete gardee ouverte proprement.");
            }
        }
        ImGui::End();
    }

    static void DrawEspDebugOverlay(ImDrawList* dl)
    {
        ImGuiIO& io = ImGui::GetIO();
        const ImU32 accent = GetEspColor(Cheat::VIS_ESPColorObject);
        const ImVec2 pos(16.0f, io.DisplaySize.y - 116.0f);
        const ImVec2 size(244.0f, 92.0f);

        dl->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(18, 14, 24, 228), 12.0f);
        dl->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(122, 72, 102, 210), 12.0f, 0, 1.0f);
        dl->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 4.0f, pos.y + size.y), accent, 12.0f, ImDrawFlags_RoundCornersLeft);

        char line1[96];
        char line2[96];
        char line3[96];
        snprintf(line1, sizeof(line1), "camera %s | w2s %s", s_espDebugCameraOk ? "ok" : "off", s_espDebugW2sOk ? "ok" : "off");
        snprintf(line2, sizeof(line2), "ghost %d | players %d", s_espDebugGhostFound, s_espDebugPlayersFound);
        snprintf(line3, sizeof(line3), "objects %d | draw %d", s_espDebugObjectsFound, s_espDebugObjectsDrawn);

        DrawTextShadow(dl, ImVec2(pos.x + 12.0f, pos.y + 8.0f), IM_COL32(250, 236, 244, 255), "ESP Debug");
        DrawTextShadow(dl, ImVec2(pos.x + 12.0f, pos.y + 30.0f), IM_COL32(210, 194, 206, 255), line1);
        DrawTextShadow(dl, ImVec2(pos.x + 12.0f, pos.y + 48.0f), IM_COL32(210, 194, 206, 255), line2);
        DrawTextShadow(dl, ImVec2(pos.x + 12.0f, pos.y + 66.0f), IM_COL32(210, 194, 206, 255), line3);
    }

    // ══════════════════════════════════════════════════════
    //  ESP MAIN
    // ══════════════════════════════════════════════════════
    void ESP_OnFrame()
    {
        if (!ImGui::GetCurrentContext())
            return;

        const bool inGame = IsInGame();
        if (inGame && !s_espLastInGameFrame)
            s_espInGameEnteredAtMs = GetTickCount64();
        else if (!inGame) {
            s_espInGameEnteredAtMs = 0;
            s_cachedObjectEspEntries.clear();
            s_cachedObjectEspNextRefreshMs = 0;
        }
        s_espLastInGameFrame = inGame;
        s_espDebugCameraOk = false;
        s_espDebugW2sOk = false;
        s_espDebugGhostFound = 0;
        s_espDebugPlayersFound = 0;
        s_espDebugObjectsFound = 0;
        s_espDebugObjectsDrawn = 0;

        // Check if any ESP feature is enabled
        bool anyESP = VIS_GhostESP || VIS_PlayerESP || VIS_EvidenceESP
                   || VIS_FuseBoxESP || VIS_ObjectESP || VIS_EMFESP || VIS_ESPDebugOverlay;

        if (!anyESP && !VIS_PlayersWindow) return;

        if (!hIl2Cpp) return;

        ImDrawList* dl = nullptr;
        __try {
            dl = ImGui::GetForegroundDrawList();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return;
        }
        if (!dl) return;

        if (inGame) {
            // Draw live overlays only during an active investigation.
            __try { DrawGhostESP(dl); }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                s_ghostEspCrashCount += 1;
                s_ghostEspSuspendUntilMs = GetTickCount64() + 2000;
                Logger::WriteLine("[ESP] Ghost ESP crashed (cooldown 2s, retrying)");
            }

            __try { DrawPlayerESP(dl); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}

            __try { DrawEvidenceESP(dl); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}

            __try { DrawFuseBoxESP(dl); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}

            __try { DrawObjectESP(dl); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}

            __try { DrawEMFESP(dl); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        if (anyESP && Cheat::VIS_ESPDebugOverlay) {
            __try { DrawEspDebugOverlay(dl); }
            __except (EXCEPTION_EXECUTE_HANDLER) {}
        }

        // ImGui windows
        __try { DrawPlayersWindow(); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

} // namespace Cheat

#include "pch.h"
#include "cheat.h"
#include "phasmo_helpers.h"
#include "global.h"
#include "logger.h"
#include "imgui.h"
#include "config.h"
#include "fov_changer.h"
#include "fps_indicator.h"
#include "fps_unlocker.h"
#include "global_speed_changer.h"

// ══════════════════════════════════════════════════════════
//  CHEAT.CPP — Phasmophobia
//  Implémentation principale du cheat
// ══════════════════════════════════════════════════════════

namespace Cheat
{
    // ═══════════════════════════════════════════════════
    //  VISUAL
    // ═══════════════════════════════════════════════════
    bool VIS_Watermark = true;
    bool VIS_StatsWindow = false;
    bool VIS_GhostProfile = false;
    bool VIS_EvidenceESP = false;
    bool VIS_PlayerESP = false;
    bool VIS_Fullbright = false;
    int VIS_FullbrightMode = 0;
    float VIS_FullbrightIntensity = 1.0f;
    bool VIS_GhostESP = false;
    bool VIS_PlayersWindow = false;
    bool VIS_FuseBoxESP = false;
    bool VIS_ObjectESP = false;
    bool VIS_ESPDebugOverlay = false;
    bool VIS_GhostSkeletonESP = false;
    bool VIS_PlayerSkeletonESP = false;
    bool VIS_ObjectESPShowDoors = false;
    bool VIS_ObjectESPShowItems = true;
    bool VIS_ObjectESPShowProps = true;
    bool VIS_ObjectESPOnlyUsable = false;
    bool VIS_ObjectESPUseNameFilter = false;
    bool VIS_ObjectESPUseJsonFilter = false;
    bool VIS_ObjectESPExportJsonNow = false;
    float VIS_ObjectESPMaxDistance = 28.0f;
    int VIS_ObjectESPMaxDrawCount = 8;
    int VIS_ObjectESPMaxPerName = 2;
    float VIS_ESPLineThickness = 1.5f;
    bool VIS_EMFESP = false;
    bool VIS_ActivityMonitor = false;
    char VIS_ObjectESPExactName[128] = "";
    char VIS_ObjectESPJsonPath[MAX_PATH] = "";
    float VIS_ESPColorGhost[4] = { 0.96f, 0.45f, 0.67f, 1.0f };
    float VIS_ESPColorPlayer[4] = { 0.98f, 0.72f, 0.84f, 1.0f };
    float VIS_ESPColorEvidence[4] = { 1.00f, 0.84f, 0.92f, 1.0f };
    float VIS_ESPColorFuseBox[4] = { 0.95f, 0.64f, 0.78f, 1.0f };
    float VIS_ESPColorObject[4] = { 0.90f, 0.36f, 0.56f, 1.0f };
    float VIS_ESPColorEMF[4] = { 1.00f, 0.58f, 0.74f, 1.0f };

    // ═══════════════════════════════════════════════════
    //  PLAYER
    // ═══════════════════════════════════════════════════
    bool PLAYER_GodMode = false;
    bool PLAYER_FovEditor = false;
    float PLAYER_FovValue = 60.0f;
    int PLAYER_TeleportTarget = -1;
    int PLAYER_KillTarget = -1;
    int PLAYER_FreezeTarget = -1;
    float PLAYER_SanityValue = 100.0f;
    bool PLAYER_SetSanity = false;
    bool PLAYER_RequestTeleport = false;
    bool PLAYER_RequestKill = false;
    bool PLAYER_RequestFreeze = false;
    bool PLAYER_RequestUnfreeze = false;
    bool PLAYER_RequestSuicide = false;
    bool PLAYER_RadioSpamNetwork = false;
    int PLAYER_RadioSpamDelayMs = 120;
    bool PLAYER_TruckRadioSpamNetwork = false;
    int PLAYER_TruckRadioSpamDelayMs = 900;
    int PLAYER_TruckRadioCategory = 0;
    bool PLAYER_TruckRadioRandomClip = true;
    int PLAYER_TruckRadioClipIndex = 0;
    bool PLAYER_MicSaturation = false;
    float PLAYER_MicSaturationVolume = 3.5f;
    bool PLAYER_HearDeadPlayers = false;
    bool PLAYER_AiriHeldTint = false;
    bool PLAYER_AiriHandsTint = false;
    float PLAYER_AiriTintColor[4] = { 0.96f, 0.55f, 0.74f, 1.0f };

    // ═══════════════════════════════════════════════════
    //  GHOST
    // ═══════════════════════════════════════════════════
    int GHOST_WeatherIndex = 0;
    bool GHOST_WeatherEnabled = false;
    bool GHOST_CustomSpeed = false;
    float GHOST_SpeedValue = 1.0f;
    int GHOST_ForceStateIndex = 0;
    bool GHOST_ForceStateEnabled = false;
    bool GHOST_ForceAbility = false;
    bool GHOST_ForceInteract = false;
    bool GHOST_ForceInteractDoor = false;
    bool GHOST_ForceInteractProp = false;
    bool GHOST_ForceNormalInteraction = false;
    bool GHOST_ForceTwinInteraction = false;
    bool GHOST_Designer = false;
    int GHOST_AudioIndex = 0;
    bool GHOST_PlayAudio = false;
    bool GHOST_StopAudio = false;
    bool GHOST_AudioBroadcast = true;
    bool GHOST_AutoJournalSolver = false;
    bool GHOST_SolveJournalNow = false;

    // ═══════════════════════════════════════════════════
    //  CURSED ITEMS
    // ═══════════════════════════════════════════════════
    bool CURSED_BreakItems = false;
    bool CURSED_UseItems = false;
    int CURSED_NextTarotCard = 0;
    bool CURSED_InfinityCards = false;
    bool CURSED_TriggerOuijaBoard = false;
    bool CURSED_TriggerMusicBox = false;
    bool CURSED_TriggerTarotCards = false;
    bool CURSED_TriggerSummoningCircle = false;
    bool CURSED_TriggerHauntedMirror = false;
    bool CURSED_TriggerVoodooDoll = false;
    bool CURSED_TriggerMonkeyPaw = false;
    char CURSED_OuijaText[128] = "HELLO";
    bool CURSED_OuijaNetworked = true;
    bool CURSED_OuijaApplyNow = false;

    // ═══════════════════════════════════════════════════
    //  MOVEMENT
    // ═══════════════════════════════════════════════════
    bool MOV_Teleport = false;
    bool MOV_Freecam = false;
    float MOV_FreecamSpeed = 8.0f;
    bool MOV_NoClip = false;
    int MOV_NoClipMode = 0;
    bool MOV_InfinityStamina = false;
    bool MOV_CustomSpeed = false;
    float MOV_SpeedValue = 1.0f;
    bool MOV_FlyingEquipment = false;
    float MOV_FlyingEquipmentRadius = 3.0f;
    float MOV_FlyingEquipmentHeight = 0.5f;
    float MOV_FlyingEquipmentSpeed = 1.0f;
    int MOV_FlyingEquipmentShape = 0;
    bool LOBBY_EntityListEnabled = false;

    // ═══════════════════════════════════════════════════
    //  MORE
    // ═══════════════════════════════════════════════════
    bool MORE_AlwaysPerfectGame = false;
    bool MORE_CustomBonusReward = false;
    float MORE_BonusRewardValue = 1.0f;
    bool MORE_AntiKick = false;
    bool MORE_NoCameraRestrictions = false;
    bool MORE_DisableDoorInteraction = false;
    bool MORE_LockDoors = false;
    bool MORE_UnlockDoors = false;
    bool MORE_CloseDoors = false;
    bool MORE_CloseDoorsLoop = false;
    int MORE_CloseDoorsLoopDelayMs = 350;
    bool MORE_LockUnlockDoorsLoop = false;
    int MORE_LockUnlockDoorsLoopDelayMs = 500;
    bool MORE_OpenCloseDoorsLoop = false;
    int MORE_OpenCloseDoorsLoopDelayMs = 600;
    bool MORE_WalkThroughDoors = false;
    bool MORE_Pickup = false;
    float MORE_ThrowStrength = 1.0f;
    bool MORE_CanPickup = false;
    bool MORE_PocketEverything = false;
    bool MORE_PickupAnyProp = false;
    bool MORE_GrabAllKeys = false;
    bool MORE_LeavePeople = false;
    bool MORE_CompleteMissions = false;
    bool MORE_ButterFingers = false;
    bool MORE_DropHeldLocal = false;
    bool MORE_DropHeldTarget = false;
    bool MORE_DropAllInventoryLocal = false;
    bool MORE_DropAllInventoryTarget = false;
    bool MORE_DropAllInventoryAll = false;
    bool MORE_DropHeldFiltered = false;
    bool MORE_DropAllInventoryFiltered = false;
    int MORE_DropTargetSlot = -1;
    int MORE_DropAllFilterSlotMin = 0;
    int MORE_DropAllFilterSlotMax = 31;
    int MORE_DropAllFilterOwnerMode = 0;
    char MORE_DropAllFilterOwner[64] = "";
    bool MORE_CustomName = false;
    char MORE_NameBuffer[64] = "Phasmo";
    bool MORE_ApplyCustomNameNow = false;
    bool MORE_SetBadge = false;
    int MORE_BadgeIndex = 0;
    bool MORE_SetBadgeMultiplayer = false;
    bool MORE_SetBadgeNetworked = false;
    bool MORE_SetBadgeNetworkedPersistent = false;
    bool MORE_DisconnectPlayer = false;
    int MORE_DisconnectTarget = -1;
    int MORE_DisconnectMode = 0;
    bool MORE_SpamFlashlight = false;
    int MORE_SpamFlashlightMode = 0;
    int MORE_SpamFlashlightDelayMs = 50;
    bool MORE_TargetFlashlightTroll = false;
    bool MORE_TorchUseSpam = false;
    int MORE_TorchUseSpamDelayMs = 150;
    bool MORE_DoorSlamAll = false;
    bool MORE_DoorRattleAll = false;
    bool MORE_DoorLockSoundAll = false;
    bool MORE_DoorUnlockSoundAll = false;
    bool MORE_TargetDoorClose = false;
    bool MORE_TargetDoorLock = false;
    bool MORE_TargetDoorSlam = false;
    bool MORE_TargetDoorRattle = false;
    bool MORE_TargetDoorLockSound = false;
    bool MORE_TargetDoorUnlockSound = false;
    float MORE_TargetDoorRadius = 4.5f;

    bool NETWORK_QuickRejoin = false;
    bool NETWORK_ConnectUsingSettings = false;
    bool NETWORK_Reconnect = false;
    bool NETWORK_JoinRandomRoom = false;
    bool NETWORK_JoinRandomRoomFiltered = false;
    bool NETWORK_JoinRandomOrCreateRoomFiltered = false;
    bool NETWORK_JoinRoomByName = false;
    bool NETWORK_JoinOrCreateRoom = false;
    bool NETWORK_CreateRoomByName = false;
    bool NETWORK_LeaveRoom = false;
    bool NETWORK_LeaveLobby = false;
    bool NETWORK_DisconnectSelf = false;
    bool NETWORK_RoomSniper = false;
    bool NETWORK_StealHost = false;
    bool NETWORK_AutoFarm = false;
    bool NETWORK_AutoFarmAutoRerun = true;
    bool NETWORK_AutoFarmUseMinPlayers = false;
    bool NETWORK_AutoFarmRefreshContracts = false;
    int NETWORK_CreateRoomMaxPlayers = 4;
    int NETWORK_FilterExpectedMaxPlayers = 0;
    int NETWORK_FilterMatchmakingMode = 0;
    int NETWORK_FilterLobbyType = 0;
    int NETWORK_RoomSniperDelayMs = 2000;
    int NETWORK_AutoFarmStartDelaySec = 4;
    int NETWORK_AutoFarmEndDelaySec = 4;
    int NETWORK_AutoFarmMaxRoundSec = 600;
    int NETWORK_AutoFarmLobbyTimeoutSec = 60;
    int NETWORK_AutoFarmMinPlayers = 1;
    int NETWORK_AutoFarmContractMode = 1;
    char NETWORK_RoomNameBuffer[64] = "";
    char NETWORK_LobbyNameBuffer[64] = "";
    char NETWORK_SqlFilterBuffer[128] = "";
    bool NETWORK_WebhookEnabled = false;
    bool NETWORK_WebhookLogSession = true;
    bool NETWORK_WebhookLogRooms = true;
    bool NETWORK_WebhookLogContracts = true;
    bool NETWORK_WebhookLogDeaths = true;
    bool NETWORK_WebhookLogCommands = true;
    char NETWORK_WebhookUrl[512] = "";

    // ═══════════════════════════════════════════════════
    //  CONFIG
    // ═══════════════════════════════════════════════════
    bool CFG_DifficultyChanger = false;
    int CFG_DifficultyIndex = 0;

    // ── Profile edits ──
    int PROFILE_CashDelta = 0;
    int PROFILE_CashTarget = 0;
    int PROFILE_XPDelta = 0;
    bool PROFILE_AddCash = false;
    bool PROFILE_AddXP = false;
    bool PROFILE_SetCash = false;
    int PROFILE_CashMode = 0;
    bool PROFILE_AutoEndGameBonus = false;
    bool PROFILE_ApplyEndGameBonusNow = false;
    int PROFILE_EndGameCashBonus = 0;
    int PROFILE_EndGameXPBonus = 0;
    int PROFILE_ItemCount = 0;
    bool PROFILE_SetItems = false;
    bool PROFILE_SetLevel = false;
    int PROFILE_LevelValue = 1;
    bool PROFILE_SetPrestige = false;
    int PROFILE_PrestigeValue = 0;
    bool PROFILE_CashOverride = false;
    int PROFILE_CashOverrideValue = 5000000;
    bool PROFILE_AutoSaveAfterEdit = true;
    bool PROFILE_BackupSaveNow = false;
    bool PROFILE_ReloadSaveNow = false;

    // ═══════════════════════════════════════════════════
    //  SETUP
    // ═══════════════════════════════════════════════════
    bool Setup()
    {
        Logger::WriteLine("[Cheat] Setup start (Phasmophobia)");
        Config::Setup();
        FovChanger::Setup();
        FpsIndicator::Setup();
        FpsUnlocker::Setup();
        GlobalSpeedChanger::Setup();
        Webhook_Setup();
        Logger::WriteLine("[Cheat] Setup OK");
        return true;
    }

    // ═══════════════════════════════════════════════════
    //  SHUTDOWN
    // ═══════════════════════════════════════════════════
    void Shutdown()
    {
        Webhook_Shutdown();
        Logger::WriteLine("[Cheat] Shutdown");
    }

    // ═══════════════════════════════════════════════════
    //  BEFORE FRAME
    // ═══════════════════════════════════════════════════
    void BeforeFrame()
    {
        Config::BeforeFrame();
        FovChanger::BeforeFrame();
        FpsIndicator::BeforeFrame();
        FpsUnlocker::BeforeFrame();
        GlobalSpeedChanger::BeforeFrame();
    }

    // ═══════════════════════════════════════════════════
    //  ON FRAME — Boucle principale
    // ═══════════════════════════════════════════════════
    void OnFrame()
    {
        uintptr_t base = Phasmo_Base();
        if (!base) return;

        __try {
            Config::OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in Config::OnFrame");
        }

        __try {
            FovChanger::OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in FovChanger::OnFrame");
        }

        __try {
            FpsIndicator::OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in FpsIndicator::OnFrame");
        }

        __try {
            FpsUnlocker::OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in FpsUnlocker::OnFrame");
        }

        __try {
            GlobalSpeedChanger::OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in GlobalSpeedChanger::OnFrame");
        }

        __try {
            PhasmoFeatures_OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in PhasmoFeatures_OnFrame");
        }

        __try {
            ESP_OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in ESP_OnFrame");
        }

        __try {
            Visuals_OnFrame();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in Visuals_OnFrame");
        }

        __try {
            Webhook_TrackState();
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            Logger::WriteLine("[Cheat] Exception in Webhook_TrackState");
        }
    }

    // ═══════════════════════════════════════════════════
    //  APPLY THEME
    // ═══════════════════════════════════════════════════
    void ApplyThemeFromOptions()
    {
        auto clamp01 = [](float value) -> float
        {
            if (value < 0.0f) return 0.0f;
            if (value > 1.0f) return 1.0f;
            return value;
        };

        ImGuiStyle& style = ImGui::GetStyle();
        const float scale = style.FontScaleDpi > 0.0f ? style.FontScaleDpi : 1.0f;

        ImGuiStyle defaultStyle;
        if (scale != 1.0f)
            defaultStyle.ScaleAllSizes(scale);
        defaultStyle.FontScaleDpi = scale;
        style = defaultStyle;

        ImVec4* colors = style.Colors;
        float accentR = Options.AccentR;
        float accentG = Options.AccentG;
        float accentB = Options.AccentB;
        float bgR = Options.BgR;
        float bgG = Options.BgG;
        float bgB = Options.BgB;
        float sidebarR = Options.SidebarR;
        float sidebarG = Options.SidebarG;
        float sidebarB = Options.SidebarB;
        float textR = Options.TextR;
        float textG = Options.TextG;
        float textB = Options.TextB;

        switch (Options.ThemePreset)
        {
        case 0: // Airi
            accentR = 0.95f; accentG = 0.69f; accentB = 0.81f;
            bgR = 0.10f; bgG = 0.07f; bgB = 0.09f;
            sidebarR = 0.15f; sidebarG = 0.10f; sidebarB = 0.13f;
            textR = 0.99f; textG = 0.95f; textB = 0.97f;
            break;
        case 1: // Unicore
            accentR = 0.20f; accentG = 0.46f; accentB = 0.96f;
            bgR = 0.05f; bgG = 0.07f; bgB = 0.14f;
            sidebarR = 0.07f; sidebarG = 0.09f; sidebarB = 0.18f;
            textR = 0.92f; textG = 0.95f; textB = 1.00f;
            break;
        case 2: // Refund / iRucifix
            accentR = 0.90f; accentG = 0.30f; accentB = 0.30f;
            bgR = 0.14f; bgG = 0.05f; bgB = 0.05f;
            sidebarR = 0.19f; sidebarG = 0.08f; sidebarB = 0.08f;
            textR = 0.98f; textG = 0.92f; textB = 0.92f;
            break;
        default:
            break;
        }

        Options.AccentR = accentR;
        Options.AccentG = accentG;
        Options.AccentB = accentB;
        Options.BgR = bgR;
        Options.BgG = bgG;
        Options.BgB = bgB;
        Options.SidebarR = sidebarR;
        Options.SidebarG = sidebarG;
        Options.SidebarB = sidebarB;
        Options.TextR = textR;
        Options.TextG = textG;
        Options.TextB = textB;

        const ImVec4 accent(accentR, accentG, accentB, 1.00f);
        const ImVec4 accentSoft(
            clamp01(accentR + 0.08f),
            clamp01(accentG + 0.08f),
            clamp01(accentB + 0.08f),
            1.00f);
        const ImVec4 accentDeep(
            accentR * 0.62f,
            accentG * 0.62f,
            accentB * 0.62f,
            1.00f);
        const ImVec4 text(textR, textG, textB, 1.00f);
        const ImVec4 textDisabled(
            textR * 0.72f,
            textG * 0.70f,
            textB * 0.72f,
            1.00f);
        const ImVec4 windowBg(bgR, bgG, bgB, 0.96f);
        const ImVec4 childBg(
            clamp01(bgR + 0.04f),
            clamp01(bgG + 0.02f),
            clamp01(bgB + 0.03f),
            0.97f);
        const ImVec4 popupBg(
            clamp01(bgR + 0.02f),
            clamp01(bgG + 0.01f),
            clamp01(bgB + 0.02f),
            0.98f);
        const ImVec4 border(
            clamp01(sidebarR + 0.20f),
            clamp01(sidebarG + 0.13f),
            clamp01(sidebarB + 0.16f),
            1.00f);

        colors[ImGuiCol_Text] = text;
        colors[ImGuiCol_TextDisabled] = textDisabled;
        colors[ImGuiCol_WindowBg] = windowBg;
        colors[ImGuiCol_ChildBg] = childBg;
        colors[ImGuiCol_PopupBg] = popupBg;
        colors[ImGuiCol_Border] = border;
        colors[ImGuiCol_BorderShadow] = ImVec4(0.20f, 0.20f, 0.20f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(sidebarR, sidebarG, sidebarB, 0.95f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(sidebarR + 0.04f, sidebarG + 0.03f, sidebarB + 0.04f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(sidebarR + 0.08f, sidebarG + 0.05f, sidebarB + 0.06f, 1.00f);
        colors[ImGuiCol_TitleBg] = popupBg;
        colors[ImGuiCol_TitleBgActive] = popupBg;
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(bgR, bgG, bgB, 0.85f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(sidebarR, sidebarG, sidebarB, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(bgR, bgG, bgB, 0.30f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(accent.x, accent.y, accent.z, 0.90f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(accentSoft.x, accentSoft.y, accentSoft.z, 0.96f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(accentSoft.x, accentSoft.y, accentSoft.z, 0.98f);
        colors[ImGuiCol_CheckMark] = text;
        colors[ImGuiCol_SliderGrab] = accent;
        colors[ImGuiCol_SliderGrabActive] = accentSoft;
        colors[ImGuiCol_Button] = ImVec4(sidebarR, sidebarG, sidebarB, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(sidebarR + 0.06f, sidebarG + 0.04f, sidebarB + 0.05f, 1.00f);
        colors[ImGuiCol_ButtonActive] = accentDeep;
        colors[ImGuiCol_Header] = ImVec4(accentDeep.x, accentDeep.y, accentDeep.z, 0.85f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(accent.x, accent.y, accent.z, 0.92f);
        colors[ImGuiCol_HeaderActive] = ImVec4(accentSoft.x, accentSoft.y, accentSoft.z, 0.95f);
        colors[ImGuiCol_Separator] = border;
        colors[ImGuiCol_SeparatorHovered] = accent;
        colors[ImGuiCol_SeparatorActive] = accentSoft;
        colors[ImGuiCol_ResizeGrip] = ImVec4(accent.x, accent.y, accent.z, 0.22f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(accent.x, accent.y, accent.z, 0.70f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(accentSoft.x, accentSoft.y, accentSoft.z, 0.92f);
        colors[ImGuiCol_Tab] = ImVec4(sidebarR, sidebarG, sidebarB, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(accentDeep.x, accentDeep.y, accentDeep.z, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(accent.x, accent.y, accent.z, 0.92f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(sidebarR, sidebarG, sidebarB, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(sidebarR + 0.03f, sidebarG + 0.02f, sidebarB + 0.03f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(accent.x, accent.y, accent.z, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = accent;
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

} // namespace Cheat

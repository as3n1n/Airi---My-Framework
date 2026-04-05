#pragma once

// ══════════════════════════════════════════════════════════
//  CHEAT.H — Phasmophobia
//  Déclarations des variables et fonctions de cheat
// ══════════════════════════════════════════════════════════

namespace Cheat
{
    // ═══════════════════════════════════════════════════
    //  VISUAL
    // ═══════════════════════════════════════════════════
    extern bool VIS_Watermark;
    extern bool VIS_StatsWindow;
    extern bool VIS_GhostProfile;
    extern bool VIS_EvidenceESP;
    extern bool VIS_PlayerESP;
    extern bool VIS_Fullbright;
    extern int VIS_FullbrightMode;
    extern float VIS_FullbrightIntensity;
    extern bool VIS_GhostESP;
    extern bool VIS_PlayersWindow;
    extern bool VIS_FuseBoxESP;
    extern bool VIS_ObjectESP;
    extern bool VIS_ESPDebugOverlay;
    extern bool VIS_GhostSkeletonESP;
    extern bool VIS_PlayerSkeletonESP;
    extern bool VIS_ObjectESPShowDoors;
    extern bool VIS_ObjectESPShowItems;
    extern bool VIS_ObjectESPShowProps;
    extern bool VIS_ObjectESPOnlyUsable;
    extern bool VIS_ObjectESPUseNameFilter;
    extern bool VIS_ObjectESPUseJsonFilter;
    extern bool VIS_ObjectESPExportJsonNow;
    extern float VIS_ObjectESPMaxDistance;
    extern int VIS_ObjectESPMaxDrawCount;
    extern int VIS_ObjectESPMaxPerName;
    extern float VIS_ESPLineThickness;
    extern bool VIS_EMFESP;
    extern bool VIS_ActivityMonitor;
    extern char VIS_ObjectESPExactName[128];
    extern char VIS_ObjectESPJsonPath[MAX_PATH];
    extern float VIS_ESPColorGhost[4];
    extern float VIS_ESPColorPlayer[4];
    extern float VIS_ESPColorEvidence[4];
    extern float VIS_ESPColorFuseBox[4];
    extern float VIS_ESPColorObject[4];
    extern float VIS_ESPColorEMF[4];

    // ═══════════════════════════════════════════════════
    //  PLAYER
    // ═══════════════════════════════════════════════════
    extern bool PLAYER_GodMode;
    extern bool PLAYER_FovEditor;
    extern float PLAYER_FovValue;
    extern int PLAYER_TeleportTarget;       // player index to teleport to
    extern int PLAYER_KillTarget;           // player index to kill
    extern int PLAYER_FreezeTarget;         // player index to freeze/unfreeze
    extern float PLAYER_SanityValue;        // 0-100
    extern bool PLAYER_SetSanity;
    extern bool PLAYER_RequestTeleport;
    extern bool PLAYER_RequestKill;
    extern bool PLAYER_RequestFreeze;
    extern bool PLAYER_RequestUnfreeze;
    extern bool PLAYER_RequestSuicide;
    extern bool PLAYER_RadioSpamNetwork;
    extern int PLAYER_RadioSpamDelayMs;
    extern bool PLAYER_TruckRadioSpamNetwork;
    extern int PLAYER_TruckRadioSpamDelayMs;
    extern int PLAYER_TruckRadioCategory;
    extern bool PLAYER_TruckRadioRandomClip;
    extern int PLAYER_TruckRadioClipIndex;
    extern bool PLAYER_MicSaturation;
    extern float PLAYER_MicSaturationVolume;
    extern bool PLAYER_HearDeadPlayers;
    extern bool PLAYER_AiriHeldTint;
    extern bool PLAYER_AiriHandsTint;
    extern float PLAYER_AiriTintColor[4];

    // ═══════════════════════════════════════════════════
    //  GHOST
    // ═══════════════════════════════════════════════════
    extern int GHOST_WeatherIndex;          // weather changer
    extern bool GHOST_WeatherEnabled;
    extern bool GHOST_CustomSpeed;
    extern float GHOST_SpeedValue;
    extern int GHOST_ForceStateIndex;
    extern bool GHOST_ForceStateEnabled;
    extern bool GHOST_ForceAbility;
    extern bool GHOST_ForceInteract;
    extern bool GHOST_ForceInteractDoor;
    extern bool GHOST_ForceInteractProp;
    extern bool GHOST_ForceNormalInteraction;
    extern bool GHOST_ForceTwinInteraction;
    extern bool GHOST_Designer;             // HOST - ghost/evidence config
    extern int GHOST_AudioIndex;
    extern bool GHOST_PlayAudio;
    extern bool GHOST_StopAudio;
    extern bool GHOST_AudioBroadcast;
    extern bool GHOST_AutoJournalSolver;
    extern bool GHOST_SolveJournalNow;

    // ═══════════════════════════════════════════════════
    //  CURSED ITEMS
    // ═══════════════════════════════════════════════════
    extern bool CURSED_BreakItems;
    extern bool CURSED_UseItems;
    extern int CURSED_NextTarotCard;        // force next tarot card index
    extern bool CURSED_InfinityCards;
    extern bool CURSED_TriggerOuijaBoard;
    extern bool CURSED_TriggerMusicBox;
    extern bool CURSED_TriggerTarotCards;
    extern bool CURSED_TriggerSummoningCircle;
    extern bool CURSED_TriggerHauntedMirror;
    extern bool CURSED_TriggerVoodooDoll;
    extern bool CURSED_TriggerMonkeyPaw;
    extern char CURSED_OuijaText[128];
    extern bool CURSED_OuijaNetworked;
    extern bool CURSED_OuijaApplyNow;

    // ═══════════════════════════════════════════════════
    //  MOVEMENT
    // ═══════════════════════════════════════════════════
    extern bool MOV_Teleport;               // teleport to target pos
    extern bool MOV_Freecam;
    extern float MOV_FreecamSpeed;
    extern bool MOV_NoClip;
    extern int MOV_NoClipMode;
    extern bool MOV_InfinityStamina;
    extern bool MOV_CustomSpeed;
    extern float MOV_SpeedValue;
    extern bool MOV_FlyingEquipment;
    extern float MOV_FlyingEquipmentRadius;
    extern float MOV_FlyingEquipmentHeight;
    extern float MOV_FlyingEquipmentSpeed;
    extern int MOV_FlyingEquipmentShape;
    extern bool LOBBY_EntityListEnabled;
    // ═══════════════════════════════════════════════════
    //  MORE
    // ═══════════════════════════════════════════════════
    extern bool MORE_AlwaysPerfectGame;
    extern bool MORE_CustomBonusReward;
    extern float MORE_BonusRewardValue;
    extern bool MORE_AntiKick;
    extern bool MORE_NoCameraRestrictions;
    extern bool MORE_DisableDoorInteraction;
    extern bool MORE_LockDoors;
    extern bool MORE_UnlockDoors;
    extern bool MORE_CloseDoors;
    extern bool MORE_CloseDoorsLoop;
    extern int MORE_CloseDoorsLoopDelayMs;
    extern bool MORE_LockUnlockDoorsLoop;
    extern int MORE_LockUnlockDoorsLoopDelayMs;
    extern bool MORE_OpenCloseDoorsLoop;
    extern int MORE_OpenCloseDoorsLoopDelayMs;
    extern bool MORE_WalkThroughDoors;
    extern bool MORE_Pickup;
    extern float MORE_ThrowStrength;
    extern bool MORE_CanPickup;
    extern bool MORE_PocketEverything;
    extern bool MORE_PickupAnyProp;
    extern bool MORE_GrabAllKeys;
    extern bool MORE_LeavePeople;
    extern bool MORE_CompleteMissions;
    extern bool MORE_ButterFingers;
    extern bool MORE_DropHeldLocal;
    extern bool MORE_DropHeldTarget;
    extern bool MORE_DropAllInventoryLocal;
    extern bool MORE_DropAllInventoryTarget;
    extern bool MORE_DropAllInventoryAll;
    extern bool MORE_DropHeldFiltered;
    extern bool MORE_DropAllInventoryFiltered;
    extern int MORE_DropTargetSlot;
    extern int MORE_DropAllFilterSlotMin;
    extern int MORE_DropAllFilterSlotMax;
    extern int MORE_DropAllFilterOwnerMode;
    extern char MORE_DropAllFilterOwner[64];
    extern bool MORE_CustomName;
    extern char MORE_NameBuffer[64];
    extern bool MORE_ApplyCustomNameNow;
    extern bool MORE_SetBadge;
    extern int MORE_BadgeIndex;
    extern bool MORE_SetBadgeMultiplayer;
    extern bool MORE_SetBadgeNetworked;
    extern bool MORE_SetBadgeNetworkedPersistent;
    extern bool MORE_DisconnectPlayer;
    extern int MORE_DisconnectTarget;
    extern int MORE_DisconnectMode;         // 0-6, see DisconnectMethod enum
    extern bool MORE_SpamFlashlight;
    extern int MORE_SpamFlashlightMode;
    extern int MORE_SpamFlashlightDelayMs;
    extern bool MORE_TargetFlashlightTroll;
    extern bool MORE_TorchUseSpam;
    extern int MORE_TorchUseSpamDelayMs;
    extern bool MORE_DoorSlamAll;
    extern bool MORE_DoorRattleAll;
    extern bool MORE_DoorLockSoundAll;
    extern bool MORE_DoorUnlockSoundAll;
    extern bool MORE_TargetDoorClose;
    extern bool MORE_TargetDoorLock;
    extern bool MORE_TargetDoorSlam;
    extern bool MORE_TargetDoorRattle;
    extern bool MORE_TargetDoorLockSound;
    extern bool MORE_TargetDoorUnlockSound;
    extern float MORE_TargetDoorRadius;

    // NETWORK / MATCHMAKING
    extern bool NETWORK_QuickRejoin;
    extern bool NETWORK_ConnectUsingSettings;
    extern bool NETWORK_Reconnect;
    extern bool NETWORK_JoinRandomRoom;
    extern bool NETWORK_JoinRandomRoomFiltered;
    extern bool NETWORK_JoinRandomOrCreateRoomFiltered;
    extern bool NETWORK_JoinRoomByName;
    extern bool NETWORK_JoinOrCreateRoom;
    extern bool NETWORK_CreateRoomByName;
    extern bool NETWORK_LeaveRoom;
    extern bool NETWORK_LeaveLobby;
    extern bool NETWORK_DisconnectSelf;
    extern bool NETWORK_RoomSniper;
    extern bool NETWORK_StealHost;
    extern bool NETWORK_AutoFarm;
    extern bool NETWORK_AutoFarmAutoRerun;
    extern bool NETWORK_AutoFarmUseMinPlayers;
    extern bool NETWORK_AutoFarmRefreshContracts;
    extern int NETWORK_CreateRoomMaxPlayers;
    extern int NETWORK_FilterExpectedMaxPlayers;
    extern int NETWORK_FilterMatchmakingMode;
    extern int NETWORK_FilterLobbyType;
    extern int NETWORK_RoomSniperDelayMs;
    extern int NETWORK_AutoFarmStartDelaySec;
    extern int NETWORK_AutoFarmEndDelaySec;
    extern int NETWORK_AutoFarmMaxRoundSec;
    extern int NETWORK_AutoFarmLobbyTimeoutSec;
    extern int NETWORK_AutoFarmMinPlayers;
    extern int NETWORK_AutoFarmContractMode;
    extern char NETWORK_RoomNameBuffer[64];
    extern char NETWORK_LobbyNameBuffer[64];
    extern char NETWORK_SqlFilterBuffer[128];
    extern bool NETWORK_WebhookEnabled;
    extern bool NETWORK_WebhookLogSession;
    extern bool NETWORK_WebhookLogRooms;
    extern bool NETWORK_WebhookLogContracts;
    extern bool NETWORK_WebhookLogDeaths;
    extern bool NETWORK_WebhookLogCommands;
    extern char NETWORK_WebhookUrl[512];

    // ═══════════════════════════════════════════════════
    //  CONFIG
    // ═══════════════════════════════════════════════════
    extern bool CFG_DifficultyChanger;
    extern int CFG_DifficultyIndex;

    // ── Profile (account) edits ──
    extern int PROFILE_CashDelta;
    extern int PROFILE_CashTarget;
    extern int PROFILE_XPDelta;
    extern bool PROFILE_AddCash;
    extern bool PROFILE_AddXP;
    extern bool PROFILE_SetCash;
    extern int PROFILE_CashMode;
    extern bool PROFILE_AutoEndGameBonus;
    extern bool PROFILE_ApplyEndGameBonusNow;
    extern int PROFILE_EndGameCashBonus;
    extern int PROFILE_EndGameXPBonus;
    extern int PROFILE_ItemCount;
    extern bool PROFILE_SetItems;
    extern bool PROFILE_SetLevel;
    extern int PROFILE_LevelValue;
    extern bool PROFILE_SetPrestige;
    extern int PROFILE_PrestigeValue;
    extern bool PROFILE_CashOverride;
    extern int PROFILE_CashOverrideValue;
    extern bool PROFILE_AutoSaveAfterEdit;
    extern bool PROFILE_BackupSaveNow;
    extern bool PROFILE_ReloadSaveNow;

    // ═══════════════════════════════════════════════════
    //  FONCTIONS
    // ═══════════════════════════════════════════════════
    bool Setup();
    void Shutdown();
    void OnFrame();
    void BeforeFrame();
    void ApplyThemeFromOptions();
    void PushHotkeyMessage(const char* feature, bool enabled, const char* keyName);
    void PushNotification(const char* title, const char* detail = nullptr);
    void Visuals_OnFrame();
    void Webhook_Setup();
    void Webhook_Shutdown();
    void Webhook_Send(const char* event, const char* detail);
    void Webhook_LogCommand(const char* event, const char* detail);
    void Webhook_TrackState();

    // Fonctions de misc_features.cpp
    void PhasmoFeatures_OnFrame();

    // Fonctions ESP (esp.cpp)
    void ESP_OnFrame();

} // namespace Cheat

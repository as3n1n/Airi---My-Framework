#pragma once
#include <cstdint>

// ══════════════════════════════════════════════════════════
//  PHASMO_OFFSETS.H — Phasmophobia
//  Offsets extraits du dump Il2Cpp (Il2CppDumper v6.7.46)
//  TypeDefIndex et field offsets des classes du jeu
// ══════════════════════════════════════════════════════════

namespace Phasmo
{
    using std::int32_t;
    using std::uint8_t;
    using std::uintptr_t;

    // ═══════════════════════════════════════════════════
    //  TypeDefIndex (pour il2cpp_class_from_type)
    // ═══════════════════════════════════════════════════
constexpr int32_t TDI_GameController         = 373;
constexpr int32_t TDI_LevelController        = 393;
constexpr int32_t TDI_EvidenceController     = 355;
constexpr int32_t TDI_CursedItemsController  = 348;
constexpr int32_t TDI_Player                 = 1060;
constexpr int32_t TDI_PlayerSanity           = 1072;
constexpr int32_t TDI_PlayerStamina          = 1078;
constexpr int32_t TDI_PlayerStats            = 1079;
constexpr int32_t TDI_PlayerCharacter        = 1063;
constexpr int32_t TDI_PlayerEquipment        = 410;
constexpr int32_t TDI_GhostAI               = 241;
constexpr int32_t TDI_GhostActivity          = 231;
constexpr int32_t TDI_GhostInfo              = 246;
constexpr int32_t TDI_GhostInteraction       = 247;
constexpr int32_t TDI_Door                   = 611;
constexpr int32_t TDI_FuseBox                = 618;
constexpr int32_t TDI_EMF                    = 222;
constexpr int32_t TDI_Evidence               = 511;
constexpr int32_t TDI_DNAEvidence            = 508;
constexpr int32_t TDI_Key                    = 715;
constexpr int32_t TDI_Difficulty             = 205;
constexpr int32_t TDI_TarotCard              = 770;
constexpr int32_t TDI_TarotCards             = 773;
constexpr int32_t TDI_CursedItem             = 687;
constexpr int32_t TDI_WeatherControl         = 1390;
constexpr int32_t TDI_PhotonNetwork          = 15511;
constexpr int32_t TDI_SanityDrainer          = 251;
constexpr int32_t TDI_SanityEffectsController = 452;

    // ═══════════════════════════════════════════════════
    //  STATIC FIELD OFFSET (singleton instance = 0x0)
    //  All singleton controllers: static ClassName instance at 0x0
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t STATIC_INSTANCE = 0x0;

    // Unity Engine method RVAs
constexpr uintptr_t RVA_Camera_get_main          = 0x4B85630;
// Fixed from Il2CppDumper dump (2026-04-02) — were swapped before
constexpr uintptr_t RVA_Camera_get_fieldOfView   = 0x4B855B0;
constexpr uintptr_t RVA_Camera_set_fieldOfView   = 0x4B862C0;
// Updated camera projection RVAs from 2026-04-02 dump
constexpr uintptr_t RVA_Camera_WorldToScreenPoint = 0x4B84F60;
constexpr uintptr_t RVA_Camera_WorldToScreenPoint_Injected = 0x4B84EF0;
constexpr uintptr_t RVA_Camera_get_pixelWidth = 0x4B858F0;
constexpr uintptr_t RVA_Camera_get_pixelHeight = 0x4B85810;
constexpr uintptr_t RVA_UnityEngine_Behaviour_set_enabled = 0x4BBEFE0;
constexpr uintptr_t RVA_UnityEngine_Light_set_intensity = 0x4B938A0;
constexpr uintptr_t RVA_UnityEngine_Light_set_color = 0x4B936D0;
constexpr uintptr_t RVA_UnityEngine_Light_set_shadowStrength = 0x4B93B80;
constexpr uintptr_t RVA_UnityEngine_Light_set_bounceIntensity = 0x4B93630;
constexpr uintptr_t RVA_UnityEngine_Light_set_range = 0x4B93940;
constexpr uintptr_t RVA_UnityEngine_Light_set_shadowBias = 0x4B93A10;
constexpr uintptr_t RVA_UnityEngine_Light_set_shadowNormalBias = 0x4B93AF0;
constexpr uintptr_t RVA_UnityEngine_Light_set_shadows = 0x4B93BD0;
constexpr uintptr_t RVA_UnityEngine_Light_set_renderMode = 0x4B93990;
constexpr uintptr_t RVA_UnityEngine_Resources_FindObjectsOfTypeAll = 0x4BCC490;
constexpr uintptr_t RVA_UnityEngine_Component_get_transform = 0x4BBF9F0;
constexpr uintptr_t RVA_UnityEngine_Transform_get_position = 0x4BD4930;
constexpr uintptr_t RVA_UnityEngine_Component_get_gameObject = 0x4BBF940;
constexpr uintptr_t RVA_UnityEngine_Component_GetComponent_Type = 0x4BBF650;
constexpr uintptr_t RVA_UnityEngine_Component_GetComponentInChildren_Type = 0x4BBF4E0;
constexpr uintptr_t RVA_UnityEngine_Collider_set_enabled = 0x4C4A560;
constexpr uintptr_t RVA_UnityEngine_Renderer_get_bounds = 0x4BA2C00;
constexpr uintptr_t RVA_UnityEngine_Animator_GetBoneTransform = 0x4B78E30;
constexpr uintptr_t RVA_UnityEngine_Animator_GetBoneTransformInternal = 0x4B78DF0;
constexpr uintptr_t RVA_UnityEngine_SkinnedMeshRenderer_get_rootBone = 0x4BA4F10;
constexpr uintptr_t RVA_UnityEngine_SkinnedMeshRenderer_get_bones = 0x4BA4E90;
constexpr uintptr_t RVA_RenderSettings_set_fog = 0x4BA1F60;
constexpr uintptr_t RVA_RenderSettings_set_fogColor = 0x4BA1EE0;
constexpr uintptr_t RVA_RenderSettings_set_fogDensity = 0x4BA1F20;
constexpr uintptr_t RVA_RenderSettings_set_ambientSkyColor = 0x4BA1E60;
constexpr uintptr_t RVA_RenderSettings_set_ambientEquatorColor = 0x4BA1CA0;
constexpr uintptr_t RVA_RenderSettings_set_ambientGroundColor = 0x4BA1D20;
constexpr uintptr_t RVA_RenderSettings_set_ambientIntensity = 0x4BA1D60;
constexpr uintptr_t RVA_RenderSettings_set_ambientLight = 0x4BA1DE0;
constexpr uintptr_t RVA_QualitySettings_set_pixelLightCount = 0x4B9F220;
constexpr uintptr_t RVA_QualitySettings_set_shadowCascades = 0x4B9F260;
constexpr uintptr_t RVA_QualitySettings_set_shadowDistance = 0x4B9F2A0;

constexpr uintptr_t RVA_Door_UnlockDoor = 0x23B7560;
constexpr uintptr_t RVA_Door_LockDoor = 0x23B47B0;
constexpr uintptr_t RVA_Door_ToggleDoorLock = 0x23B7210;
constexpr uintptr_t RVA_Door_AttemptToUnlockDoor = 0x23B3650;
constexpr uintptr_t RVA_Door_HuntingCloseDoor = 0x23BD430;
constexpr uintptr_t RVA_Door_OpenDoor = 0x23BDF60;
constexpr uintptr_t RVA_Door_PlayDoorSlamNoise = 0x23B6160;
constexpr uintptr_t RVA_Door_PlayDoorRattlingNoise = 0x23BB2A0;
constexpr uintptr_t RVA_Door_NetworkedPlayLockSound = 0x23B5C60;
constexpr uintptr_t RVA_Door_NetworkedPlayUnlockSound = 0x23B5D10;
constexpr uintptr_t RVA_GhostController_GetFullEvidence = 0x2054960;
constexpr uintptr_t RVA_JournalController_AddEvidence   = 0x2062FF0;
constexpr uintptr_t RVA_JournalController_FilterOutEvidence = 0x2063430;
constexpr uintptr_t RVA_JournalController_OpenCloseJournal = 0x2064450;
constexpr uintptr_t RVA_JournalController_SelectGhost   = 0x2065900;
constexpr uintptr_t RVA_JournalController_SetGhostTypes = 0x20659F0;
constexpr uintptr_t RVA_NavMeshAgent_get_speed           = 0x4B629B0;
constexpr uintptr_t RVA_NavMeshAgent_set_speed           = 0x4B62EC0;

    // ═══════════════════════════════════════════════════
    //  PLAYER OFFSETS (TypeDefIndex: 1064)
    //  Inherits: MonoBehaviourPunCallbacks
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t PL_PHOTON_VIEW             = 0x20;
    constexpr uintptr_t PL_PLAYER_CHARACTER        = 0x30;
    constexpr uintptr_t PL_CAMERA                  = 0x58;
    constexpr uintptr_t PL_PLAYER_SANITY           = 0xC0;
    constexpr uintptr_t PL_PLAYER_STATS            = 0xC8;
    constexpr uintptr_t PL_PLAYER_SENSORS          = 0x100;
    constexpr uintptr_t PL_PLAYER_STAMINA          = 0x108;
    constexpr uintptr_t PL_PHYSICS_CHAR_CTRL       = 0x118;
    constexpr uintptr_t PL_FIRST_PERSON_CTRL       = 0x128;
    constexpr uintptr_t PL_PC_FLASHLIGHT          = 0x160;

    // ═══════════════════════════════════════════════════
    //  PLAYER RVA (method addresses)
    // ═══════════════════════════════════════════════════
constexpr uintptr_t RVA_Player_KillPlayer                = 0xB75E10;
constexpr uintptr_t RVA_Player_Teleport                  = 0xB76F20;
constexpr uintptr_t RVA_Player_SetPlayerSpeed            = 0xB76660;
constexpr uintptr_t RVA_Player_ToggleFreezePlayer        = 0xB77030;
constexpr uintptr_t RVA_Player_Dead                      = 0xB748C0;
constexpr uintptr_t RVA_Player_DeadRoomEffects           = 0xB74870;
constexpr uintptr_t RVA_Player_SpawnDeadBodyNetworked     = 0xB76370;
constexpr uintptr_t RVA_CursedItem_BreakItem            = 0x24F02E0;
constexpr uintptr_t RVA_CursedItem_TryUse               = 0x24F0640;
constexpr uintptr_t RVA_OuijaBoard_PlayMessageSequence = 0x600300;
constexpr uintptr_t RVA_OuijaBoard_NetworkedUse        = 0x5FFFD0;
constexpr uintptr_t RVA_Player_StartKillingPlayerNetworked = 0xB763E0;
constexpr uintptr_t RVA_Player_Update                    = 0xB7D7F0;
constexpr uintptr_t RVA_Player_ApplyHeadBob              = 0xB741D0;
constexpr uintptr_t RVA_Player_ApplySprintMode           = 0xB74510;
constexpr uintptr_t RVA_Player_RevivePlayer              = 0xB76070;
constexpr uintptr_t RVA_PlayerAudio_SetVolume            = 0xB80740;
constexpr uintptr_t RVA_CCTVTruckTrigger_Exit            = 0xFE1840;
constexpr uintptr_t RVA_CCTVTruckTrigger_ThereAreAlivePlayersOutsideTheTruck = 0xFE1FE0;
constexpr uintptr_t RVA_CCTVTruckTrigger_IsThisPlayersInsideTheTruck = 0xFE1DB0;
constexpr uintptr_t RVA_PlayerAudio_GetPlayerVoiceAudioSource = 0xB7E830;
constexpr uintptr_t RVA_PlayerAudio_GetPlayerVoiceAmplitude = 0xB7E650;
constexpr uintptr_t RVA_PlayerAudio_ToggleMuteAllChannels = 0xB7F4A0;
constexpr uintptr_t RVA_VivoxVoiceManager_SetBlockVivoxCommunication_Spot = 0x11BC910;
constexpr uintptr_t RVA_WalkieTalkie_TurnOn              = 0xC08DD0;
constexpr uintptr_t RVA_WalkieTalkie_TurnOff             = 0xC08BD0;
constexpr uintptr_t RVA_WalkieTalkie_Use                 = 0xC0D130;
constexpr uintptr_t RVA_WalkieTalkie_Stop                = 0xC08B20;

    // PCFlashlight RVA
constexpr uintptr_t RVA_PCFlashlight_EnableOrDisableLight = 0xAF3760;
constexpr uintptr_t RVA_Equipment_Use = 0x2520600;

    // ═══════════════════════════════════════════════════
    //  PLAYER SANITY OFFSETS (TypeDefIndex: 1076)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t PS_PHOTON_VIEW             = 0x20;
    constexpr uintptr_t PS_PLAYER                  = 0x28;
    constexpr uintptr_t PS_SANITY_VALUE            = 0x30;  // float
    constexpr uintptr_t PS_RENDER_TEXTURE          = 0x40;
    constexpr uintptr_t PS_CANDLE_LIST             = 0x60;

    // PlayerSanity RVA
constexpr uintptr_t RVA_PlayerSanity_SetInsanity         = 0xB9E0D0;
constexpr uintptr_t RVA_PlayerSanity_UpdatePlayerSanity  = 0xBA7590;
constexpr uintptr_t RVA_PlayerSanity_DrainSanity         = 0xB9DE70;
constexpr uintptr_t RVA_PlayerSanity_RestoreSanity       = 0xB9DE70;
constexpr uintptr_t RVA_PlayerSanity_NetworkedUpdate     = 0xB9E040;

    // ═══════════════════════════════════════════════════
    //  PLAYER STAMINA OFFSETS (TypeDefIndex: 1082)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t PST_AUDIO_SOURCE           = 0x20;
    constexpr uintptr_t PST_BREATH_MALE            = 0x28;
    constexpr uintptr_t PST_BREATH_FEMALE          = 0x30;
    constexpr uintptr_t PST_PLAYER                 = 0x38;
    // bool flags at 0x42-0x47
    constexpr uintptr_t PST_STAMINA_EVENTS_A       = 0x58;
    constexpr uintptr_t PST_STAMINA_EVENTS_B       = 0x60;

    // PlayerStamina RVA
constexpr uintptr_t RVA_PlayerStamina_PlayOutOfBreathSound = 0xBB3190;
constexpr uintptr_t RVA_PlayerStamina_PreventDrainForTime  = 0xBB3300;

    // ═══════════════════════════════════════════════════
    //  GHOST AI OFFSETS (TypeDefIndex: 241)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t GA_GHOST_INFO              = 0x38;
    constexpr uintptr_t GA_NAV_MESH_AGENT          = 0x40;
    constexpr uintptr_t GA_GHOST_AUDIO             = 0x48;
    constexpr uintptr_t GA_GHOST_INTERACTION       = 0x50;
    constexpr uintptr_t GA_GHOST_ACTIVITY          = 0x58;
    constexpr uintptr_t GA_GHOST_MODEL             = 0x60;
    constexpr uintptr_t GA_LOS_SENSOR              = 0xA8;
    constexpr uintptr_t GA_WHITE_SAGE              = 0x100;
    constexpr uintptr_t GA_TARGET_PLAYER           = 0x118;
    constexpr uintptr_t GA_TARGET_PLAYER_ID        = 0x120;
    constexpr uintptr_t GA_LAST_KNOWN_PLAYER_POS   = 0x124;

    constexpr uintptr_t SM_MULTIPLAYER_PROFILES   = 0xC8;
    constexpr uintptr_t SM_SELECTED_CONTRACT      = 0x138;
    constexpr uintptr_t LSM_CONTRACTS             = 0x30;
    constexpr uintptr_t LSM_SELECTED_CONTRACT     = 0x38;
    constexpr uintptr_t CONTRACT_MAP              = 0x60;
    constexpr uintptr_t PC_PLAYER_SPOT            = 0x90;
    constexpr uintptr_t PC_PLAYER                 = 0x98;

    // MultiplayerProfile RVA (TypeDefIndex: 1504)
constexpr uintptr_t RVA_MultiplayerProfile_UpdateUI      = 0x11383F0;

    // GhostAI RVA
constexpr uintptr_t RVA_GhostAI_Init                    = 0x1DA9C60;
constexpr uintptr_t RVA_GhostAI_ChangeState             = 0x1DA84E0;
constexpr uintptr_t RVA_GhostAI_MakeGhostAppear         = 0x1DAA6D0;
constexpr uintptr_t RVA_GhostAI_GetNearestPlayer         = 0x1DA92B0;
constexpr uintptr_t RVA_GhostAI_StopGhostFromHunting    = 0x1DAB6E0;
constexpr uintptr_t RVA_GhostAI_LookAtPlayer             = 0x1DAA560;
constexpr uintptr_t RVA_GhostAI_SetNewBansheeTarget     = 0x1DAAF60;
constexpr uintptr_t RVA_GhostAI_DelayTeleportToFavRoom  = 0x1DA8FD0;
constexpr uintptr_t RVA_GhostAI_Awake                   = 0x1DA8410;
constexpr uintptr_t RVA_GhostAI_IsNearActiveGhost       = 0x1DA9D00;

    // GhostAudio RVA (TypeDefIndex: 243)
constexpr uintptr_t RVA_GhostAudio_PlaySound            = 0x1DBD720;
constexpr uintptr_t RVA_GhostAudio_StopSound            = 0x1DBDB00;
constexpr uintptr_t RVA_GhostAudio_PlaySoundNetworked   = 0x1DBD620;
constexpr uintptr_t RVA_GhostAudio_StopSoundNetworked   = 0x1DBDA90;

    // ═══════════════════════════════════════════════════
    //  GHOST ACTIVITY OFFSETS (TypeDefIndex: 231)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t GAC_RANGE_SENSOR_A         = 0x20;
    constexpr uintptr_t GAC_GHOST_AI               = 0x28;
    constexpr uintptr_t GAC_RANGE_SENSOR_B         = 0x30;
    constexpr uintptr_t GAC_LEVEL_CONTROLLER       = 0x40;

    // GhostActivity RVA
constexpr uintptr_t RVA_GhostActivity_GhostAbility      = 0x1D94000;
constexpr uintptr_t RVA_GhostActivity_InteractWithARandomDoor = 0x1D94840;
constexpr uintptr_t RVA_GhostActivity_InteractWithARandomProp = 0x1D94AE0;
constexpr uintptr_t RVA_GhostActivity_NormalInteraction = 0x1D94F40;
constexpr uintptr_t RVA_GhostActivity_TryInteract        = 0x1D953C0;
constexpr uintptr_t RVA_GhostActivity_TwinInteraction   = 0x1D95520;
constexpr uintptr_t RVA_GhostActivity_GetPropsToThrow   = 0x1D93EB0;

    // ═══════════════════════════════════════════════════
    //  GAME CONTROLLER OFFSETS (TypeDefIndex: 372)
    //  Static instance at 0x0
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t GC_LEVEL_CONTROLLER        = 0xE8;
    constexpr uintptr_t GC_MULTIPLAYER_CONTROLLER  = 0xF0;
    constexpr uintptr_t GC_MATERIAL                = 0x100;
    constexpr uintptr_t GC_SPEECH_RECOGNIZER       = 0x110;

    // GameController RVA
constexpr uintptr_t RVA_GameController_Exit              = 0x202FEA0;
constexpr uintptr_t RVA_GameController_ResetHuntEffects  = 0x2030950;
constexpr uintptr_t RVA_GameController_Awake             = 0x202FE10;

    // ═══════════════════════════════════════════════════
    //  LEVEL CONTROLLER OFFSETS (TypeDefIndex: 392)
    //  Static instance at 0x0
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t LC_GHOST_AI                = 0x40;
    constexpr uintptr_t LC_DOORS                   = 0x48;
    constexpr uintptr_t LC_LEVEL_ROOMS_A           = 0x50;
    constexpr uintptr_t LC_LEVEL_ROOMS_B           = 0x58;
    constexpr uintptr_t LC_FUSE_BOX                = 0x80;
    constexpr uintptr_t LC_GAME_CONTROLLER         = 0x88;
    constexpr uintptr_t LC_ITEM_SPAWNER            = 0x98;
    constexpr uintptr_t LC_KEY                     = 0xE0;
    constexpr uintptr_t LC_DOORS_B                 = 0xA0;
    constexpr uintptr_t LC_FLOAT_LIST              = 0x108;

    // LevelController RVA
constexpr uintptr_t RVA_LevelController_GetRoomFromName  = 0x209D860;
constexpr uintptr_t RVA_LevelController_BlockCloset      = 0x209D7B0;
constexpr uintptr_t RVA_LevelController_SyncGhostRoom    = 0x209DF30;
constexpr uintptr_t RVA_LevelController_Awake            = 0x209A940;

    // LevelValues RVA
constexpr uintptr_t RVA_LevelValues_GetObjectivesTypes   = 0x20D40A0;

    // RandomWeather RVA
constexpr uintptr_t RVA_RandomWeather_SetWeatherProfile  = 0x21B43E0;
constexpr uintptr_t RVA_RandomWeather_TransitionWeatherProfile = 0x21B4640;

    // ═══════════════════════════════════════════════════
    //  EVIDENCE CONTROLLER OFFSETS (TypeDefIndex: 354)
    //  Static instance at 0x0
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t EC_EVIDENCE_LIST           = 0x20;
    constexpr uintptr_t EC_LEVEL_ROOMS             = 0x28;
    constexpr uintptr_t EC_PHOTON_VIEW             = 0x30;
    constexpr uintptr_t EC_DNA_EVIDENCE            = 0x38;
    constexpr uintptr_t EC_GHOST_ORB_TRANSFORM     = 0x40;
    constexpr uintptr_t EC_PARTICLE_RENDERER       = 0x48;
    constexpr uintptr_t EC_LEVEL_CONTROLLER        = 0x50;
    constexpr uintptr_t EC_FINGERPRINTS            = 0x60;

    // EvidenceController RVA
constexpr uintptr_t RVA_EvidenceController_ForceRespawnGhostOrb = 0x2020520;

    // ═══════════════════════════════════════════════════
    //  CURSED ITEMS CONTROLLER OFFSETS (TypeDefIndex: 347)
    //  Static instance at 0x0
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t CI_OUIJA_BOARD             = 0x20;
    constexpr uintptr_t CI_MUSIC_BOX               = 0x28;
    constexpr uintptr_t CI_TAROT_CARDS             = 0x30;
    constexpr uintptr_t CI_SUMMONING_CIRCLE        = 0x38;
    constexpr uintptr_t CI_HAUNTED_MIRROR          = 0x40;
    constexpr uintptr_t CI_VOODOO_DOLL             = 0x48;
    constexpr uintptr_t CI_MONKEY_PAW              = 0x50;
    constexpr uintptr_t CI_CURSED_ITEM_TYPES       = 0x90;

    // ═══════════════════════════════════════════════════
    //  DOOR OFFSETS (TypeDefIndex: 613)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t DR_BOOL_FLAGS_START        = 0x20;  // 0x20-0x22
    constexpr uintptr_t DR_AUDIO_SOURCE            = 0x68;
    constexpr uintptr_t DR_OCCLUSION_PORTAL        = 0x70;
    constexpr uintptr_t DR_PHOTON_VIEW             = 0x88;
    constexpr uintptr_t DR_RIGIDBODY               = 0x90;
    constexpr uintptr_t DR_PHOTON_INTERACT         = 0x98;
    constexpr uintptr_t DR_FINGERPRINT             = 0xA0;
    constexpr uintptr_t DR_COLLIDER                = 0xA8;
    constexpr uintptr_t DR_CONFIG_JOINT            = 0xC0;
    constexpr uintptr_t DR_DOOR_STATE_A            = 0xD4;  // DoorState enum
    constexpr uintptr_t DR_DOOR_STATE_B            = 0xDC;
    constexpr uintptr_t DR_ROTATION_PARAMS         = 0xE8;  // float params 0xE8-0xF4
    constexpr uintptr_t DR_CONNECTED_DOOR_A        = 0x128;
    constexpr uintptr_t DR_CONNECTED_DOOR_B        = 0x130;

    // ═══════════════════════════════════════════════════
    //  FUSE BOX OFFSETS (TypeDefIndex: 620)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t FB_TYPE                    = 0x28;   // FuseBoxType enum
    constexpr uintptr_t FB_RENDERERS               = 0x30;
    constexpr uintptr_t FB_LIGHTS                  = 0x38;
    constexpr uintptr_t FB_AUDIO_SOURCE            = 0x68;
    constexpr uintptr_t FB_IS_ON                   = 0x90;   // bool
    constexpr uintptr_t FB_LIGHT_SWITCHES          = 0x98;
    constexpr uintptr_t FB_PHOTON_INTERACT         = 0xA0;
    constexpr uintptr_t FB_HANDLE_TRANSFORMS       = 0xB0;   // 0xB0, 0xB8, 0xC0
    constexpr uintptr_t FB_HANDLE_ANGLE_ON         = 0xC8;
    constexpr uintptr_t FB_HANDLE_ANGLE_OFF        = 0xD4;

    // ═══════════════════════════════════════════════════
    //  EMF OFFSETS (TypeDefIndex: 222)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t EMF_EVIDENCE               = 0x20;
    constexpr uintptr_t EMF_LEVEL                  = 0x28;   // int
    constexpr uintptr_t EMF_TYPE                   = 0x30;   // EMF.Type enum

    // ═══════════════════════════════════════════════════
    //  EVIDENCE OFFSETS (TypeDefIndex: 513)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t EV_MEDIA_VALUES            = 0x28;
    constexpr uintptr_t EV_BOOL_A                  = 0x30;
    constexpr uintptr_t EV_BOOL_B                  = 0x31;
    constexpr uintptr_t EV_ON_MEDIA_TAKEN          = 0x38;

    //  MEDIA VALUES OFFSETS (ScriptableObject)
    constexpr uintptr_t MV_EVIDENCE_TYPE            = 0x18;

    // ═══════════════════════════════════════════════════
    //  DNA EVIDENCE OFFSETS (TypeDefIndex: 510)
    // ═══════════════════════════════════════════════════
constexpr uintptr_t RVA_DNAEvidence_PhysicsDelayNetworked = 0x21A63B0;

    // ═══════════════════════════════════════════════════
    //  DIFFICULTY OFFSETS (TypeDefIndex: 205)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t DF_DIFFICULTY_ENUM         = 0x18;
    constexpr uintptr_t DF_NAME_KEY                = 0x20;
    constexpr uintptr_t DF_DESCRIPTION_KEY         = 0x28;
    constexpr uintptr_t DF_REQUIRED_LEVEL          = 0x30;
    constexpr uintptr_t DF_SANITY_PILL_RESTORE     = 0x34;
    constexpr uintptr_t DF_STARTING_SANITY         = 0x38;
    constexpr uintptr_t DF_SANITY_DRAIN            = 0x3C;
    constexpr uintptr_t DF_SPRINTING               = 0x40;
    constexpr uintptr_t DF_FLASHLIGHTS             = 0x44;
    constexpr uintptr_t DF_PLAYER_SPEED            = 0x48;

constexpr uintptr_t RVA_ServerManager_KickPlayer = 0x96D580;
constexpr uintptr_t RVA_ServerManager_StartGame  = 0x96FB30;
constexpr uintptr_t RVA_ServerManager_Ready      = 0x96E8D0;
    constexpr uintptr_t DF_EVIDENCE_GIVEN          = 0x4C;
    constexpr uintptr_t DF_GHOST_SPEED             = 0x78;
    constexpr uintptr_t DF_SETUP_TIME              = 0x7C;
    constexpr uintptr_t DF_WEATHER                 = 0x80;
    constexpr uintptr_t DF_DOORS                   = 0x84;
    constexpr uintptr_t DF_HIDING_PLACES           = 0x88;
    constexpr uintptr_t DF_MONITORS                = 0x8C;
    constexpr uintptr_t DF_FUSE_BOX_A              = 0x8E;
    constexpr uintptr_t DF_FUSE_BOX_B              = 0x90;
    constexpr uintptr_t DF_CURSED_POSSESSIONS_QTY  = 0x94;

    // Difficulty RVA
constexpr uintptr_t RVA_Difficulty_GetRewardMultiplier   = 0x1D71530;
constexpr uintptr_t RVA_Difficulty_Clone                 = 0x1D70F80;

    // LevelValues RVA
constexpr uintptr_t RVA_LevelValues_GetInvestigationBonusReward = 0x20D3FB0;
constexpr uintptr_t RVA_LevelValues_IsPerfectGame        = 0x20D4210;

    // ═══════════════════════════════════════════════════
    //  TAROT CARD OFFSETS (TypeDefIndex: 772)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t TC_TAROT_CARDS_REF         = 0x28;
    constexpr uintptr_t TC_COLLIDER                = 0x40;
    constexpr uintptr_t TC_SMOKE_PARTICLE          = 0x48;
    constexpr uintptr_t TC_SPARKS_PARTICLE         = 0x50;
    constexpr uintptr_t TC_MESHES                  = 0x58;

    // ═══════════════════════════════════════════════════
    //  KEY OFFSETS (TypeDefIndex: 717)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t KY_PHOTON_INTERACT         = 0x28;
    constexpr uintptr_t KY_KEY_INFO                = 0x30;

    // Key RVA
constexpr uintptr_t RVA_Key_GrabbedKey                  = 0x591250;

    // ═══════════════════════════════════════════════════
    //  PHOTON NETWORK (TypeDefIndex: 15500)
    //  Static fields
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t PN_GAME_VERSION            = 0x0;
    constexpr uintptr_t PN_NETWORKING_CLIENT       = 0x8;   // LoadBalancingClient
    constexpr uintptr_t PN_MAX_VIEW_IDS            = 0x10;
    constexpr uintptr_t PN_SERVER_SETTINGS         = 0x18;
    constexpr uintptr_t PN_CONNECT_METHOD          = 0x20;
    constexpr uintptr_t PN_LOG_LEVEL               = 0x24;

    // PhotonNetwork RVA (static methods)
constexpr uintptr_t RVA_PN_GetNickName         = 0x3F51DF0;
constexpr uintptr_t RVA_PN_SetNickName         = 0x3F537E0;
constexpr uintptr_t RVA_PN_GetLocalPlayer      = 0x3F51A00;
constexpr uintptr_t RVA_PN_GetCurrentLobby     = 0x3F511F0;
constexpr uintptr_t RVA_PN_GetCurrentRoom      = 0x3F51260;
constexpr uintptr_t RVA_PN_GetPlayerList       = 0x3F52540;
constexpr uintptr_t RVA_PN_GetPlayerListOthers = 0x3F522B0;
constexpr uintptr_t RVA_PN_Disconnect          = 0x3F3EBB0;
constexpr uintptr_t RVA_PN_ConnectUsingSettings= 0x3F3CA70;
constexpr uintptr_t RVA_PN_JoinRandomRoom      = 0x3F42A20;
constexpr uintptr_t RVA_PN_JoinRandomRoomFiltered = 0x3F42460;
constexpr uintptr_t RVA_PN_JoinRandomOrCreateRoom = 0x3F41E90;
constexpr uintptr_t RVA_PN_CreateRoom          = 0x3F3D330;
constexpr uintptr_t RVA_PN_JoinOrCreateRoom    = 0x3F41910;
constexpr uintptr_t RVA_PN_JoinRoom            = 0x3F42A80;
constexpr uintptr_t RVA_PN_RejoinRoom          = 0x3F4B020;
constexpr uintptr_t RVA_PN_ReconnectAndRejoin  = 0x3F4A5E0;
constexpr uintptr_t RVA_PN_LeaveRoom           = 0x3F42FF0;
constexpr uintptr_t RVA_PN_LeaveLobby          = 0x3F42F40;
constexpr uintptr_t RVA_PN_DestroyPlayerObjs   = 0x3F3E120;
constexpr uintptr_t RVA_PN_DestroyPlayerObjsId = 0x3F3DFA0;
constexpr uintptr_t RVA_PN_RaiseEvent          = 0x3F4A330;
constexpr uintptr_t RVA_PN_IsConnected         = 0x3F51590;
constexpr uintptr_t RVA_PN_IsConnectedAndReady = 0x3F514A0;
constexpr uintptr_t RVA_PN_GetNetworkClientState = 0x3F51C60;
constexpr uintptr_t RVA_PN_GetServer           = 0x3F52BA0;
constexpr uintptr_t RVA_PN_GetAuthValues       = 0x3F50D70;
constexpr uintptr_t RVA_PN_GetOfflineMode      = 0x3F51E50;
constexpr uintptr_t RVA_PN_GetAutomaticallySyncScene = 0x3F50E00;
constexpr uintptr_t RVA_PN_GetInLobby          = 0x3F513F0;
constexpr uintptr_t RVA_PN_GetInRoom           = 0x3F51450;
constexpr uintptr_t RVA_PN_GetKeepAliveInBackground = 0x3F51800;
constexpr uintptr_t RVA_PN_GetIsMasterClient   = 0x3F51680;
constexpr uintptr_t RVA_PN_GetMasterClient     = 0x3F51A90;
constexpr uintptr_t RVA_PN_GetCountOfPlayersOnMaster = 0x3F50FC0;
constexpr uintptr_t RVA_PN_GetCountOfPlayersInRooms = 0x3F50F60;
constexpr uintptr_t RVA_PN_GetCountOfPlayers   = 0x3F51020;
constexpr uintptr_t RVA_PN_GetCountOfRooms     = 0x3F51090;
constexpr uintptr_t RVA_PN_GetNetworkStatisticsEnabled = 0x3F51D80;
constexpr uintptr_t RVA_PN_GetResentReliableCommands = 0x3F527C0;
constexpr uintptr_t RVA_PN_GetCrcCheckEnabled  = 0x3F510F0;
constexpr uintptr_t RVA_PN_GetPacketLossByCrcCheck = 0x3F51EA0;
constexpr uintptr_t RVA_PN_GetMaxResendsBeforeDisconnect = 0x3F51BF0;
constexpr uintptr_t RVA_PN_GetQuickResends     = 0x3F52750;
constexpr uintptr_t RVA_PN_GetUseAlternativeUdpPorts = 0x3F52DA0;
constexpr uintptr_t RVA_PN_NetworkStatisticsToString = 0x3F456A0;
constexpr uintptr_t RVA_PN_GetServerAddress    = 0x3F528F0;
constexpr uintptr_t RVA_PN_GetCloudRegion      = 0x3F50E90;
constexpr uintptr_t RVA_PN_GetCurrentCluster   = 0x3F51160;
constexpr uintptr_t RVA_PN_GetAppVersion       = 0x3F50D10;
constexpr uintptr_t RVA_PN_GetPing             = 0x3F410E0;
constexpr uintptr_t RVA_PN_GetServerTimestamp  = 0x3F52A40;
constexpr uintptr_t RVA_PN_SetPlayerCustomProps = 0x3F4EC00;
constexpr uintptr_t RVA_PN_GetSendRate         = 0x3F52830;
constexpr uintptr_t RVA_PN_SetSendRate         = 0x3F53D40;
constexpr uintptr_t RVA_PN_GetSerializationRate = 0x3F52890;
constexpr uintptr_t RVA_PN_SetSerializationRate = 0x3F53E50;
constexpr uintptr_t RVA_PN_LoadLevelByIndex    = 0x3F43770;
constexpr uintptr_t RVA_PN_LoadLevelByName     = 0x3F439B0;
constexpr uintptr_t RVA_PN_OpRemoveCompleteCacheOfPlayer = 0x3F48D70;
constexpr uintptr_t RVA_PN_OpCleanActorRpcBuffer       = 0x3F48B30;
constexpr uintptr_t RVA_PN_OpRemoveCompleteCache       = 0x3F48E80;
constexpr uintptr_t RVA_PN_DestroyAll                  = 0x3F3DEF0;
constexpr uintptr_t RVA_PN_DestroyAllLocalOnly         = 0x3F3DCC0;  // DestroyAll(bool localOnly)
constexpr uintptr_t RVA_PN_DestroyPlayerObjsLocal      = 0x3F3E320;  // DestroyPlayerObjects(int, bool)
constexpr uintptr_t RVA_PN_RemoveRPCs_Player           = 0x3F4C730;
constexpr uintptr_t RVA_PN_SendAllOutgoingCommands     = 0x3F4D030;
constexpr uintptr_t RVA_PN_Reconnect                   = 0x3F4A8F0;

constexpr uintptr_t RVA_PN_RequestOwnership            = 0x3F4C820;  // internal static void RequestOwnership(int viewID, int fromOwner)
constexpr uintptr_t RVA_PN_TransferOwnership           = 0x3F4F990;  // internal static void TransferOwnership(int viewID, int playerID)
constexpr uintptr_t RVA_PN_OpRemoveFromServerInstantiations = 0x3F48F50; // private static void OpRemoveFromServerInstantiationsOfPlayer(int actorNr)

    // PhotonNetwork static field offsets
    constexpr uintptr_t PN_ENABLE_CLOSE_CONNECTION     = 0x28;  // bool
    constexpr uintptr_t PN_PHOTON_VIEW_LIST            = 0xB0;  // NonAllocDictionary<int, PhotonView>
constexpr uintptr_t RVA_SingleplayerProfile_AddXPAndFunds = 0x11A3670;

    // SaveFileController RVA
constexpr uintptr_t RVA_SaveFileController_CreateBackup = 0x2140970;
constexpr uintptr_t RVA_SaveFileController_ResetSave = 0x2140C40;
constexpr uintptr_t RVA_SaveFileController_SaveGameSettings = 0x2141020;
constexpr uintptr_t RVA_SaveFileController_LoadGameState = 0x2140B50;
constexpr uintptr_t RVA_SaveFileController_SaveCacheFile = 0x2140F50;

    // RewardManager RewardUI RVA
constexpr uintptr_t RVA_RewardManager_RewardUi_SetValue_A = 0x945480;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetValue_B = 0x942170;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetValue_C = 0x94D0F0;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetValue_D = 0x942830;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetValue_E = 0x94D430;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetPair_A = 0x953240;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetPair_B = 0x951A00;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetPair_C = 0x950330;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetPair_D = 0x957F60;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetPair_E = 0x9457C0;
constexpr uintptr_t RVA_RewardManager_RewardUi_SetPair_F = 0x94C340;

    // ChallengesController RVA
constexpr uintptr_t RVA_ChallengesController_GetInstance = 0x1FB39A0;
constexpr uintptr_t RVA_ChallengesController_RequestRefreshManagers = 0x1FA6920;
constexpr uintptr_t RVA_ChallengesController_ChangeChallengeProgression = 0x1FA5AE0;
constexpr uintptr_t RVA_ChallengesController_SetChallengeProgression = 0x1FA6930;
constexpr uintptr_t RVA_ChallengesController_GetChallengeFromType = 0x1FA5DC0;

    // GhostOS RVA
constexpr uintptr_t RVA_GhostOS_OpenShop = 0x860FC0;
constexpr uintptr_t RVA_GhostOS_UpdatePlayerLevelUI = 0x8730D0;
constexpr uintptr_t RVA_GhostOS_UpdateMoneyUI = 0x872F20;

    // TMPro TMP_Text (text getter)
constexpr uintptr_t RVA_TMP_Text_get_text      = 0x49EFA20;

    // InventoryEquipment RVA
constexpr uintptr_t RVA_InventoryEquipment_SaveItem = 0x880EC0;
constexpr uintptr_t RVA_InventoryEquipment_UpdateUpgradeStatus = 0x88B4A0;

    // InventoryManager RVA
constexpr uintptr_t RVA_InventoryManager_SaveInventory = 0x891150;
constexpr uintptr_t RVA_InventoryManager_UpdateInventory = 0x895360;
constexpr uintptr_t RVA_InventoryManager_UpdateUpgradeStatus = 0x895450;

    // StoreEquipment RVA (TypeDefIndex: 976)
constexpr uintptr_t RVA_StoreEquipment_Buy = 0x99E970;
constexpr uintptr_t RVA_StoreEquipment_Sell = 0x99F2F0;
constexpr uintptr_t RVA_StoreEquipment_CanBuy = 0x99EB10;
constexpr uintptr_t RVA_StoreEquipment_CanSell = 0x99EB40;
constexpr uintptr_t RVA_StoreEquipment_GetBuyCost = 0x99EB70;
constexpr uintptr_t RVA_StoreEquipment_GetSellValue = 0x99EBA0;
constexpr uintptr_t RVA_StoreEquipment_UpdateUpgradeStatus = 0x88B4A0;

    // StoreManager RVA (TypeDefIndex: 983)
constexpr uintptr_t RVA_StoreManager_Buy = 0x9B54E0;
constexpr uintptr_t RVA_StoreManager_Sell = 0x9B5AE0;
constexpr uintptr_t RVA_StoreManager_BuyMissingFromLoadout = 0x9B5330;
constexpr uintptr_t RVA_StoreManager_UpdateBuySellValue = 0x9B43D0;
constexpr uintptr_t RVA_StoreManager_UpdateBuySellInteractableState = 0x9B4180;
constexpr uintptr_t RVA_StoreManager_DisplayStoreInfo_Inventory = 0x9B5570;
constexpr uintptr_t RVA_StoreManager_DisplayStoreInfo_StoreEquipment = 0x9B5690;
constexpr uintptr_t RVA_StoreManager_UpdateUpgradeStatus = 0x9C0270;

    // PCPropGrab RVA
constexpr uintptr_t RVA_PCPropGrab_Drop           = 0xB05180;
constexpr uintptr_t RVA_PCPropGrab_DropNetworked  = 0xB050B0;
constexpr uintptr_t RVA_PCPropGrab_DropAllInventory = 0xB04F90;

    // ObjectiveManager RVA
constexpr uintptr_t RVA_ObjectiveManager_CompleteObjective       = 0xAA3EC0;
constexpr uintptr_t RVA_ObjectiveManager_CompleteObjectiveSynced = 0xAA3DB0;
constexpr uintptr_t RVA_ObjectiveManager_SetObjectives           = 0xAA4F90;

    // ═══════════════════════════════════════════════════
    //  LOAD BALANCING CLIENT (via PhotonNetwork.NetworkingClient)
    //  Instance methods — bypass PhotonNetwork static checks
    // ═══════════════════════════════════════════════════
constexpr uintptr_t RVA_LBC_OpRaiseEvent       = 0x3F24D80;  // virtual Slot 10, THE key method
constexpr uintptr_t RVA_LBC_Disconnect         = 0x3F201C0;  // Disconnect(DisconnectCause)
constexpr uintptr_t RVA_LBC_SimulateConnLoss   = 0x3F26810;  // SimulateConnectionLoss(bool)
constexpr uintptr_t RVA_LBC_OpLeaveRoom        = 0x3F24C50;
constexpr uintptr_t RVA_LBC_OpSetActorProps    = 0x3F24FA0;  // OpSetCustomPropertiesOfActor(int, HT, HT, WF)
constexpr uintptr_t RVA_LBC_OpSetRoomProps     = 0x3F25530;  // OpSetPropertiesOfRoom(HT, HT, WF)
constexpr uintptr_t RVA_LBC_OpChangeGroups     = 0x3F23D00;  // OpChangeGroups(byte[], byte[])
// LoadBalancingClient NickName get/set
constexpr uintptr_t RVA_LBC_GetNickName        = 0x3F27870;
constexpr uintptr_t RVA_LBC_SetNickName        = 0x3F27B40;
    // LBC field offsets (from object start = 0x10 header + field offset)
    constexpr uintptr_t LBC_PEER               = 0x10;   // LoadBalancingPeer (field 0x00)

    // ═══════════════════════════════════════════════════
    //  LOAD BALANCING PEER (lowest level before PhotonPeer)
    //  Bypasses ALL PUN validation — can send any event code
    // ═══════════════════════════════════════════════════
constexpr uintptr_t RVA_LBP_OpRaiseEvent       = 0x3F29E20;  // virtual, raw event send
constexpr uintptr_t RVA_LBP_OpSetActorProps    = 0x3F2A1C0;  // raw actor property set

    // ═══════════════════════════════════════════════════
    //  PHOTON PEER (transport level)
    // ═══════════════════════════════════════════════════
constexpr uintptr_t RVA_PP_SendOutgoingCommands = 0x3EE47E0;  // flush to network
constexpr uintptr_t RVA_PP_Disconnect          = 0x3EE30D0;   // transport disconnect

    // PhotonNetwork internal (bypasses eventCode >= 200 check)
constexpr uintptr_t RVA_PN_RaiseEventInternal  = 0x3F4A180;

    // ═══════════════════════════════════════════════════
    //  PHOTON VIEW (Photon.Pun.PhotonView)
    // ═══════════════════════════════════════════════════
constexpr uintptr_t RVA_PV_RPC                 = 0x3F56310;
constexpr uintptr_t RVA_PV_RPC_Player          = 0x3F56280;
constexpr uintptr_t RVA_PV_RpcSecure           = 0x3F56810;
constexpr uintptr_t RVA_PV_RpcSecure_Player    = 0x3F568A0;
constexpr uintptr_t RVA_PV_TransferOwnership   = 0x3F56B50;
constexpr uintptr_t RVA_PV_TransferOwnershipId = 0x3F567A0;
constexpr uintptr_t RVA_PV_GetOwnerActorNr     = 0x25C4410;
constexpr uintptr_t RVA_PV_GetControllerActorNr = 0x1A07FC0;
constexpr uintptr_t RVA_PV_get_IsMine          = 0x488420;
constexpr uintptr_t RVA_PV_get_Owner           = 0xCE9A70;

    // ═══════════════════════════════════════════════════
    //  PHOTON REALTIME PLAYER (Photon.Realtime.Player)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t PP_ACTOR_NUMBER            = 0x18;   // int
    constexpr uintptr_t PP_NICKNAME                = 0x20;   // string
    constexpr uintptr_t PP_CUSTOM_PROPERTIES       = 0x38;   // Hashtable

constexpr uintptr_t RVA_PP_GetNickName         = 0x3F2DC80;
constexpr uintptr_t RVA_PP_SetNickName         = 0x3F2DD20;
constexpr uintptr_t RVA_PP_GetUserId           = 0x647990;
constexpr uintptr_t RVA_PP_GetActorNumber      = 0x12F0A70;
constexpr uintptr_t RVA_PP_GetIsMasterClient   = 0x3F2DC60;
constexpr uintptr_t RVA_PP_SetCustomProperties  = 0x3F2D550;

    // ═══════════════════════════════════════════════════
    //  PHOTON ROOM (Photon.Realtime.Room)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t PR_PLAYERS_DICT            = 0x70;   // Dictionary<int, Player>
    constexpr uintptr_t PR_MAX_PLAYERS             = 0x20;   // byte
    constexpr uintptr_t PR_IS_OPEN                 = 0x38;   // bool

constexpr uintptr_t RVA_PR_GetPlayers          = 0x4884E0;
constexpr uintptr_t RVA_PR_RemovePlayer        = 0x3F31DA0;
constexpr uintptr_t RVA_PR_RemovePlayerId      = 0x3F31D60;
constexpr uintptr_t RVA_PR_GetPlayer           = 0x3F31C20;
constexpr uintptr_t RVA_PR_SetMaxPlayers       = 0x3F32C80;
constexpr uintptr_t RVA_PR_SetIsOpen           = 0x3F32AC0;
constexpr uintptr_t RVA_PR_GetMasterClientId   = 0x133F0C0;
constexpr uintptr_t RVA_PR_GetName             = 0x473C50;
constexpr uintptr_t RVA_PR_GetPlayerCount      = 0x3F329A0;
constexpr uintptr_t RVA_PR_GetIsOpen           = 0x647980;
constexpr uintptr_t RVA_PR_GetIsVisible        = 0x25F6F50;
constexpr uintptr_t RVA_PR_GetMaxPlayers       = 0x1335330;

    // ═══════════════════════════════════════════════════
    //  PHOTON EVENT CODES
    // ═══════════════════════════════════════════════════
    constexpr uint8_t EVT_JOIN                     = 255;
    constexpr uint8_t EVT_LEAVE                    = 254;
    constexpr uint8_t EVT_PROPERTIES_CHANGED       = 253;
    constexpr uint8_t EVT_ERROR_INFO               = 251;
    constexpr uint8_t EVT_CACHE_SLICE_CHANGED      = 250;

    // ═══════════════════════════════════════════════════
    //  RPC TARGET ENUM (Photon.Pun.RpcTarget)
    // ═══════════════════════════════════════════════════
    constexpr int RPC_ALL                          = 0;
    constexpr int RPC_OTHERS                       = 1;
    constexpr int RPC_MASTER_CLIENT                = 2;
    constexpr int RPC_ALL_BUFFERED                 = 3;
    constexpr int RPC_OTHERS_BUFFERED              = 4;
    constexpr int RPC_ALL_VIA_SERVER               = 5;

    // ═══════════════════════════════════════════════════
    //  UNITY TRANSFORM OFFSETS (internal)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t TF_POSITION                = 0x90;
    constexpr uintptr_t TF_ROTATION                = 0xA0;
    constexpr uintptr_t TF_SCALE                   = 0xB0;

    // ═══════════════════════════════════════════════════
    //  IL2CPP LIST<T> OFFSETS (System.Collections.Generic)
    // ═══════════════════════════════════════════════════
    constexpr uintptr_t LIST_ITEMS                 = 0x10;  // T[] _items
    constexpr uintptr_t LIST_SIZE                  = 0x18;  // int _size
    constexpr uintptr_t ARRAY_FIRST_ELEMENT        = 0x20;  // first element in Il2Cpp array
    constexpr uintptr_t ARRAY_LENGTH               = 0x18;  // array length

} // namespace Phasmo

#pragma once
#include <cstdint>

// ══════════════════════════════════════════════════════════
//  PHASMO_STRUCTS.H — Phasmophobia
//  Structures du jeu extraites du dump Il2Cpp
// ══════════════════════════════════════════════════════════

namespace Phasmo
{
    // ─── Unity Math Types ───────────────────────────────
    struct Vec3
    {
        float x, y, z;
    };

    struct Bounds
    {
        Vec3 center;
        Vec3 extents;
    };

    struct Quat
    {
        float x, y, z, w;
    };

    struct Color
    {
        float r, g, b, a;
    };

    struct NullableInt32
    {
        bool hasValue;                // 0x0
        uint8_t _pad[0x03];           // 0x1-0x3
        int32_t value;                // 0x4
    };

    // ─── Forward Declarations ───────────────────────────
    struct Transform;
    struct PhotonView;
    struct MapController;
    struct NavMeshAgent;
    struct Difficulty;
    struct PCPropGrab;
    struct ObjectiveManager;
    struct PlayerCharacter;
    struct LevelRoom;
    struct GhostOS;
    struct SingleplayerProfile;
    struct GhostAudio;
    struct EMF;
    struct EMFData;

    // ─── Il2Cpp Internal Types ──────────────────────────
    struct Il2CppObject
    {
        void* klass;    // 0x00
        void* monitor;  // 0x08
    };

    struct Il2CppArray : Il2CppObject
    {
        void* bounds;       // 0x10
        int32_t max_length; // 0x18
        // elements start at 0x20
    };

    template<typename T>
    struct Il2CppList : Il2CppObject
    {
        void* _items;    // 0x10 - T[] array
        int32_t _size;   // 0x18
        int32_t _version;// 0x1C

        int32_t Count() { return _size; }

        T Get(int idx)
        {
            if (idx < 0 || idx >= _size) return T{};
            // _items is an Il2CppArray, elements start at offset 0x20
            return *reinterpret_cast<T*>(
                reinterpret_cast<uint8_t*>(_items) + 0x20 + idx * sizeof(T));
        }
    };

    // ─── MonoBehaviour Base ─────────────────────────────
    struct MonoBehaviour : Il2CppObject
    {
        // Unity internals between 0x10 - 0x20
        uint8_t _pad[0x10]; // 0x10-0x1F
    };

    struct MonoBehaviourPun : MonoBehaviour
    {
        void* pvCache; // 0x20
    };

    struct PhotonView : MonoBehaviour
    {
        // PhotonView fields are accessed via Il2Cpp methods.
    };

    struct MapController : MonoBehaviour
    {
        void* playerIcons;         // 0x20 - List<RectTransform>
        void* players;             // 0x28 - List<Player>
        void* allFloors;           // 0x30 - MapController.Floor[]
        int32_t currentFloorIndex; // 0x38
        float iconScale;           // 0x3C
        float playerIconScale;     // 0x40
        void* motionSensorData;    // 0x48
        void* gameController;      // 0x50
        int32_t defaultFloor;      // 0x58 - LevelRoom.Floor enum
        Vec3 playerIconOffset;     // 0x5C
        void* mapCamera;           // 0x68
    };

    // ══════════════════════════════════════════════════════
    //  PHASMO ROLES UPDATED - PlayerRoleType Enum
    //  Mis à jour depuis Il2CppDumper v6.7.46 (01/04/2026)
    // ══════════════════════════════════════════════════════
    enum class PlayerRoleType : int32_t
    {
        None = 0,
        Developer = 1,
        Discord = 2,
        Creator = 3,
        Translator = 4,
        Competition = 5,
        Artist = 6,
        Holiday22 = 7,
        Easter23 = 8,
        Halloween23 = 9,
        Holiday23 = 10,
        Easter24 = 11,
        Halloween24 = 12,
        Holiday24 = 13,
        Easter25 = 14,
        Halloween25 = 15,
        Holiday25 = 16,
        ApocalypseBronze = 17,
        ApocalypseSilver = 18,
        ApocalypseGold = 19,
        LighthouseKeeper = 20,
        LighthouseFerryman = 21,
        Ranger = 22,
        Inmate = 23,
        Halloween24Foil = 24,
        Holiday24Foil = 25,
        Easter24Foil = 26,
        AchievementHunter = 27,
        Halloween25Foil = 28,
        Holiday25Foil = 29,
        Easter25Foil = 30,
        TwitchDropChronicle = 31,
        TwitchDropChronicleFoil = 32,
        AmericanHeart = 33,
        AmericanHeartFoil = 34,
        FarmhouseFieldwork = 35,
        TwitchDropGrafton = 36,
        TwitchDropGraftonFoil = 37,
        Anniversary25 = 38,
        TwitchCon25 = 39,
        Anniversary25Foil = 40,
        TwitchCon25Foil = 41,
        DinerGhostInTheMachine = 42,
        NellsDiner = 43,
        TwitchDropNellsDiner = 44,
        TwitchDropNellsDinerFoil = 45,
        Moneybags = 46,
        TwitchDropTanglewood = 47,
        TwitchDropTanglewoodFoil = 48,
        Tanglewood = 49,
        TwitchDropGalaxies = 50,
        TwitchDropGalaxiesFoil = 51,
        Easter26 = 52,
        Easter26Foil = 53
    };

    inline const char* GetRoleName(PlayerRoleType role)
    {
        switch (role)
        {
        case PlayerRoleType::None: return "None";
        case PlayerRoleType::Developer: return "Developer";
        case PlayerRoleType::Discord: return "Discord";
        case PlayerRoleType::Creator: return "Creator";
        case PlayerRoleType::Translator: return "Translator";
        case PlayerRoleType::Competition: return "Competition";
        case PlayerRoleType::Artist: return "Artist";
        case PlayerRoleType::Holiday22: return "Holiday 22";
        case PlayerRoleType::Easter23: return "Easter 23";
        case PlayerRoleType::Halloween23: return "Halloween 23";
        case PlayerRoleType::Holiday23: return "Holiday 23";
        case PlayerRoleType::Easter24: return "Easter 24";
        case PlayerRoleType::Halloween24: return "Halloween 24";
        case PlayerRoleType::Holiday24: return "Holiday 24";
        case PlayerRoleType::Easter25: return "Easter 25";
        case PlayerRoleType::Halloween25: return "Halloween 25";
        case PlayerRoleType::Holiday25: return "Holiday 25";
        case PlayerRoleType::ApocalypseBronze: return "Apocalypse Bronze";
        case PlayerRoleType::ApocalypseSilver: return "Apocalypse Silver";
        case PlayerRoleType::ApocalypseGold: return "Apocalypse Gold";
        case PlayerRoleType::LighthouseKeeper: return "Lighthouse Keeper";
        case PlayerRoleType::LighthouseFerryman: return "Lighthouse Ferryman";
        case PlayerRoleType::Ranger: return "Ranger";
        case PlayerRoleType::Inmate: return "Inmate";
        case PlayerRoleType::Halloween24Foil: return "Halloween 24 Foil";
        case PlayerRoleType::Holiday24Foil: return "Holiday 24 Foil";
        case PlayerRoleType::Easter24Foil: return "Easter 24 Foil";
        case PlayerRoleType::AchievementHunter: return "Achievement Hunter";
        case PlayerRoleType::Halloween25Foil: return "Halloween 25 Foil";
        case PlayerRoleType::Holiday25Foil: return "Holiday 25 Foil";
        case PlayerRoleType::Easter25Foil: return "Easter 25 Foil";
        case PlayerRoleType::TwitchDropChronicle: return "Twitch Drop Chronicle";
        case PlayerRoleType::TwitchDropChronicleFoil: return "Twitch Drop Chronicle Foil";
        case PlayerRoleType::AmericanHeart: return "American Heart";
        case PlayerRoleType::AmericanHeartFoil: return "American Heart Foil";
        case PlayerRoleType::FarmhouseFieldwork: return "Farmhouse Fieldwork";
        case PlayerRoleType::TwitchDropGrafton: return "Twitch Drop Grafton";
        case PlayerRoleType::TwitchDropGraftonFoil: return "Twitch Drop Grafton Foil";
        case PlayerRoleType::Anniversary25: return "Anniversary 25";
        case PlayerRoleType::TwitchCon25: return "TwitchCon 25";
        case PlayerRoleType::Anniversary25Foil: return "Anniversary 25 Foil";
        case PlayerRoleType::TwitchCon25Foil: return "TwitchCon 25 Foil";
        case PlayerRoleType::DinerGhostInTheMachine: return "Ghost In The Machine";
        case PlayerRoleType::NellsDiner: return "Nell's Diner";
        case PlayerRoleType::TwitchDropNellsDiner: return "Twitch Drop Nell's Diner";
        case PlayerRoleType::TwitchDropNellsDinerFoil: return "Twitch Drop Nell's Diner Foil";
        case PlayerRoleType::Moneybags: return "Moneybags";
        case PlayerRoleType::TwitchDropTanglewood: return "Twitch Drop Tanglewood";
        case PlayerRoleType::TwitchDropTanglewoodFoil: return "Twitch Drop Tanglewood Foil";
        case PlayerRoleType::Tanglewood: return "Tanglewood";
        case PlayerRoleType::TwitchDropGalaxies: return "Twitch Drop Galaxies";
        case PlayerRoleType::TwitchDropGalaxiesFoil: return "Twitch Drop Galaxies Foil";
        case PlayerRoleType::Easter26: return "Easter 26";
        case PlayerRoleType::Easter26Foil: return "Easter 26 Foil";
        default: return "Unknown";
        }
    }

    struct NetworkPlayerSpot : Il2CppObject
    {
        bool playerReady;               // 0x10
        uint8_t _pad0[0x04];            // 0x11-0x14
        void* photonPlayer;             // 0x18
        void* unityPlayerId;            // 0x20
        int32_t experience;             // 0x28
        int32_t level;                  // 0x2C
        int32_t prestige;               // 0x30
        uint8_t _pad1[0x04];            // 0x34
        void* player;                   // 0x38
        float playerVolume;             // 0x40
        void* accountName;              // 0x48
        bool isKicked;                  // 0x50
        bool isHacker;                  // 0x51
        bool isBlocked;                 // 0x52
        uint8_t _pad2[0x05];            // 0x53-0x57
        void* roleBadges;               // 0x58
        PlayerRoleType role;            // 0x60
        int32_t prestigeIndex;          // 0x64
        bool prestigeTheme;             // 0x68
        uint8_t _pad3[0x07];            // 0x69-0x6F
        void* votedContract;            // 0x70
        int32_t platformType;           // 0x78
        bool hasReceivedPlayerInformation; // 0x7C
        bool playerIsBlocked;           // 0x7D
    };

    struct Network : MonoBehaviour
    {
        uint8_t _pad1[0x08];
        void* localPlayer;              // 0x28
        void* playerSpots;              // 0x30 - List<Network.PlayerSpot>
    };

    // ─── Player (TypeDefIndex: 1064) ────────────────────
    struct Player : MonoBehaviour
    {
        PhotonView* photonView;        // 0x20
        bool isDead;                   // 0x28
        bool flagA;                    // 0x29
        bool flagB;                    // 0x2A
        uint8_t _pad2[0x01];           // 0x2B
        int32_t modelId;               // 0x2C
        void* playerCharacter;         // 0x30
        void* closetZone;              // 0x38
        void* headObject;              // 0x40
        void* field48;                 // 0x48
        void* keyInfoList;             // 0x50
        void* camera;                  // 0x58
        void* levelRoom;               // 0x60
        void* mapIcon;                 // 0x68
        void* photonObjectInteractA;   // 0x70
        void* photonObjectInteractB;   // 0x78
        void* field80;                 // 0x80
        void* field88;                 // 0x88
        int32_t layerMask;             // 0x90
        uint8_t _pad3[0x04];           // 0x94
        void* field98;                 // 0x98
        void* fieldA0;                 // 0xA0
        void* fieldA8;                 // 0xA8
        void* fieldB0;                 // 0xB0
        void* deadPlayer;              // 0xB8
        void* playerSanity;            // 0xC0
        void* playerStats;             // 0xC8
        void* footstepController;      // 0xD0
        void* journalController;       // 0xD8
        void* renderers;               // 0xE0
        bool fieldE8;                  // 0xE8
        uint8_t _pad4[0x07];           // 0xE9-0xEF
        void* playerAudio;             // 0xF0
        void* playerGraphics;          // 0xF8
        void* playerSensors;           // 0x100
        void* playerStamina;           // 0x108
        float field110;                // 0x110
        bool field114;                 // 0x114
        uint8_t _pad5[0x03];           // 0x115-0x117
        void* physicsCharController;   // 0x118
        void* audioListener;           // 0x120
        void* firstPersonController;   // 0x128
        void* pcPropGrab;              // 0x130
        void* dragRigidbodyUse;        // 0x138
        void* pcCanvas;                // 0x140
        void* pcCrouch;                // 0x148
        void* pcMenu;                  // 0x150
        void* pcControls;              // 0x158
        void* pcFlashlight;            // 0x160
    };

    struct PlayerAudio : MonoBehaviour
    {
        Player* player;                // 0x20
        void* walkieTalkie;            // 0x28
        void* voiceVolume;             // 0x30
        void* voiceSourceA;            // 0x38
        void* voiceSourceB;            // 0x40
        void* voiceSourceC;            // 0x48
        void* voiceSourceD;            // 0x50
        void* voiceSourceE;            // 0x58
        void* voiceSourceF;            // 0x60
        void* voiceOcclusion;          // 0x68
    };

    struct WalkieTalkie : MonoBehaviour
    {
        void* source;                  // 0x20
        Player* player;                // 0x28
        void* staticSource;            // 0x30
        void* staticEffect;            // 0x38
        PhotonView* view;              // 0x40
        bool field48;                  // 0x48
        bool field49;                  // 0x49
        uint8_t _pad1[0x06];           // 0x4A-0x4F
        void* photonInteract;          // 0x50
        void* col;                     // 0x58
    };

    struct Gun : MonoBehaviour
    {
        uint8_t _pad1[0x1C0];          // 0x20-0x1DF
        Il2CppArray* allRenderers;     // 0x1E0 - MeshRenderer[]
    };

    struct Equipment : MonoBehaviour
    {
        uint8_t _pad1[0xB0];           // 0x20-0xCF
        Il2CppArray* renderers;        // 0xD0 - Renderer[]
    };

    struct PCDisablePlayerComponents : MonoBehaviour
    {
        uint8_t _pad1[0x10];           // 0x20-0x2F
        void* leftHandRend;            // 0x30
        void* rightHandRend;           // 0x38
        Il2CppArray* handMaterials;    // 0x40 - Material[]
    };

    struct TruckRadioController : MonoBehaviour
    {
        PhotonView* view;              // 0x20
        void* source;                  // 0x28
        bool flag30;                   // 0x30
        uint8_t _pad1[0x07];           // 0x31-0x37
        Il2CppArray* introductionClips;    // 0x38
        Il2CppArray* keyWarningClips;      // 0x40
        Il2CppArray* noHintClips;          // 0x48
        Il2CppArray* aggressiveHintClips;  // 0x50
        Il2CppArray* friendlyHintClips;    // 0x58
        Il2CppArray* nonFriendlyHintClips; // 0x60
        Il2CppArray* cursedPossessionsClips; // 0x68
    };

    struct CCTVTruckTrigger : MonoBehaviour
    {
        bool inTruck;                  // 0x20
    };

    // --- PlayerCharacter (TypeDefIndex: 1067) ---
    struct PlayerCharacter : MonoBehaviour
    {
        uint8_t _pad1[0x08];           // 0x20-0x27
        bool flagA;                    // 0x28
        uint8_t _pad2[0x07];           // 0x29-0x2F
        void* meshRenderer;            // 0x30
        void* gameObject;              // 0x38
        Transform* transform;          // 0x40
    };
    // ─── PlayerSanity (TypeDefIndex: 1076) ──────────────
    struct PlayerSanity : MonoBehaviour
    {
        PhotonView* photonView;        // 0x20
        void* player;                  // 0x28
        float sanity;                  // 0x30 - current sanity (0.0 - 100.0)
        float sanityInternalA;         // 0x34
        float sanityInternalB;         // 0x38
        uint8_t _pad2[0x04];           // 0x3C
        void* renderTexture;           // 0x40
        uint8_t _pad3[0x18];           // 0x48
        void* candleList;              // 0x60
    };

    // ─── PlayerStamina (TypeDefIndex: 1082) ─────────────
    struct PlayerStamina : MonoBehaviour
    {
        void* audioSource;             // 0x20
        void* outOfBreathMale;         // 0x28
        void* outOfBreathFemale;       // 0x30
        void* player;                  // 0x38
        bool maleSoundsOverride;       // 0x40
        bool femaleSoundsOverride;     // 0x41
        bool staminaFlags[6];          // 0x42-0x47
        bool staminaFlagsB[4];         // 0x48-0x4B
        float staminaValue;            // 0x4C
        float staminaRecover;          // 0x50
        float staminaDrain;            // 0x54
        void* staminaEventA;           // 0x58
        void* staminaEventB;           // 0x60
    };

    // --- PCPropGrab (TypeDefIndex: 1036) ---
    struct PCPropGrab : MonoBehaviour
    {
        uint8_t _pad1[0x08];           // 0x20-0x27
        void* player;                  // 0x28
        void* pcCanvas;                // 0x30
        void* pcFlashlight;            // 0x38
        void* propDropPoint;           // 0x40
        void* interactList;            // 0x48 - List<PhotonObjectInteract>
        int32_t currentIndex;          // 0x50
        uint8_t _pad2[0x04];           // 0x54
        void* playerCam;               // 0x58
        int32_t mask;                  // 0x60 (LayerMask)
        uint8_t _pad3[0x04];           // 0x64
        void* fixedJoint;              // 0x68
        void* heldTransform;           // 0x70
        bool isHolding;                // 0x78
        uint8_t _pad4[0x07];           // 0x79-0x7F
    };

    // --- PCFlashlight (TypeDefIndex: 1029) ---
    struct PCFlashlight : MonoBehaviour
    {
        uint8_t _pad1[0x08];           // 0x20-0x27
        void* headLight;               // 0x28 (UnityEngine.Light)
        void* pcPropGrab;              // 0x30
        void* player;                  // 0x38
        void* source;                  // 0x40 (AudioSource)
        bool enabled;                  // 0x48
        uint8_t _pad2[0x07];           // 0x49-0x4F
    };

    // ─── Ghost Type (TypeDefIndex: 378) ────────────────
    enum class GhostType : int32_t
    {
        Spirit = 0,
        Wraith = 1,
        Phantom = 2,
        Poltergeist = 3,
        Banshee = 4,
        Jinn = 5,
        Mare = 6,
        Revenant = 7,
        Shade = 8,
        Demon = 9,
        Yurei = 10,
        Oni = 11,
        Yokai = 12,
        Hantu = 13,
        Goryo = 14,
        Myling = 15,
        Onryo = 16,
        TheTwins = 17,
        Raiju = 18,
        Obake = 19,
        Mimic = 20,
        Moroi = 21,
        Deogen = 22,
        Thaye = 23,
        None = 24,
        Gallu = 25,
        Dayan = 26,
        Obambo = 27
    };

    // ─── Ghost Type Info (TypeDefIndex: 379) ───────────
    struct GhostTypeInfo
    {
        GhostType primaryType;         // 0x00
        GhostType secondaryType;       // 0x04
        void* evidencePrimary;         // 0x08 - List<EvidenceType>
        void* evidenceSecondary;       // 0x10 - List<EvidenceType>
        int32_t specialIndex;          // 0x18
        bool isHidden;                 // 0x1C
        void* ghostName;               // 0x20 - Il2CppString*
        int32_t ghostIndex;            // 0x28
        int32_t ghostReward;           // 0x2C
        bool isNew;                    // 0x30
        int32_t abilityType;           // 0x34
        int32_t evidenceCount;         // 0x38
        bool hasBehavior;              // 0x3C
    };

    // ─── GhostInfo (TypeDefIndex: 246) ────────────────
    struct GhostInfo : MonoBehaviour
    {
        uint8_t _pad1[0x08];           // 0x20-0x27
        GhostTypeInfo ghostTypeInfo;   // 0x28
        void* ghost;                   // 0x68 - GhostAI*
        void* levelRoom;               // 0x70
        float ghostSpeed;              // 0x78
        bool flag;                     // 0x7C
    };

    // ─── GhostAI (TypeDefIndex: 241) ────────────────────
    struct GhostAI : MonoBehaviour
    {
        uint8_t _pad1[0x10];           // 0x20-0x2F
        int32_t state;                 // 0x30
        uint8_t _pad1b[0x04];          // 0x34-0x37
        GhostInfo* ghostInfo;          // 0x38
        NavMeshAgent* navMeshAgent;    // 0x40
        GhostAudio* ghostAudio;        // 0x48
        void* ghostInteraction;        // 0x50
        void* ghostActivity;           // 0x58
        void* ghostModel;              // 0x60
        uint8_t _pad2[0x40];           // 0x68-0xA7
        void* losSensor;               // 0xA8
        uint8_t _pad3[0x50];           // 0xB0-0xFF
        void* whiteSage;               // 0x100
        uint8_t _pad4[0x10];           // 0x108-0x117
        void* targetPlayer;            // 0x118
        int32_t targetPlayerId;        // 0x120
        Vec3 lastKnownPlayerPos;       // 0x124
    };

    // --- GhostController (TypeDefIndex: 374) ---
    struct GhostController : MonoBehaviour
    {
        PhotonView* photonView;        // 0x20
    };

    // ─── GhostAudio (TypeDefIndex: 243) ───────────────────
    struct GhostAudio : MonoBehaviour
    {
        PhotonView* photonView;        // 0x20
        GhostAI* ghostAI;              // 0x28
        void* appearSource;            // 0x30 (AudioSource)
        void* sfxNoise;                // 0x38 (Noise)
        void* ghostNoise;              // 0x40 (Noise)
        void* screamSoundClips;        // 0x48 (AudioClip[])
        void* screamClip;              // 0x50 (AudioClip)
    };

    // ─── GhostActivity (TypeDefIndex: 231) ──────────────
    struct GhostActivity : MonoBehaviour
    {
        void* rangeSensorA;            // 0x20
        void* ghostAI;                 // 0x28
        void* rangeSensorB;            // 0x30
        uint8_t _pad2[0x08];           // 0x38
        void* levelController;         // 0x40
    };

    // ─── GameController (TypeDefIndex: 372) ─────────────
    //  Static singleton at static_fields + 0x0
    struct GameController : MonoBehaviour
    {
        uint8_t _pad1[0xC8];           // 0x20-0xE7
        void* levelController;         // 0xE8
        void* multiplayerController;   // 0xF0
        bool flag1;                    // 0xF8
        bool flag2;                    // 0xF9
        uint8_t _pad2[0x06];           // 0xFA-0xFF
        void* material;                // 0x100
        uint8_t _pad3[0x08];           // 0x108
        void* speechRecognizer;        // 0x110
    };

    // ─── LevelController (TypeDefIndex: 392) ────────────
    //  Static singleton at static_fields + 0x0
    struct LevelController : MonoBehaviour
    {
        uint8_t _pad1[0x20];           // 0x20-0x3F
        GhostAI* ghostAI;             // 0x40
        void* doors;                   // 0x48 - Door[]
        void* levelRoomsA;             // 0x50
        void* levelRoomsB;             // 0x58
        uint8_t _pad2[0x20];           // 0x60-0x7F
        void* fuseBox;                 // 0x80
        void* gameController;          // 0x88
        uint8_t _pad3[0x08];           // 0x90
        void* itemSpawner;             // 0x98
        uint8_t _pad4[0x18];           // 0xA0-0xB7
        void* fireSources;             // 0xB8 - List<FireSource>
        void* photonInteracts;         // 0xC0 - List<PhotonObjectInteract>
        uint8_t _pad5[0x18];           // 0xC8-0xDF
        void* key;                     // 0xE0
    };

    // --- LevelRoom (TypeDefIndex: 825) ---
    struct LevelRoom : MonoBehaviour
    {
        uint8_t _pad1[0x40];           // 0x20-0x5F
        void* roomName;                // 0x60 - string
    };

    // ─── EvidenceController (TypeDefIndex: 354) ─────────
    //  Static singleton at static_fields + 0x0
    struct EvidenceController : MonoBehaviour
    {
        void* evidenceList;            // 0x20 - List<Evidence>
        void* levelRooms;              // 0x28
        PhotonView* photonView;        // 0x30
        void* dnaEvidence;             // 0x38
        Transform* ghostOrbTransform;  // 0x40
        void* particleRenderer;        // 0x48
        void* levelController;         // 0x50
        uint8_t _pad2[0x08];           // 0x58
        void* fingerprints;            // 0x60 - List<Fingerprint>
    };

    // ─── CursedItemsController (TypeDefIndex: 347) ──────
    //  Static singleton at static_fields + 0x0
    struct CursedItemsController : MonoBehaviour
    {
        void* ouijaBoard;              // 0x20
        void* musicBox;                // 0x28
        void* tarotCards;              // 0x30
        void* summoningCircle;         // 0x38
        void* hauntedMirror;           // 0x40
        void* voodooDoll;              // 0x48
        void* monkeyPaw;               // 0x50
        uint8_t _pad2[0x38];           // 0x58-0x8F
        void* cursedItemTypes;         // 0x90 - List<CursedItemType>
    };

    // ─── Door (TypeDefIndex: 613) ───────────────────────
    struct Door : MonoBehaviour
    {
        bool boolFlags[3];             // 0x20-0x22
        uint8_t _pad2[0x45];           // 0x23-0x67
        void* audioSource;             // 0x68
        void* occlusionPortal;         // 0x70
        void* volume;                  // 0x78
        uint8_t _pad3[0x08];           // 0x80
        PhotonView* photonView;        // 0x88
        void* rigidbody;               // 0x90
        void* photonInteract;          // 0x98
        void* fingerprint;             // 0xA0
        void* collider;                // 0xA8
        uint8_t _pad4[0x10];           // 0xB0
        void* configurableJoint;       // 0xC0
        uint8_t _pad5[0x0C];           // 0xC8
        int32_t doorStateA;            // 0xD4
        uint8_t _pad6[0x04];           // 0xD8
        int32_t doorStateB;            // 0xDC
    };

    struct PhotonObjectInteract : MonoBehaviour
    {
        PhotonView* photonView;        // 0x20
        void* photonTransformView;     // 0x28
        void* vrGrabbable;             // 0x30
        void* rigidbody;               // 0x38
        bool isProp;                   // 0x40
        bool isItem;                   // 0x41
        bool field42;                  // 0x42
        bool isDroppable;              // 0x43
        bool isUsable;                 // 0x44
        bool field45;                  // 0x45
        bool field46;                  // 0x46
        bool field47;                  // 0x47
        bool field48;                  // 0x48
        uint8_t _pad1[0x07];           // 0x49-0x4F
        void* drawer;                  // 0x50
        Door* door;                    // 0x58
        bool field60;                  // 0x60
        uint8_t _pad2[0x07];           // 0x61-0x67
        void* colliderArray1;          // 0x68
        void* colliderArray2;          // 0x70
        void* colliderArray3;          // 0x78
        float throwMultiplier;         // 0x80
        uint8_t _pad3[0x04];           // 0x84-0x87
        void* transform;               // 0x88
    };

    // ─── FuseBox (TypeDefIndex: 620) ────────────────────
    struct FuseBox : MonoBehaviour
    {
        uint8_t _pad1[0x08];
        int32_t fuseBoxType;           // 0x28
        uint8_t _pad2[0x04];           // 0x2C
        void* renderers;               // 0x30
        void* lights;                  // 0x38
        uint8_t _pad3[0x28];           // 0x40-0x67
        void* audioSource;             // 0x68
        uint8_t _pad4[0x20];           // 0x70-0x8F
        bool isOn;                     // 0x90
        uint8_t _pad5[0x07];           // 0x91-0x97
        void* lightSwitches;           // 0x98
        void* photonInteract;          // 0xA0
    };

    // ─── EMF (TypeDefIndex: 222) ────────────────────────
    struct EMF : MonoBehaviour
    {
        void* evidenceComponent;       // 0x20
        int32_t emfValue;              // 0x28
        int32_t emfAux;                // 0x2C
        int32_t emfType;               // 0x30 (EMF.Type enum)
    };

    // ─── EMFData (TypeDefIndex: 1428) ───────────────────
    //  Static singleton at static_fields + 0x0
    struct EMFData : MonoBehaviour
    {
        uint8_t _pad1[0x18];           // 0x20-0x37
        Il2CppList<EMF*>* emfSources;  // 0x38
    };

    // ─── EMFReader (TypeDefIndex: 691) ────────────────────────
    struct EMFReader : MonoBehaviour
    {
        void* evidence;                // 0x20
        int32_t level;                 // 0x28  (EMF level 1-5)
        uint8_t _pad2[0x04];           // 0x2C
        int32_t type;                  // 0x30  (EMF.Type enum)
    };

    // ─── MediaValues (ScriptableObject) ────────────────
    struct MediaValues : Il2CppObject
    {
        uint8_t _pad1[0x08];           // 0x10-0x17
        int32_t evidenceType;          // 0x18 (MV_EVIDENCE_TYPE)
    };

    // ─── Evidence (TypeDefIndex: 513) ───────────────────
    struct Evidence : MonoBehaviour
    {
        uint8_t _pad1[0x08];
        void* mediaValues;             // 0x28
        bool flagA;                    // 0x30
        bool flagB;                    // 0x31
        uint8_t _pad2[0x06];           // 0x32-0x37
        void* onMediaTaken;            // 0x38
    };

    // ─── Difficulty (TypeDefIndex: 205) ─────────────────
    struct Difficulty : Il2CppObject
    {
        uint8_t _pad0[0x08];
        int32_t difficulty;            // 0x18 (enum)
        uint8_t _pad1[0x04];           // 0x1C
        void* nameKey;                 // 0x20
        void* descriptionKey;          // 0x28
        int32_t requiredLevel;         // 0x30
        float sanityPillRestore;       // 0x34
        float startingSanity;          // 0x38
        float sanityDrain;             // 0x3C
        float sprinting;               // 0x40
        float flashlights;             // 0x44
        float playerSpeed;             // 0x48
        int32_t evidenceGiven;         // 0x4C
        uint8_t _pad2[0x28];           // 0x50-0x77
        float ghostSpeed;              // 0x78
        float setupTime;               // 0x7C
        int32_t weather;               // 0x80
        int32_t doors;                 // 0x84
        int32_t hidingPlaces;          // 0x88
        int32_t monitors;              // 0x8C
        bool fuseBoxA;                 // 0x8E (note: not aligned)
        uint8_t _pad3[0x01];           // 0x8F
        int32_t fuseBoxB;              // 0x90
        int32_t cursedPossQty;         // 0x94
        uint8_t _pad4[0x08];           // 0x98-0x9F
        float overrideMultiplier;      // 0xA0
        int32_t actualWeather;         // 0xA4
    };

    struct LevelValues : MonoBehaviour
    {
        bool stayInServerRoom;         // 0x20
        bool inGame;                   // 0x21
        bool setupPhase;               // 0x22
        bool isTutorial;               // 0x23
        bool isPublicServer;           // 0x24
        uint8_t _pad2[0x03];
        int32_t maxRoomPlayers;        // 0x28
        int32_t smallMapIndex;         // 0x2C
        uint8_t _pad3[0x08];
        Difficulty* currentDifficulty; // 0x38
        Difficulty* previousDifficulty;// 0x40
        void* map;                     // 0x48
        uint8_t _pad4[0x40];
        void* equipmentLoadout;        // 0x88
        void* currentWeather;          // 0x90
    };

    struct ObjectiveManager : MonoBehaviour
    {
        uint8_t _pad1[0xB8];           // 0x20-0xD7
        void* objectiveStates;         // 0xD8 - int[]
    };

    struct RewardUI : MonoBehaviour
    {
        void* rewardType;     // 0x20
        void* primaryText;    // 0x28 - TextMeshProUGUI
        void* valueText;      // 0x30 - TextMeshProUGUI
        void* iconImage;      // 0x38 - Image
    };

    struct RewardManager : MonoBehaviourPun
    {
        uint8_t _pad1[0xA8];               // 0x28-0xCF
        RewardUI* mainObjectiveRewardUI;   // 0xD0
        void* sideObjectivesRewardUI;      // 0xD8 - RewardUI[]
        RewardUI* dnaRewardUI;             // 0xE0
        RewardUI* investigationBonusRewardUI; // 0xE8
        RewardUI* perfectBonusRewardUI;    // 0xF0
        RewardUI* multiplierRewardUI;      // 0xF8
        RewardUI* negativeMultiplierRewardUI; // 0x100
        RewardUI* diedRewardUI;            // 0x108
        RewardUI* photoRewardUI;           // 0x110
        RewardUI* bloodMoonRewardUI;       // 0x118
        RewardUI* insuranceRewardUI;       // 0x120
        RewardUI* eventMultiplierRewardUI; // 0x128
        void* dailyRewardUis;              // 0x130 - RewardUI[]
        void* weeklyRewardUis;             // 0x138 - RewardUI[]
        RewardUI* challengeRewardUI;       // 0x140
        RewardUI* eventRewardUI;           // 0x148
        RewardUI* totalRewardUI;           // 0x150
        void* ghostTypeText;               // 0x158 - TextMeshProUGUI
        void* playerProfile;               // 0x160
        void* xpGainedText;                // 0x168 - TextMeshProUGUI
    };

    struct ChallengesController : MonoBehaviour
    {
        uint8_t _pad1[0x68];               // 0x20-0x87
    };

    struct WeatherProfile : Il2CppObject
    {
        uint8_t _pad1[0x08];           // 0x10-0x17
        void* azureProfile;            // 0x18
        int32_t weatherType;           // 0x20
        Color fogColor;                // 0x24
        float fogDensity;              // 0x34
        float timeOfDay;               // 0x38
    };

    struct RandomWeather : MonoBehaviour
    {
        void* weather;                 // 0x20
        void* environment;            // 0x28
        void* time;                   // 0x30
        void* effects;                // 0x38
        void* sky;                    // 0x40
        void* weatherParticles;       // 0x48
        void* cameraNightVisionPostProcess; // 0x50
        void* transitionPP;           // 0x58
        void* transitionAudio;        // 0x60
        void* directionalLight;       // 0x68
        void* allWeatherProfiles;      // 0x70 - WeatherProfile[]
        void* deadWeatherProfile;      // 0x78
        WeatherProfile* currentWeatherProfile; // 0x80
    };

    // --- InventoryEquipment (TypeDefIndex: 888) ---
    struct InventoryEquipment : MonoBehaviour
    {
        uint8_t _pad1[0x80];           // 0x20-0x7F (EquipmentInfo + UI refs)
        int32_t ownedCount;            // 0x80
        int32_t loadoutCount;          // 0x84
    };

    // --- InventoryManager (TypeDefIndex: 896) ---
    struct InventoryManager : MonoBehaviour
    {
        uint8_t _pad1[0xB0];           // 0x20-0xAF
        Il2CppArray* inventoryEquipment; // 0xB0 - InventoryEquipment[]
    };

    // --- StoreManager (TypeDefIndex: 983) ---
    struct StoreManager : MonoBehaviour
    {
        void* ghostOS;                 // 0x20
        void* storeItemInfo;           // 0x28
        void* xrSlider;               // 0x30
        uint8_t _pad1[0xA0];          // 0x38-0xD7 (color fields)
        Il2CppArray* allStoreEquipment;// 0xD8 - StoreEquipment[]
        void* selectedEquipment;       // 0xE0 - StoreEquipment
    };

    // --- GhostOS (TypeDefIndex: 880) ---
    struct GhostOS : MonoBehaviour
    {
        void* storeManager;            // 0x20
        void* inventoryManager;        // 0x28
        void* loadoutManager;          // 0x30
        void* singleplayerProfile;     // 0x38
        void* levelText;               // 0x40 (TMP_Text)
        void* playerMoneyText;         // 0x48 (TMP_Text)
    };

    // --- SingleplayerProfile (TypeDefIndex: 1525) ---
    struct ProfileCard : MonoBehaviourPun
    {
        uint8_t _pad1[0xB8];           // 0x28-0xDF
    };

    struct SingleplayerProfile : ProfileCard
    {
        uint8_t _pad1[0x48];           // 0xE0-0x127
    };

    struct SaveFileController : MonoBehaviour
    {
    };

    struct TarotCards : MonoBehaviour
    {
        uint8_t _pad1[0x1A8];
        bool inUse;                    // 0x1C0-ish in sdk, preserved via explicit fields below
        bool hasBroken;
        bool hasInitialised;
        uint8_t _pad2[0x45];           // 0x1CB-0x20F
        NullableInt32 forcedCard;      // 0x210
    };

    // ─── TarotCard (TypeDefIndex: 772) ──────────────────
    struct TarotCard : MonoBehaviour
    {
        uint8_t _pad1[0x08];
        void* tarotCardsRef;           // 0x28
        uint8_t _pad2[0x10];           // 0x30
        void* collider;                // 0x40
        void* smokeParticle;           // 0x48
        void* sparksParticle;          // 0x50
        void* meshes;                  // 0x58 - Mesh[]
    };

    // ─── Key (TypeDefIndex: 717) ────────────────────────
    struct Key : MonoBehaviour
    {
        uint8_t _pad1[0x08];
        void* photonInteract;          // 0x28
        void* keyInfo;                 // 0x30
    };

} // namespace Phasmo

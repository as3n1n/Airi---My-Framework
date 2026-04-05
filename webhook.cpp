#include "pch.h"
#include "cheat.h"
#include "phasmo_helpers.h"
#include "phasmo_offsets.h"
#include "phasmo_structs.h"
#include "logger.h"
#include <windows.h>
#include <wininet.h>
#include <algorithm>
#include <atomic>
#include <cctype>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "wininet.lib")

namespace
{
    struct Il2CppListRaw : Phasmo::Il2CppObject
    {
        void* _items;
        int32_t _size;
        int32_t _version;
        void* _syncRoot;
    };

    static std::atomic<int> s_webhookEventCount = 0;
    static char s_lastWebhookEvent[256] = "No events yet.";
    static bool s_lastInRoom = false;
    static bool s_lastInGame = false;
    static bool s_lastLocalDead = false;
    static bool s_sentGameIntelForCurrentContract = false;
    static std::vector<std::string> s_deadPlayersThisContract;
    static std::string s_lastRoomName;

    static std::string JsonEscape(const char* text)
    {
        std::string out;
        const char* s = text ? text : "";
        for (; *s; ++s) {
            switch (*s) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += *s; break;
            }
        }
        return out;
    }

    static bool LooksLikeDiscordWebhook(const char* url)
    {
        if (!url || !*url)
            return false;

        const std::string value(url);
        return value.find("https://discord.com/api/webhooks/") == 0 ||
               value.find("https://canary.discord.com/api/webhooks/") == 0 ||
               value.find("https://ptb.discord.com/api/webhooks/") == 0;
    }

    static bool LooksLikeWebhookTarget(const char* url)
    {
        if (!url || !*url)
            return false;
        const std::string value(url);
        return value.find("https://") == 0 || value.find("http://") == 0;
    }

    static void* TryGetCurrentRoomRaw()
    {
        __try { return Phasmo_Call<void*>(Phasmo::RVA_PN_GetCurrentRoom, nullptr); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    static Il2CppString* TryGetRoomNameRaw(void* room)
    {
        __try { return Phasmo_Call<Il2CppString*>(Phasmo::RVA_PR_GetName, room, nullptr); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
    }

    static bool TryGetRoomAndGameFlagsRaw(bool& inRoom, bool& inGame)
    {
        inRoom = false;
        inGame = false;
        __try {
            inRoom = Phasmo_Call<bool>(Phasmo::RVA_PN_GetInRoom);
            Phasmo::LevelValues* values = Phasmo_GetLevelValues();
            inGame = values && Phasmo_Safe(values, sizeof(Phasmo::LevelValues)) && values->inGame;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static bool TryGetLocalDeadRaw(bool& localDead)
    {
        localDead = false;
        __try {
            Phasmo::Network* network = Phasmo_GetNetwork();
            Phasmo::Player* localPlayer = nullptr;
            if (network && Phasmo_Safe(network, sizeof(Phasmo::Network)) && network->localPlayer)
                localPlayer = reinterpret_cast<Phasmo::Player*>(network->localPlayer);
            localDead = localPlayer && Phasmo_Safe(localPlayer, sizeof(Phasmo::Player)) && localPlayer->isDead;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }

    static std::string GetPhotonRoomName()
    {
        void* room = TryGetCurrentRoomRaw();

        if (!room || !Phasmo_Safe(room, 0x20))
            return {};

        Il2CppString* roomName = TryGetRoomNameRaw(room);

        return roomName ? Phasmo_StringToUtf8(roomName) : std::string{};
    }

    static std::string GetSpotSteamName(Phasmo::NetworkPlayerSpot* spot)
    {
        if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
            return {};

        if (spot->accountName && Phasmo_Safe(spot->accountName, sizeof(Il2CppString))) {
            std::string value = Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(spot->accountName));
            if (!value.empty())
                return value;
        }

        if (spot->photonPlayer && Phasmo_Safe(spot->photonPlayer, Phasmo::PP_NICKNAME + sizeof(void*))) {
            void* nickPtr = *reinterpret_cast<void**>(
                reinterpret_cast<uint8_t*>(spot->photonPlayer) + Phasmo::PP_NICKNAME);
            if (nickPtr && Phasmo_Safe(nickPtr, sizeof(Il2CppString))) {
                std::string value = Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(nickPtr));
                if (!value.empty())
                    return value;
            }
        }

        return {};
    }

    struct SteamRosterEntry
    {
        std::string displayName;
        std::string userId;
        std::string profileUrl;
        bool isLocal = false;
    };

    static std::string TryGetPhotonUserId(void* photonPlayer)
    {
        if (!photonPlayer)
            return {};

        Il2CppString* userId = Phasmo_Call<Il2CppString*>(Phasmo::RVA_PP_GetUserId, photonPlayer, nullptr);
        if (!userId || !Phasmo_Safe(userId, sizeof(Il2CppString)))
            return {};

        return Phasmo_StringToUtf8(userId);
    }

    static bool IsAllDigits(const std::string& value)
    {
        if (value.empty())
            return false;
        for (char c : value) {
            if (!std::isdigit(static_cast<unsigned char>(c)))
                return false;
        }
        return true;
    }

    static std::string BuildSteamProfileUrl(const std::string& userId)
    {
        if (userId.empty())
            return {};
        if (IsAllDigits(userId) && userId.size() >= 16)
            return "https://steamcommunity.com/profiles/" + userId;
        return "https://steamcommunity.com/id/" + userId;
    }

    static std::string UrlEncodeSimple(const std::string& value)
    {
        std::ostringstream out;
        for (unsigned char c : value) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
                out << c;
            } else if (c == ' ') {
                out << "%20";
            } else {
                const char* hex = "0123456789ABCDEF";
                out << '%' << hex[(c >> 4) & 0xF] << hex[c & 0xF];
            }
        }
        return out.str();
    }

    static std::string BuildSteamSearchUrl(const std::string& name)
    {
        if (name.empty())
            return {};
        return "https://steamcommunity.com/search/users/#text=" + UrlEncodeSimple(name);
    }

    static std::string FormatProfileMarkdown(const std::string& name, const std::string& profileUrl)
    {
        const std::string safeName = name.empty() ? std::string("Unknown") : name;
        std::string url = profileUrl;
        if (url.empty())
            url = BuildSteamSearchUrl(safeName);
        if (url.empty())
            return safeName;
        return "[" + safeName + "](" + url + ")";
    }

    static const char* EvidenceTypeName(int evidenceType)
    {
        switch (evidenceType)
        {
        case 0: return "EMF Spot";
        case 1: return "Fingerprints";
        case 2: return "Freezing Temps";
        case 3: return "Ghost Orbs";
        case 4: return "Ghost Writing";
        case 5: return "Spirit Box";
        case 6: return "DOTS";
        default: return "Unknown";
        }
    }

    static std::string EvidenceListToString(void* listPtr)
    {
        if (!listPtr || !Phasmo_Safe(listPtr, sizeof(Il2CppListRaw)))
            return {};

        Il2CppListRaw* list = reinterpret_cast<Il2CppListRaw*>(listPtr);
        if (!Phasmo_Safe(list, sizeof(Il2CppListRaw)) || !list->_items || list->_size <= 0 || list->_size > 16)
            return {};

        const size_t bytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(int32_t);
        if (!Phasmo_Safe(list->_items, bytes))
            return {};

        auto* items = reinterpret_cast<int32_t*>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        std::ostringstream out;
        bool first = true;
        for (int32_t i = 0; i < list->_size; ++i) {
            const char* name = EvidenceTypeName(items[i]);
            if (!name || !*name)
                continue;
            if (!first)
                out << ", ";
            out << name;
            first = false;
        }
        return out.str();
    }

    static bool TryBuildGhostIntel(std::string& outGhostName, std::string& outEvidence)
    {
        outGhostName.clear();
        outEvidence.clear();

        Phasmo::GhostAI* ghost = Phasmo_GetGhostAI();
        if (!ghost || !Phasmo_Safe(ghost, sizeof(Phasmo::GhostAI)))
            return false;

        Phasmo::GhostInfo* info = ghost->ghostInfo;
        if (!info || !Phasmo_Safe(info, sizeof(Phasmo::GhostInfo)))
            return false;

        if (info->ghostTypeInfo.ghostName && Phasmo_Safe(info->ghostTypeInfo.ghostName, sizeof(Il2CppString))) {
            outGhostName = Phasmo_StringToUtf8(reinterpret_cast<Il2CppString*>(info->ghostTypeInfo.ghostName));
        }

        const std::string evidenceA = EvidenceListToString(info->ghostTypeInfo.evidencePrimary);
        const std::string evidenceB = EvidenceListToString(info->ghostTypeInfo.evidenceSecondary);

        if (!evidenceA.empty() && !evidenceB.empty())
            outEvidence = evidenceA + " | " + evidenceB;
        else if (!evidenceA.empty())
            outEvidence = evidenceA;
        else if (!evidenceB.empty())
            outEvidence = evidenceB;

        return !outGhostName.empty() || !outEvidence.empty();
    }

    static void CollectNewPlayerDeaths(bool inGame, std::vector<std::string>& outDeaths)
    {
        outDeaths.clear();

        if (!inGame) {
            s_deadPlayersThisContract.clear();
            return;
        }

        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return;

        Il2CppListRaw* list = reinterpret_cast<Il2CppListRaw*>(network->playerSpots);
        if (!list || !Phasmo_Safe(list, sizeof(Il2CppListRaw)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return;

        auto** entries = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::NetworkPlayerSpot* spot = entries[i];
            if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
                continue;

            Phasmo::Player* player = reinterpret_cast<Phasmo::Player*>(spot->player);
            if (!player || !Phasmo_Safe(player, sizeof(Phasmo::Player)) || !player->isDead)
                continue;

            std::string name = GetSpotSteamName(spot);
            if (name.empty())
                name = "Unknown Player";

            if (std::find(s_deadPlayersThisContract.begin(), s_deadPlayersThisContract.end(), name) ==
                s_deadPlayersThisContract.end()) {
                s_deadPlayersThisContract.push_back(name);
                outDeaths.push_back(name);
            }
        }
    }

    static std::string BuildSteamRosterContext()
    {
        std::string localSteam = "Unknown";
        std::string localProfile;
        std::vector<SteamRosterEntry> players;

        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return {};

        void* localPhotonPlayer = nullptr;
        Phasmo::Player* localMapPlayer = nullptr;
        if (Phasmo_Safe(network, sizeof(Phasmo::Network))) {
            localMapPlayer = reinterpret_cast<Phasmo::Player*>(network->localPlayer);
            if (Phasmo_Safe(network->playerSpots, sizeof(Il2CppListRaw)))
                localPhotonPlayer = Phasmo_Call<void*>(Phasmo::RVA_PN_GetLocalPlayer, nullptr);
        }

        Il2CppListRaw* list = reinterpret_cast<Il2CppListRaw*>(network->playerSpots);
        if (!list || !Phasmo_Safe(list, sizeof(Il2CppListRaw)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return {};

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return {};

        auto** entries = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::NetworkPlayerSpot* spot = entries[i];
            if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
                continue;

            std::string name = GetSpotSteamName(spot);
            if (name.empty())
                name = "Unknown";
            const std::string userId = TryGetPhotonUserId(spot->photonPlayer);
            const std::string profile = BuildSteamProfileUrl(userId);

            const bool isLocal =
                (localPhotonPlayer && spot->photonPlayer == localPhotonPlayer) ||
                (localMapPlayer && spot->player == localMapPlayer);
            if (isLocal) {
                localSteam = name;
                localProfile = profile;
            }

            bool exists = false;
            for (const SteamRosterEntry& current : players) {
                if (current.displayName == name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                SteamRosterEntry entry{};
                entry.displayName = name;
                entry.userId = userId;
                entry.profileUrl = profile;
                entry.isLocal = isLocal;
                players.push_back(entry);
            }
        }

        if (players.empty() && localSteam == "Unknown")
            return {};

        std::ostringstream out;
        out << "**Local Steam:** " << FormatProfileMarkdown(localSteam, localProfile);
        if (!players.empty()) {
            out << "\n**Players:**";
            for (const SteamRosterEntry& player : players) {
                out << "\n- " << FormatProfileMarkdown(player.displayName, player.profileUrl);
                if (!player.userId.empty())
                    out << " (`" << player.userId << "`)";
            }
        }
        return out.str();
    }

    static std::string BuildPlayersField()
    {
        std::string roster = BuildSteamRosterContext();
        if (roster.empty())
            return "N/A";
        return roster;
    }

    static bool TryGetLocalIdentity(std::string& outName, std::string& outUnityId)
    {
        outName.clear();
        outUnityId.clear();

        Phasmo::Network* network = Phasmo_GetNetwork();
        if (!network || !Phasmo_Safe(network, sizeof(Phasmo::Network)))
            return false;

        void* localPhotonPlayer = nullptr;
        Phasmo::Player* localMapPlayer = reinterpret_cast<Phasmo::Player*>(network->localPlayer);
        if (Phasmo_Safe(network->playerSpots, sizeof(Il2CppListRaw)))
            localPhotonPlayer = Phasmo_Call<void*>(Phasmo::RVA_PN_GetLocalPlayer, nullptr);

        Il2CppListRaw* list = reinterpret_cast<Il2CppListRaw*>(network->playerSpots);
        if (!list || !Phasmo_Safe(list, sizeof(Il2CppListRaw)) || !list->_items || list->_size <= 0 || list->_size > 32)
            return false;

        const size_t arrayBytes = Phasmo::ARRAY_FIRST_ELEMENT + static_cast<size_t>(list->_size) * sizeof(void*);
        if (!Phasmo_Safe(list->_items, arrayBytes))
            return false;

        auto** entries = reinterpret_cast<Phasmo::NetworkPlayerSpot**>(
            reinterpret_cast<uint8_t*>(list->_items) + Phasmo::ARRAY_FIRST_ELEMENT);

        for (int32_t i = 0; i < list->_size; ++i) {
            Phasmo::NetworkPlayerSpot* spot = entries[i];
            if (!spot || !Phasmo_Safe(spot, sizeof(Phasmo::NetworkPlayerSpot)))
                continue;

            const bool isLocal =
                (localPhotonPlayer && spot->photonPlayer == localPhotonPlayer) ||
                (localMapPlayer && spot->player == localMapPlayer);
            if (!isLocal)
                continue;

            outName = GetSpotSteamName(spot);
            outUnityId = TryGetPhotonUserId(spot->photonPlayer);
            return !outName.empty() || !outUnityId.empty();
        }

        return false;
    }

    static void SendWebhookAsync(const std::string& url, const std::string& jsonBody)
    {
        if (url.empty())
            return;

        std::thread([url, jsonBody]()
        {
            char host[256] = {};
            char path[1024] = {};

            const char* p = url.c_str();
            INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
            DWORD openFlags = INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD;

            if (strncmp(p, "https://", 8) == 0)
                p += 8;
            else if (strncmp(p, "http://", 7) == 0) {
                p += 7;
                port = INTERNET_DEFAULT_HTTP_PORT;
                openFlags = INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_RELOAD;
            }

            const char* slash = strchr(p, '/');
            if (!slash)
                return;

            strncpy_s(host, p, slash - p);
            strncpy_s(path, slash, sizeof(path) - 1);

            HINTERNET hInet = InternetOpenA("Chino-Heart/Phasmo", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
            if (!hInet)
                return;

            HINTERNET hConn = InternetConnectA(hInet, host, port, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
            if (!hConn) {
                InternetCloseHandle(hInet);
                return;
            }

            HINTERNET hReq = HttpOpenRequestA(hConn, "POST", path, nullptr, nullptr, nullptr, openFlags, 0);
            if (!hReq) {
                InternetCloseHandle(hConn);
                InternetCloseHandle(hInet);
                return;
            }

            const char* headers = "Content-Type: application/json\r\nUser-Agent: Chino-Heart/Phasmo\r\n";
            HttpSendRequestA(hReq, headers, static_cast<DWORD>(strlen(headers)),
                const_cast<char*>(jsonBody.data()), static_cast<DWORD>(jsonBody.size()));

            InternetCloseHandle(hReq);
            InternetCloseHandle(hConn);
            InternetCloseHandle(hInet);
        }).detach();
    }
}

namespace Cheat
{
    void Webhook_Send(const char* event, const char* detail)
    {
        if (!NETWORK_WebhookEnabled)
            return;
        if (!LooksLikeWebhookTarget(NETWORK_WebhookUrl))
            return;

        s_webhookEventCount.fetch_add(1);
        sprintf_s(s_lastWebhookEvent, "[%s] %s", event ? event : "Event", detail ? detail : "");

        std::string description = detail ? detail : "";

        bool inRoom = false;
        bool inGame = false;
        TryGetRoomAndGameFlagsRaw(inRoom, inGame);
        const std::string roomName = inRoom ? GetPhotonRoomName() : std::string{};

        std::string ghostName;
        std::string evidence;
        if (inGame)
            TryBuildGhostIntel(ghostName, evidence);

        const std::string playersField = BuildPlayersField();
        const std::string contractState = inGame ? "In Contract" : "Lobby/Menu";
        bool localDead = false;
        TryGetLocalDeadRaw(localDead);

        std::string localName;
        std::string localUnityId;
        TryGetLocalIdentity(localName, localUnityId);

        const bool isDiscord = LooksLikeDiscordWebhook(NETWORK_WebhookUrl);
        if (isDiscord) {
            const std::string roster = BuildSteamRosterContext();
            if (!roster.empty()) {
                if (!description.empty())
                    description += "\n";
                description += roster;
            }
        }
        std::ostringstream body;
        if (isDiscord) {
            body << "{";
            body << "\"username\":\"Chino-Heart\",";
            body << "\"embeds\":[{";
            body << "\"title\":\"" << JsonEscape(event ? event : "Event") << "\",";
            body << "\"description\":\"" << JsonEscape(description.c_str()) << "\",";
            body << "\"color\":15158332,";
            body << "\"fields\":[";
            body << "{\"name\":\"Room\",\"value\":\"" << JsonEscape(roomName.empty() ? "N/A" : roomName.c_str()) << "\",\"inline\":true},";
            body << "{\"name\":\"Contract\",\"value\":\"" << JsonEscape(contractState.c_str()) << "\",\"inline\":true},";
            body << "{\"name\":\"Ghost\",\"value\":\"" << JsonEscape(ghostName.empty() ? "Unknown" : ghostName.c_str()) << "\",\"inline\":true},";
            body << "{\"name\":\"Evidence\",\"value\":\"" << JsonEscape(evidence.empty() ? "Unknown" : evidence.c_str()) << "\",\"inline\":false},";
            body << "{\"name\":\"Players\",\"value\":\"" << JsonEscape(playersField.c_str()) << "\",\"inline\":false},";
            body << "{\"name\":\"Local Unity ID\",\"value\":\"" << JsonEscape(localUnityId.empty() ? "Unknown" : localUnityId.c_str()) << "\",\"inline\":false}";
            body << "]";
            body << "}]}";
        } else {
            body << "{";
            body << "\"event\":\"" << JsonEscape(event ? event : "Event") << "\",";
            body << "\"detail\":\"" << JsonEscape(description.c_str()) << "\",";
            body << "\"room\":\"" << JsonEscape(roomName.empty() ? "N/A" : roomName.c_str()) << "\",";
            body << "\"contract\":\"" << JsonEscape(contractState.c_str()) << "\",";
            body << "\"ghost\":\"" << JsonEscape(ghostName.empty() ? "Unknown" : ghostName.c_str()) << "\",";
            body << "\"evidence\":\"" << JsonEscape(evidence.empty() ? "Unknown" : evidence.c_str()) << "\",";
            body << "\"players\":\"" << JsonEscape(playersField.c_str()) << "\",";
            body << "\"local_name\":\"" << JsonEscape(localName.empty() ? "Unknown" : localName.c_str()) << "\",";
            body << "\"local_unity_id\":\"" << JsonEscape(localUnityId.empty() ? "Unknown" : localUnityId.c_str()) << "\",";
            body << "\"local_dead\":" << (localDead ? "true" : "false") << ",";
            body << "\"in_game\":" << (inGame ? "true" : "false");
            body << "}";
        }

        SendWebhookAsync(NETWORK_WebhookUrl, body.str());
    }

    void Webhook_LogCommand(const char* event, const char* detail)
    {
        PushNotification(event ? event : "Command", detail);
        if (NETWORK_WebhookLogCommands)
            Webhook_Send(event, detail);
    }

    void Webhook_Setup()
    {
        s_lastInRoom = false;
        s_lastInGame = false;
        s_lastLocalDead = false;
        s_sentGameIntelForCurrentContract = false;
        s_lastRoomName.clear();

        PushNotification("Chino-Heart", "Session initialized");
        if (NETWORK_WebhookEnabled && NETWORK_WebhookLogSession)
            Webhook_Send("Session Start", "Chino-Heart has been injected.");
    }

    void Webhook_Shutdown()
    {
        if (NETWORK_WebhookEnabled && NETWORK_WebhookLogSession)
            Webhook_Send("Session End", "Chino-Heart shutdown requested.");
    }

    void Webhook_TrackState()
    {
        const bool webhookOn = NETWORK_WebhookEnabled && LooksLikeWebhookTarget(NETWORK_WebhookUrl);

        bool inRoom = false;
        bool inGame = false;
        TryGetRoomAndGameFlagsRaw(inRoom, inGame);

        const std::string roomName = inRoom ? GetPhotonRoomName() : std::string{};

        if (inRoom && (!s_lastInRoom || roomName != s_lastRoomName)) {
            std::string msg = roomName.empty() ? "Joined a Photon room." : ("Joined room: " + roomName);
            PushNotification("Room Joined", msg.c_str());
            if (webhookOn && NETWORK_WebhookLogRooms)
                Webhook_Send("Room Joined", msg.c_str());
        }

        if (!inRoom && s_lastInRoom) {
            std::string msg = s_lastRoomName.empty() ? "Left the current Photon room." : ("Left room: " + s_lastRoomName);
            PushNotification("Room Left", msg.c_str());
            if (webhookOn && NETWORK_WebhookLogRooms)
                Webhook_Send("Room Left", msg.c_str());
        }

        if (inGame && !s_lastInGame) {
            s_sentGameIntelForCurrentContract = false;
            s_deadPlayersThisContract.clear();
            PushNotification("Contract Started", "Investigation has started");
            if (webhookOn && NETWORK_WebhookLogContracts)
                Webhook_Send("Contract Started", "Investigation has started.");
        }

        if (!inGame && s_lastInGame) {
            s_sentGameIntelForCurrentContract = false;
            s_deadPlayersThisContract.clear();
            const bool survived = !s_lastLocalDead;
            const char* resultText = survived ? "SUCCESS (Alive)" : "FAILED (Died)";
            std::string endMsg = std::string("Returned from investigation. Result: ") + resultText;
            PushNotification("Contract Ended", endMsg.c_str());
            if (webhookOn && NETWORK_WebhookLogContracts)
                Webhook_Send("Contract Ended", endMsg.c_str());
        }

        if (inGame && !s_sentGameIntelForCurrentContract) {
            std::string ghostName;
            std::string evidence;
            if (TryBuildGhostIntel(ghostName, evidence)) {
                std::string detail = "Ghost: " + (ghostName.empty() ? std::string("Unknown") : ghostName);
                if (!evidence.empty())
                    detail += " | Evidence: " + evidence;

                PushNotification("Game Intel", detail.c_str());
                if (webhookOn && NETWORK_WebhookLogContracts)
                    Webhook_Send("Game Intel", detail.c_str());
                s_sentGameIntelForCurrentContract = true;
            }
        }

        bool localDead = false;
        TryGetLocalDeadRaw(localDead);

        if (localDead && !s_lastLocalDead) {
            std::string detail = "Local player death detected.";
            std::string ghostName;
            std::string evidence;
            if (TryBuildGhostIntel(ghostName, evidence)) {
                detail += " Ghost: ";
                detail += ghostName.empty() ? "Unknown" : ghostName;
                if (!evidence.empty()) {
                    detail += " | Evidence: ";
                    detail += evidence;
                }
            }

            PushNotification("You Died", detail.c_str());
            if (webhookOn && NETWORK_WebhookLogDeaths)
                Webhook_Send("Player Death", detail.c_str());
        }

        std::vector<std::string> newDeaths;
        CollectNewPlayerDeaths(inGame, newDeaths);
        if (webhookOn && NETWORK_WebhookLogDeaths) {
            for (const std::string& name : newDeaths) {
                std::string detail = "Player death detected: " + name;
                Webhook_Send("Player Death", detail.c_str());
            }
        }

        s_lastInRoom = inRoom;
        s_lastInGame = inGame;
        s_lastLocalDead = localDead;
        s_lastRoomName = roomName;
    }
}

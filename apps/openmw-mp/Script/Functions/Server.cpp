#include "Server.hpp"

#include <components/misc/stringops.hpp>
#include <components/openmw-mp/NetworkMessages.hpp>
#include <components/openmw-mp/Log.hpp>
#include <components/openmw-mp/Version.hpp>

#include <apps/openmw-mp/Script/ScriptFunctions.hpp>
#include <apps/openmw-mp/Networking.hpp>
#include <apps/openmw-mp/MasterClient.hpp>
#include <Script/Script.hpp>

static std::string tempFilename;
static std::chrono::high_resolution_clock::time_point startupTime = std::chrono::high_resolution_clock::now();

void ServerFunctions::LogMessage(unsigned short level, const char *message) noexcept
{
    LOG_MESSAGE_SIMPLE(level, "[Script]: %s", message);
}

void ServerFunctions::LogAppend(unsigned short level, const char *message) noexcept
{
    LOG_APPEND(level, "[Script]: %s", message);
}

void ServerFunctions::StopServer(int code) noexcept
{
    mwmp::Networking::getPtr()->stopServer(code);
}

void ServerFunctions::Kick(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player,);

    LOG_MESSAGE_SIMPLE(Log::LOG_INFO, "Kicking player %s (%i)", player->npc.mName.c_str(), player->getId());
    mwmp::Networking::getPtr()->kickPlayer(player->guid);
    player->setLoadState(Player::KICKED);
}

void ServerFunctions::BanAddress(const char *ipAddress) noexcept
{
    mwmp::Networking::getPtr()->banAddress(ipAddress);
}

void ServerFunctions::UnbanAddress(const char *ipAddress) noexcept
{
    mwmp::Networking::getPtr()->unbanAddress(ipAddress);
}

bool ServerFunctions::DoesFilePathExist(const char *filePath) noexcept
{
    return boost::filesystem::exists(filePath);
}

const char *ServerFunctions::GetCaseInsensitiveFilename(const char *folderPath, const char *filename) noexcept
{
    if (!boost::filesystem::exists(folderPath)) return "invalid";

    boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end

    for (boost::filesystem::directory_iterator itr(folderPath); itr != end_itr; ++itr)
    {
        if (Misc::StringUtils::ciEqual(itr->path().filename().string(), filename))
        {
            tempFilename = itr->path().filename().string();
            return tempFilename.c_str();
        }
    }
    return "invalid";
}

const char* ServerFunctions::GetDataPath() noexcept
{
    return Script::GetModDir();
}

unsigned int ServerFunctions::GetMillisecondsSinceServerStart() noexcept
{
    std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
    std::chrono::milliseconds milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startupTime);
    return milliseconds.count();
}

const char *ServerFunctions::GetOperatingSystemType() noexcept
{
    return Utils::getOperatingSystemType().c_str();
}

const char *ServerFunctions::GetArchitectureType() noexcept
{
    return Utils::getArchitectureType().c_str();
}

const char *ServerFunctions::GetServerVersion() noexcept
{
    return TES3MP_VERSION;
}

const char *ServerFunctions::GetProtocolVersion() noexcept
{
    static std::string version = std::to_string(TES3MP_PROTO_VERSION);
    return version.c_str();
}

int ServerFunctions::GetAvgPing(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, -1);
    return mwmp::Networking::get().getAvgPing(player->guid);
}

const char *ServerFunctions::GetIP(unsigned short pid) noexcept
{
    Player *player;
    GET_PLAYER(pid, player, "");
    RakNet::SystemAddress addr = mwmp::Networking::getPtr()->getSystemAddress(player->guid);
    return addr.ToString(false);
}

unsigned short ServerFunctions::GetPort() noexcept
{
    return mwmp::Networking::get().getPort();
}

unsigned int ServerFunctions::GetMaxPlayers() noexcept
{
    return mwmp::Networking::get().maxConnections();
}

bool ServerFunctions::HasPassword() noexcept
{
    return mwmp::Networking::get().isPassworded();
}

bool ServerFunctions::GetPluginEnforcementState() noexcept
{
    return mwmp::Networking::getPtr()->getPluginEnforcementState();
}

bool ServerFunctions::GetScriptErrorIgnoringState() noexcept
{
    return mwmp::Networking::getPtr()->getScriptErrorIgnoringState();
}

void ServerFunctions::SetGameMode(const char *gameMode) noexcept
{
    if (mwmp::Networking::getPtr()->getMasterClient())
        mwmp::Networking::getPtr()->getMasterClient()->SetModname(gameMode);
}

void ServerFunctions::SetHostname(const char *name) noexcept
{
    if (mwmp::Networking::getPtr()->getMasterClient())
        mwmp::Networking::getPtr()->getMasterClient()->SetHostname(name);
}

void ServerFunctions::SetServerPassword(const char *password) noexcept
{
    mwmp::Networking::getPtr()->setServerPassword(password);
}

void ServerFunctions::SetPluginEnforcementState(bool state) noexcept
{
    mwmp::Networking::getPtr()->setPluginEnforcementState(state);
}

void ServerFunctions::SetScriptErrorIgnoringState(bool state) noexcept
{
    mwmp::Networking::getPtr()->setScriptErrorIgnoringState(state);
}

void ServerFunctions::SetRuleString(const char *key, const char *value) noexcept
{
    auto mc = mwmp::Networking::getPtr()->getMasterClient();
    if (mc)
        mc->SetRuleString(key, value);
}

void ServerFunctions::SetRuleValue(const char *key, double value) noexcept
{
    auto mc = mwmp::Networking::getPtr()->getMasterClient();
    if (mc)
        mc->SetRuleValue(key, value);
}

void ServerFunctions::AddDataFileRequirement(const char *dataFilename, const char *checksumString) noexcept
{
    auto &samples = mwmp::Networking::getPtr()->getSamples();
    auto it = std::find_if(samples.begin(), samples.end(), [&dataFilename](mwmp::PacketPreInit::PluginPair &item) {
        return item.first == dataFilename;
    });
    if (it != samples.end())
        it->second.push_back((unsigned) std::stoul(checksumString));
    else
    {
        mwmp::PacketPreInit::HashList checksumList;

        unsigned checksum = 0;

        if (strlen(checksumString) != 0)
        {
            checksum = (unsigned) std::stoul(checksumString);
            checksumList.push_back(checksum);
        }
        samples.emplace_back(dataFilename, checksumList);

        auto mclient = mwmp::Networking::getPtr()->getMasterClient();
        if (mclient)
            mclient->PushPlugin({dataFilename, checksum});
    }
}

// All methods below are deprecated versions of methods from above

bool ServerFunctions::DoesFileExist(const char *filePath) noexcept
{
    return DoesFilePathExist(filePath);
}

const char* ServerFunctions::GetModDir() noexcept
{
    return GetDataPath();
}

void ServerFunctions::AddPluginHash(const char *pluginName, const char *checksumString) noexcept
{
    AddDataFileRequirement(pluginName, checksumString);
}

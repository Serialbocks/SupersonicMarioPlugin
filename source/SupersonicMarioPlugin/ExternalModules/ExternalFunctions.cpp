#include "SupersonicMarioPlugin.h"

#include "RLConstants.inc"

bool SupersonicMarioPlugin::isMapJoinable(const std::filesystem::path&)
{
    BM_WARNING_LOG("redacted function");

    return false;
}


std::wstring SupersonicMarioPlugin::getPlayerNickname(const uint64_t) const
{
    BM_WARNING_LOG("redacted function");

    return L"";
}


void SupersonicMarioPlugin::getSubscribedWorkshopMapsAsync(const bool)
{
    BM_WARNING_LOG("redacted function");
}


void SupersonicMarioPlugin::setMatchSettings(const std::string&) const
{
    BM_WARNING_LOG("redacted function");
}


void SupersonicMarioPlugin::setMatchMapName(const std::string&) const
{
    BM_WARNING_LOG("redacted function");
}


bool SupersonicMarioPlugin::setSearchStatus(const std::wstring&, const bool) const
{
    BM_WARNING_LOG("redacted function");

    return false;
}


void SupersonicMarioPlugin::broadcastJoining()
{
    BM_WARNING_LOG("redacted function");
}


bool SupersonicMarioPlugin::isHostingLocalGame() const
{
    BM_WARNING_LOG("redacted function");

    return false;
}


bool SupersonicMarioPlugin::preLoadMap(const std::filesystem::path&, const bool, const bool)
{
    BM_WARNING_LOG("redacted function");

    return false;
}


void SupersonicMarioPlugin::loadRLConstants()
{
    gameModes = RLConstants::GAME_MODES;
    maps = RLConstants::MAPS;
    botDifficulties = RLConstants::BOT_DIFFICULTIES;
    customColorHues = RLConstants::CUSTOM_HUE_COUNT;
    customColors = RLConstants::CUSTOM_COLORS;
    clubColorHues = RLConstants::CLUB_HUE_COUNT;
    clubColors = RLConstants::CUSTOM_COLORS;
    defaultBluePrimaryColor = RLConstants::DEFAULT_BLUE_PRIMARY_COLOR;
    defaultBlueAccentColor = RLConstants::DEFAULT_BLUE_ACCENT_COLOR;
    defaultOrangePrimaryColor = RLConstants::DEFAULT_ORANGE_PRIMARY_COLOR;
    defaultOrangeAccentColor = RLConstants::DEFAULT_ORANGE_ACCENT_COLOR;
}


bool SupersonicMarioPlugin::isCurrentMapModded() const
{
    BM_WARNING_LOG("redacted function");

    return false;
}

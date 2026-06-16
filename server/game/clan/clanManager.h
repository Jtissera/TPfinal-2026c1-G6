#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clan.h"

class GameManager;

class ClanManager
{
public:
    static ClanManager &instance();

    void bindGameManager(GameManager *gameManager);

    enum class Result
    {
        OK,
        NAME_TAKEN,
        ALREADY_IN_CLAN,
        CLAN_NOT_FOUND,
        NOT_FOUNDER,
        NOT_A_MEMBER,
        NOT_AN_APPLICANT,
        ALREADY_APPLIED,
        ALREADY_MEMBER,
        BANNED,
        CLAN_FULL,
        FOUNDER_CANNOT_LEAVE,
        CANNOT_KICK_SELF,
    };

    Result foundClan(const std::string &clanName, const std::string &founderNick);
    Result applyToJoin(const std::string &clanName, const std::string &applicantNick);
    Result acceptApplicant(const std::string &founderNick, const std::string &targetNick);
    Result rejectApplicant(const std::string &founderNick, const std::string &targetNick);
    Result banPlayer(const std::string &founderNick, const std::string &targetNick);
    Result kickMember(const std::string &founderNick, const std::string &targetNick);
    Result leaveClan(const std::string &nick);

    std::optional<std::pair<std::string, bool>> findClanInfoForMember(const std::string &nick) const;

    struct ClanOverview
    {
        std::string clanName;
        std::vector<std::string> members;
        std::vector<std::string> applicants;
    };
    std::optional<ClanOverview> getOverviewForFounder(const std::string &founderNick) const;

    void notifyPlayer(const std::string &nick, const std::string &text);
    void notifyClan(const std::string &clanName, const std::string &excludeNick, const std::string &text);

    void syncPlayerClanState(const std::string &nick);

    ClanManager(const ClanManager &) = delete;
    ClanManager &operator=(const ClanManager &) = delete;

private:
    ClanManager() = default;

    Clan *findClanByNameUnlocked(const std::string &name);
    Clan *findClanOfMemberUnlocked(const std::string &nick);
    void removeApplicantEverywhereUnlocked(const std::string &nick);

    mutable std::mutex mutex;
    std::unordered_map<std::string, Clan> clans;
    GameManager *gameManager = nullptr;
};
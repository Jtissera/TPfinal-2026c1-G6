#pragma once

#include "BinaryArchiveFile.h"
#include "../game/clan/clan.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <set>

namespace ClanArchiveConstants
{
    const std::size_t NAME_LEN       = 32;
    const std::size_t MAX_MEMBERS    = 16;
    const std::size_t MAX_APPLICANTS = 32;
    const std::size_t MAX_BANNED     = 64;
}

struct ClanRecord
{
    uint8_t memberCount    = 0;
    uint8_t applicantCount = 0;
    uint8_t bannedCount    = 0;
    uint8_t _pad           = 0;

    char name[ClanArchiveConstants::NAME_LEN]        = {};
    char founderNick[ClanArchiveConstants::NAME_LEN] = {};

    char members[ClanArchiveConstants::MAX_MEMBERS][ClanArchiveConstants::NAME_LEN]       = {};
    char applicants[ClanArchiveConstants::MAX_APPLICANTS][ClanArchiveConstants::NAME_LEN] = {};
    char bannedPlayers[ClanArchiveConstants::MAX_BANNED][ClanArchiveConstants::NAME_LEN]  = {};
};
static_assert(std::is_trivially_copyable_v<ClanRecord>, "ClanRecord must be POD");

class ClanArchive
{
public:
    ClanArchive(const std::string &datPath, const std::string &indexPath);

    void save(const Clan &clan);
    bool exists(const std::string &clanName) const;
    std::optional<Clan> load(const std::string &clanName) const;
    std::vector<Clan> loadAll() const;

private:
    ClanRecord toRecord(const Clan &clan) const;
    Clan fromRecord(const ClanRecord &rec) const;

    BinaryArchiveFile<std::string> archive_;
};
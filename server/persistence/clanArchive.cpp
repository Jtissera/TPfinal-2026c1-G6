#include "clanArchive.h"

#include <cstring>
#include <iostream>
#include <set>

ClanArchive::ClanArchive(const std::string &datPath,
                         const std::string &indexPath)
    : archive_(datPath, indexPath, sizeof(ClanRecord), ClanArchiveConstants::NAME_LEN)
{
}

bool ClanArchive::exists(const std::string &clanName) const
{
    return archive_.hasKey(clanName);
}

ClanRecord ClanArchive::toRecord(const Clan &clan) const
{
    ClanRecord rec;
    std::memset(&rec, 0, sizeof(rec));

    std::strncpy(rec.name, clan.getName().c_str(), sizeof(rec.name) - 1);
    std::strncpy(rec.founderNick, clan.getFounderNick().c_str(),
                 sizeof(rec.founderNick) - 1);

    const std::set<std::string> &members = clan.getMembers();
    rec.memberCount = static_cast<uint8_t>(
        std::min(members.size(), ClanArchiveConstants::MAX_MEMBERS));
    std::size_t i = 0;
    for (std::set<std::string>::const_iterator it = members.begin();
         it != members.end() && i < rec.memberCount; ++it, ++i)
    {
        std::strncpy(rec.members[i], it->c_str(), ClanArchiveConstants::NAME_LEN - 1);
    }

    const std::set<std::string> &applicants = clan.getApplicants();
    rec.applicantCount = static_cast<uint8_t>(
        std::min(applicants.size(), ClanArchiveConstants::MAX_APPLICANTS));
    i = 0;
    for (std::set<std::string>::const_iterator it = applicants.begin();
         it != applicants.end() && i < rec.applicantCount; ++it, ++i)
    {
        std::strncpy(rec.applicants[i], it->c_str(), ClanArchiveConstants::NAME_LEN - 1);
    }

    const std::set<std::string> &banned = clan.getBannedPlayers();
    rec.bannedCount = static_cast<uint8_t>(
        std::min(banned.size(), ClanArchiveConstants::MAX_BANNED));
    i = 0;
    for (std::set<std::string>::const_iterator it = banned.begin();
         it != banned.end() && i < rec.bannedCount; ++it, ++i)
    {
        std::strncpy(rec.bannedPlayers[i], it->c_str(), ClanArchiveConstants::NAME_LEN - 1);
    }

    return rec;
}

Clan ClanArchive::fromRecord(const ClanRecord &rec) const
{
    std::string name(rec.name, strnlen(rec.name, sizeof(rec.name)));
    std::string founder(rec.founderNick, strnlen(rec.founderNick, sizeof(rec.founderNick)));

    Clan clan(name, founder, ClanArchiveConstants::MAX_MEMBERS);

    for (uint8_t i = 0; i < rec.memberCount; ++i)
    {
        std::string nick(rec.members[i], strnlen(rec.members[i], ClanArchiveConstants::NAME_LEN));
        clan.addMember(nick);
    }

    for (uint8_t i = 0; i < rec.applicantCount; ++i)
    {
        std::string nick(rec.applicants[i], strnlen(rec.applicants[i], ClanArchiveConstants::NAME_LEN));
        clan.addApplicant(nick);
    }

    for (uint8_t i = 0; i < rec.bannedCount; ++i)
    {
        std::string nick(rec.bannedPlayers[i], strnlen(rec.bannedPlayers[i], ClanArchiveConstants::NAME_LEN));
        clan.banPlayer(nick);
    }

    return clan;
}

void ClanArchive::save(const Clan &clan)
{
    const std::string &name = clan.getName();
    const ClanRecord rec = toRecord(clan);

    std::optional<uint64_t> existingOffset = archive_.getOffset(name);
    if (existingOffset)
    {
        archive_.writeRecord(*existingOffset, &rec, sizeof(rec));
    }
    else
    {
        const uint64_t offset = archive_.allocateSlot(name);
        archive_.writeRecord(offset, &rec, sizeof(rec));
        archive_.appendToIndex(name, offset);
    }

    std::cout << "[ClanArchive] Guardado clan '" << name
              << "' members=" << static_cast<int>(rec.memberCount) << std::endl;
}

std::optional<Clan> ClanArchive::load(const std::string &clanName) const
{
    std::optional<uint64_t> offset = archive_.getOffset(clanName);
    if (!offset)
        return std::nullopt;

    ClanRecord rec;
    if (!archive_.readRecord(*offset, &rec, sizeof(rec)))
        return std::nullopt;

    return fromRecord(rec);
}

std::vector<Clan> ClanArchive::loadAll() const
{
    std::vector<Clan> result;
    const std::vector<uint64_t> offsets = archive_.allOffsets();

    for (uint64_t offset : offsets)
    {
        ClanRecord rec;
        if (archive_.readRecord(offset, &rec, sizeof(rec)))
            result.push_back(fromRecord(rec));
    }

    return result;
}
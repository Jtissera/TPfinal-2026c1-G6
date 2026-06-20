#include "clanManager.h"

#include <algorithm>
#include <cctype>
#include <iostream>

#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "server/game/session/gameManager.h"

namespace
{
    std::string toLower(const std::string &s)
    {
        std::string out = s;
        std::transform(out.begin(), out.end(), out.begin(),
                       [](unsigned char c)
                       { return std::tolower(c); });
        return out;
    }
}

ClanManager::ClanManager(ClanArchive &clanArchive,
                         CharacterArchive &characterArchive)
    : clanArchive(clanArchive), characterArchive(characterArchive) {}

void ClanManager::bindGameManager(GameManager *gm)
{
    std::lock_guard<std::mutex> lock(mutex);
    gameManager = gm;
}

void ClanManager::restoreFromArchive()
{
    auto loaded = clanArchive.loadAll();

    std::lock_guard<std::mutex> lock(mutex);
    for (auto &clan : loaded)
    {
        std::string key = toLower(clan.getName());
        std::cout << "[ClanManager] restore clan='" << clan.getName()
                  << "' founder='" << clan.getFounderNick()
                  << "' members=" << clan.getMembers().size() << std::endl;
        clans.emplace(key, std::move(clan));
    }
}

Clan *ClanManager::findClanByNameUnlocked(const std::string &name)
{
    auto it = clans.find(toLower(name));
    return it == clans.end() ? nullptr : &it->second;
}

Clan *ClanManager::findClanOfMemberUnlocked(const std::string &nick)
{
    for (auto &[key, clan] : clans)
    {
        if (clan.isMember(nick))
            return &clan;
    }
    return nullptr;
}

void ClanManager::removeApplicantEverywhereUnlocked(const std::string &nick)
{
    for (auto &[key, clan] : clans)
        clan.removeApplicant(nick);
}

ClanManager::Result ClanManager::foundClan(const std::string &clanName,
                                           const std::string &founderNick)
{
    std::lock_guard<std::mutex> lock(mutex);

    if (clanName.empty())
        return Result::CLAN_NOT_FOUND;

    if (findClanByNameUnlocked(clanName) != nullptr)
        return Result::NAME_TAKEN;

    if (findClanOfMemberUnlocked(founderNick) != nullptr)
        return Result::ALREADY_IN_CLAN;

    Clan clan(clanName, founderNick);
    clan.addMember(founderNick);

    // Persistir ANTES de mover el clan al mapa, así tenemos una copia
    // estable para pasarle al archive.
    clanArchive.save(clan);
    characterArchive.updateClan(founderNick, clanName);

    clans.emplace(toLower(clanName), std::move(clan));
    return Result::OK;
}

ClanManager::Result ClanManager::applyToJoin(const std::string &clanName,
                                             const std::string &applicantNick)
{
    std::lock_guard<std::mutex> lock(mutex);

    Clan *clan = findClanByNameUnlocked(clanName);
    if (clan == nullptr)
        return Result::CLAN_NOT_FOUND;

    if (findClanOfMemberUnlocked(applicantNick) != nullptr)
        return Result::ALREADY_IN_CLAN;

    if (clan->isBanned(applicantNick))
        return Result::BANNED;

    if (clan->isMember(applicantNick))
        return Result::ALREADY_MEMBER;

    if (clan->isApplicant(applicantNick))
        return Result::ALREADY_APPLIED;

    clan->addApplicant(applicantNick);

    // applicants no afecta CharacterRecord (el personaje no tiene clan
    // todavía), pero sí cambia el estado del Clan en sí.
    clanArchive.save(*clan);

    return Result::OK;
}

ClanManager::Result ClanManager::acceptApplicant(const std::string &founderNick,
                                                 const std::string &targetNick)
{
    std::lock_guard<std::mutex> lock(mutex);

    Clan *clan = findClanOfMemberUnlocked(founderNick);
    if (clan == nullptr)
        return Result::NOT_A_MEMBER;
    if (clan->getFounderNick() != founderNick)
        return Result::NOT_FOUNDER;
    if (!clan->isApplicant(targetNick))
        return Result::NOT_AN_APPLICANT;
    if (clan->isFull())
        return Result::CLAN_FULL;

    clan->removeApplicant(targetNick);
    clan->addMember(targetNick);
    removeApplicantEverywhereUnlocked(targetNick);

    clanArchive.save(*clan);
    characterArchive.updateClan(targetNick, clan->getName());

    return Result::OK;
}

ClanManager::Result ClanManager::rejectApplicant(const std::string &founderNick,
                                                 const std::string &targetNick)
{
    std::lock_guard<std::mutex> lock(mutex);

    Clan *clan = findClanOfMemberUnlocked(founderNick);
    if (clan == nullptr)
        return Result::NOT_A_MEMBER;
    if (clan->getFounderNick() != founderNick)
        return Result::NOT_FOUNDER;
    if (!clan->isApplicant(targetNick))
        return Result::NOT_AN_APPLICANT;

    clan->removeApplicant(targetNick);

    // El rechazado nunca fue miembro, así que el CharacterRecord no cambia.
    clanArchive.save(*clan);

    return Result::OK;
}

ClanManager::Result ClanManager::banPlayer(const std::string &founderNick,
                                           const std::string &targetNick)
{
    std::lock_guard<std::mutex> lock(mutex);

    Clan *clan = findClanOfMemberUnlocked(founderNick);
    if (clan == nullptr)
        return Result::NOT_A_MEMBER;
    if (clan->getFounderNick() != founderNick)
        return Result::NOT_FOUNDER;

    bool wasMember = clan->isMember(targetNick);

    clan->banPlayer(targetNick);

    clanArchive.save(*clan);
    // Si era miembro y lo baneamos, también pierde el clan en su registro.
    if (wasMember)
        characterArchive.updateClan(targetNick, "");

    return Result::OK;
}

ClanManager::Result ClanManager::kickMember(const std::string &founderNick,
                                            const std::string &targetNick)
{
    std::lock_guard<std::mutex> lock(mutex);

    Clan *clan = findClanOfMemberUnlocked(founderNick);
    if (clan == nullptr)
        return Result::NOT_A_MEMBER;
    if (clan->getFounderNick() != founderNick)
        return Result::NOT_FOUNDER;
    if (targetNick == founderNick)
        return Result::CANNOT_KICK_SELF;
    if (!clan->isMember(targetNick))
        return Result::NOT_A_MEMBER;

    clan->removeMember(targetNick);

    clanArchive.save(*clan);
    characterArchive.updateClan(targetNick, "");

    return Result::OK;
}

ClanManager::Result ClanManager::leaveClan(const std::string &nick)
{
    std::lock_guard<std::mutex> lock(mutex);

    Clan *clan = findClanOfMemberUnlocked(nick);
    if (clan == nullptr)
        return Result::NOT_A_MEMBER;
    if (clan->getFounderNick() == nick)
        return Result::FOUNDER_CANNOT_LEAVE;

    clan->removeMember(nick);

    clanArchive.save(*clan);
    characterArchive.updateClan(nick, "");

    return Result::OK;
}

std::optional<std::pair<std::string, bool>>
ClanManager::findClanInfoForMember(const std::string &nick) const
{
    std::lock_guard<std::mutex> lock(mutex);

    for (const auto &[key, clan] : clans)
    {
        if (clan.isMember(nick))
            return std::make_pair(clan.getName(), clan.getFounderNick() == nick);
    }
    return std::nullopt;
}

std::optional<ClanManager::ClanOverview>
ClanManager::getOverviewForFounder(const std::string &founderNick) const
{
    std::lock_guard<std::mutex> lock(mutex);

    for (const auto &[key, clan] : clans)
    {
        if (clan.getFounderNick() != founderNick)
            continue;

        ClanOverview overview;
        overview.clanName = clan.getName();
        overview.members.assign(clan.getMembers().begin(), clan.getMembers().end());
        overview.applicants.assign(clan.getApplicants().begin(), clan.getApplicants().end());
        return overview;
    }
    return std::nullopt;
}

void ClanManager::notifyPlayer(const std::string &nick, const std::string &text)
{
    GameManager *gm = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex);
        gm = gameManager;
    }
    if (gm == nullptr)
        return;

    auto clientId = gm->findOnlineClientByNick(nick);
    if (!clientId)
        return;

    gm->sendToClient(*clientId,
                     std::make_shared<const ChatNotificationMessage>(text, ChatMsgType::CLAN));
}

void ClanManager::notifyClan(const std::string &clanName,
                             const std::string &excludeNick,
                             const std::string &text)
{
    std::vector<std::string> members;
    {
        std::lock_guard<std::mutex> lock(mutex);
        Clan *clan = findClanByNameUnlocked(clanName);
        if (clan == nullptr)
            return;
        members.assign(clan->getMembers().begin(), clan->getMembers().end());
    }

    for (const auto &nick : members)
    {
        if (nick == excludeNick)
            continue;
        notifyPlayer(nick, text);
    }
}

void ClanManager::syncPlayerClanState(const std::string &nick)
{
    GameManager *gm;
    std::string clanName;
    bool isFounder = false;
    bool hasClan = false;

    {
        std::lock_guard<std::mutex> lock(mutex);
        gm = gameManager;
        Clan *clan = findClanOfMemberUnlocked(nick);
        if (clan != nullptr)
        {
            hasClan = true;
            clanName = clan->getName();
            isFounder = (clan->getFounderNick() == nick);
        }
    }

    if (gm == nullptr)
        return;

    gm->updatePlayerClanState(nick, hasClan ? clanName : std::string(), isFounder);
}
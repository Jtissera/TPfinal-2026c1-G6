#include "clan.h"

Clan::Clan(std::string name, std::string founderNick)
    : name(std::move(name)), founderNick(std::move(founderNick)) {}

bool Clan::isMember(const std::string &nick) const
{
    return members.find(nick) != members.end();
}

bool Clan::isApplicant(const std::string &nick) const
{
    return applicants.find(nick) != applicants.end();
}

bool Clan::isBanned(const std::string &nick) const
{
    return bannedPlayers.find(nick) != bannedPlayers.end();
}

bool Clan::isFull() const
{
    return members.size() >= MAX_MEMBERS;
}

bool Clan::addMember(const std::string &nick)
{
    if (isFull())
        return false;
    members.insert(nick);
    return true;
}

void Clan::removeMember(const std::string &nick)
{
    members.erase(nick);
}

void Clan::addApplicant(const std::string &nick)
{
    applicants.insert(nick);
}

void Clan::removeApplicant(const std::string &nick)
{
    applicants.erase(nick);
}

void Clan::banPlayer(const std::string &nick)
{
    applicants.erase(nick);
    members.erase(nick);
    bannedPlayers.insert(nick);
}
#pragma once

#include <cstddef>
#include <set>
#include <string>

class Clan
{
public:
    Clan() = default;
    Clan(std::string name, std::string founderNick, std::size_t maxMembers);

    const std::string &getName() const;
    const std::string &getFounderNick() const;
    const std::set<std::string> &getMembers() const;
    const std::set<std::string> &getApplicants() const;
    const std::set<std::string> &getBannedPlayers() const;

    bool isMember(const std::string &nick) const;
    bool isApplicant(const std::string &nick) const;
    bool isBanned(const std::string &nick) const;
    bool isFull() const;

    bool addMember(const std::string &nick);
    void removeMember(const std::string &nick);
    void addApplicant(const std::string &nick);
    void removeApplicant(const std::string &nick);
    void banPlayer(const std::string &nick);

private:
    std::string name;
    std::string founderNick;
    std::size_t maxMembers = 0;
    std::set<std::string> members;
    std::set<std::string> applicants;
    std::set<std::string> bannedPlayers;
};
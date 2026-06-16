#pragma once

#include <cstddef>
#include <set>
#include <string>

class Clan
{
public:
    static constexpr std::size_t MAX_MEMBERS = 16;

    Clan() = default;
    Clan(std::string name, std::string founderNick);

    const std::string &getName() const { return name; }
    const std::string &getFounderNick() const { return founderNick; }

    bool isMember(const std::string &nick) const;
    bool isApplicant(const std::string &nick) const;
    bool isBanned(const std::string &nick) const;
    bool isFull() const;

    bool addMember(const std::string &nick);
    void removeMember(const std::string &nick);

    void addApplicant(const std::string &nick);
    void removeApplicant(const std::string &nick);

    void banPlayer(const std::string &nick);

    const std::set<std::string> &getMembers() const { return members; }
    const std::set<std::string> &getApplicants() const { return applicants; }
    const std::set<std::string> &getBannedPlayers() const { return bannedPlayers; }

private:
    std::string name;
    std::string founderNick;
    std::set<std::string> members;
    std::set<std::string> applicants;
    std::set<std::string> bannedPlayers;
};
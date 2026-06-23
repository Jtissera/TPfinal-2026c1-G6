#pragma once

#include "BinaryArchiveFile.h"

#include <optional>
#include <string>

struct CharacterRecord
{
    char name[32]     = {};
    char race[32]     = {};
    char cls[32]      = {};
    char clanName[32] = {};
};
static_assert(sizeof(CharacterRecord) == 128, "CharacterRecord size mismatch");

class CharacterArchive
{
public:
    CharacterArchive(const std::string &datPath, const std::string &indexPath);

    bool save(const std::string &name, const std::string &race, const std::string &cls);
    bool exists(const std::string &name) const;
    std::optional<CharacterRecord> load(const std::string &name) const;
    bool updateClan(const std::string &name, const std::string &clanName);

private:
    BinaryArchiveFile<std::string> archive_;
};
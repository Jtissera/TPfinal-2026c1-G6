#pragma once
#include "npcStats.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <toml++/toml.hpp>

class NpcRepository
{
public:
    explicit NpcRepository(const toml::table &config);

    const NpcStats &get(const std::string &typeName) const;
    bool exists(const std::string &typeName) const;

private:
    std::map<std::string, NpcStats> npcs;
    const toml::table &config;

    NpcStats parse(const std::string &typeName, const toml::table &entry) const;
    float zoneMultiplier(const std::string &zoneKey,
                         const std::string &multiplierKey) const;
};

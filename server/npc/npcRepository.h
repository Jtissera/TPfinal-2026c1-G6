#pragma once
#include "npcStats.h"
#include <map>
#include <string>
#include <toml++/toml.hpp>
#include <stdexcept>

class NpcRepository
{
public:
    explicit NpcRepository(const toml::table &config);
    const NpcStats &get(const std::string &typeName) const;
    bool exists(const std::string &typeName) const;

private:
    std::map<std::string, NpcStats> npcs;
    NpcStats parse(const std::string &name, const toml::table &entry) const;
    const toml::table &config;
};


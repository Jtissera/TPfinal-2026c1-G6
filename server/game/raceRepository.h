#pragma once
#include <map>
#include <string>
#include "../../../common/game/raceStats.h"
#include <toml++/toml.h>

class RaceRepository {
public:
    explicit RaceRepository(const toml::table& config);
    const RaceStats& get(const std::string& raceName) const;
    bool exists(const std::string& raceName) const;

private:
    std::map<std::string, RaceStats> races;
    RaceStats parse(const std::string& name, const toml::table& entry) const;
};
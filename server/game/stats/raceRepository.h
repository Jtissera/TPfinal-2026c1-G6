#pragma once

#include "raceStats.h"
#include <map>
#include <string>
#include <stdexcept>
#include <toml++/toml.hpp>

class RaceRepository
{
public:
  explicit RaceRepository(const toml::table &config);

  const RaceStats &get(const std::string &raceName) const;
  bool exists(const std::string &raceName) const;

private:
  std::map<std::string, RaceStats> races;

  RaceStats parse(const std::string &name, const toml::table &entry) const;
};
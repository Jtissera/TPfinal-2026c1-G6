// server/game/repositories/raceRepository.cpp
#include "raceRepository.h"
#include <stdexcept>

RaceRepository::RaceRepository(const toml::table& config) {
    const auto* racesSection = config.get_as<toml::table>("races");
    if (!racesSection)
        throw std::runtime_error("RaceRepository: falta la seccion [races] en el TOML");

    for (const auto& [key, value] : *racesSection) {
        const auto* entry = value.as_table();
        if (!entry) continue;
        std::string name(key.str());
        races[name] = parse(name, *entry);
    }
}

const RaceStats& RaceRepository::get(const std::string& raceName) const {
    auto it = races.find(raceName);
    if (it == races.end())
        throw std::out_of_range("RaceRepository: raza desconocida '" + raceName + "'");
    return it->second;
}

bool RaceRepository::exists(const std::string& raceName) const {
    return races.count(raceName) > 0;
}

RaceStats RaceRepository::parse(const std::string& name, const toml::table& entry) const {
    RaceStats stats;
    stats.name         = name;
    stats.health       = entry["health"].value_or<float>(1.0f);
    stats.mana         = entry["mana"].value_or<float>(1.0f);
    stats.recovery     = entry["recovery"].value_or<float>(1.0f);
    stats.constitution = entry["constitution"].value_or<uint8_t>(10);
    stats.intelligence = entry["intelligence"].value_or<uint8_t>(10);
    stats.strength     = entry["strength"].value_or<uint8_t>(10);
    stats.agility      = entry["agility"].value_or<uint8_t>(10);
    return stats;
}
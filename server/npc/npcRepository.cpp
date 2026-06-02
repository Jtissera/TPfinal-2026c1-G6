#include "npcRepository.h"
#include <stdexcept>

NpcRepository::NpcRepository(const toml::table &config) : config(config)
{
    const auto *section = config.get_as<toml::table>("npcs");

    if (!section)
        throw std::runtime_error("NpcRepository: falta [npcs] en el TOML");

    for (const auto &[key, value] : *section)
    {
        const auto *entry = value.as_table();
        if (!entry)
            continue;
        std::string name(key.str());
        npcs[name] = parse(name, *entry);
    }
}

const NpcStats &NpcRepository::get(const std::string &typeName) const
{
    auto it = npcs.find(typeName);
    if (it == npcs.end())
        throw std::out_of_range("NpcRepository: tipo desconocido '" + typeName + "'");
    return it->second;
}

bool NpcRepository::exists(const std::string &typeName) const
{
    return npcs.count(typeName) > 0;
}

NpcStats NpcRepository::parse(const std::string &name, const toml::table &entry) const
{
    NpcStats stats;
    stats.typeName = name;
    stats.maxHp = entry["hp"].value_or<int16_t>(50);
    stats.damageMin = entry["damage_min"].value_or<uint16_t>(1);
    stats.damageMax = entry["damage_max"].value_or<uint16_t>(3);
    stats.level = entry["level"].value_or<uint8_t>(1);
    stats.agility = entry["agility"].value_or<uint8_t>(5);
    stats.strength = entry["strength"].value_or<uint8_t>(5);
    stats.detectionRange = entry["detection_range"].value_or<int>(5);
    stats.homeRange = entry["home_range"].value_or<int>(10);
    stats.attackCooldownMs = entry["attack_cooldown_ms"].value_or<uint32_t>(1000);
    stats.moveCooldownMs = entry["move_cooldown_ms"].value_or<uint32_t>(500);

    if (const auto *arr = entry["zones"].as_array())
        for (const auto &z : *arr)
            if (auto s = z.value<std::string>())
                stats.zones.push_back(*s);

    ZoneType homeZone = ZoneType::COMBAT;
    float goldMult = 1.0f, xpMult = 1.0f, itemMult = 1.0f;

    if (!stats.zones.empty())
    {
        const std::string &z = stats.zones[0];
        if (z == "CAVERN")
        {
            homeZone = ZoneType::CAVERN;
            goldMult = config["cavern"]["gold_multiplier"].value_or(1.5f);
            xpMult = config["cavern"]["xp_multiplier"].value_or(1.5f);
            itemMult = config["cavern"]["item_multiplier"].value_or(1.5f);
        }
        else if (z == "DUNGEON")
        {
            homeZone = ZoneType::DUNGEON;
            goldMult = config["dungeon"]["gold_multiplier"].value_or(3.0f);
            xpMult = config["dungeon"]["xp_multiplier"].value_or(2.5f);
            itemMult = config["dungeon"]["item_multiplier"].value_or(2.5f);
        }
    }

    stats.homeZone = homeZone;
    stats.goldMultiplier = goldMult;
    stats.xpMultiplier = xpMult;
    stats.itemMultiplier = itemMult;

    return stats;
}
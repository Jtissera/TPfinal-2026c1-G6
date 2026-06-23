#include "npcRepository.h"

namespace
{

    ZoneType zoneTypeFromString(const std::string &s)
    {
        if (s == "CAVERN")
            return ZoneType::CAVERN;
        if (s == "DUNGEON")
            return ZoneType::DUNGEON;
        if (s == "DESERT")
            return ZoneType::DESERT;
        return ZoneType::COMBAT;
    }

}

NpcRepository::NpcRepository(const toml::table &config)
    : config(config)
{
    const toml::table *section = config.get_as<toml::table>("npcs");

    if (!section)
    {
        throw std::runtime_error("NpcRepository: missing [npcs] section in TOML");
    }

    for (const auto &[key, value] : *section)
    {
        const toml::table *entry = value.as_table();
        if (!entry)
        {
            continue;
        }
        std::string name(key.str());
        npcs[name] = parse(name, *entry);
    }
}

const NpcStats &NpcRepository::get(const std::string &typeName) const
{
    std::map<std::string, NpcStats>::const_iterator it = npcs.find(typeName);
    if (it == npcs.end())
    {
        throw std::out_of_range("NpcRepository: unknown type '" + typeName + "'");
    }
    return it->second;
}

bool NpcRepository::exists(const std::string &typeName) const
{
    return npcs.count(typeName) > 0;
}

float NpcRepository::zoneMultiplier(const std::string &zoneKey,
                                    const std::string &multiplierKey) const
{
    return config[zoneKey][multiplierKey].value_or(1.0f);
}

NpcStats NpcRepository::parse(const std::string &typeName, const toml::table &entry) const
{
    NpcStats stats;
    stats.typeName = typeName;
    stats.type = npcTypeFromKey(typeName);

    if (stats.type == NpcType::NONE)
    {
        throw std::runtime_error("NpcRepository: unknown npc typeName: " + typeName);
    }

    stats.name = npcTypeName(stats.type);
    if (std::optional<std::string> nameOverride = entry["name"].value<std::string>())
    {
        stats.name = *nameOverride;
    }

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
    stats.hostile = entry["hostile"].value_or<bool>(true);

    if (const toml::array *arr = entry["zones"].as_array())
    {
        for (const toml::node &z : *arr)
        {
            if (std::optional<std::string> s = z.value<std::string>())
            {
                stats.zones.push_back(*s);
            }
        }
    }

    ZoneType homeZone = ZoneType::COMBAT;
    float goldMult = 1.0f;
    float xpMult = 1.0f;
    float itemMult = 1.0f;

    if (!stats.zones.empty())
    {
        const std::string &zoneStr = stats.zones[0];
        homeZone = zoneTypeFromString(zoneStr);

        std::string zoneKey = zoneStr;
        for (char &c : zoneKey)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }

        goldMult = zoneMultiplier(zoneKey, "gold_multiplier");
        xpMult = zoneMultiplier(zoneKey, "xp_multiplier");
        itemMult = zoneMultiplier(zoneKey, "item_multiplier");
    }

    stats.homeZone = homeZone;
    stats.goldMultiplier = goldMult;
    stats.xpMultiplier = xpMult;
    stats.itemMultiplier = itemMult;

    return stats;
}
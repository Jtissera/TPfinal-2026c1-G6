#include "gameFormulas.h"

GameFormulas::GameFormulas(const toml::table &config)
    : config(config), zoneNaming()
{
}

int16_t GameFormulas::calcMaxHp(const RaceStats &race,
                                const ClassStats &cls,
                                uint8_t level) const
{
  const int16_t baseHealth =
      config["player"]["base_health"].value_or<int16_t>(100);

  const float constitutionBonus =
      static_cast<float>(race.constitution) * static_cast<float>(level);

  const float result =
      baseHealth * race.health * cls.health + constitutionBonus;

  return static_cast<int16_t>(std::max(1.0f, result));
}

int16_t GameFormulas::calcMaxMana(const RaceStats &race,
                                  const ClassStats &cls,
                                  uint8_t level) const
{
  if (!cls.canUseMagic)
  {
    return 0;
  }

  const int16_t baseMana =
      config["player"]["base_mana"].value_or<int16_t>(50);

  const float intelligenceBonus =
      static_cast<float>(race.intelligence) * static_cast<float>(level);

  const float result =
      baseMana * race.mana * cls.mana + intelligenceBonus;

  return static_cast<int16_t>(std::max(0.0f, result));
}

uint32_t GameFormulas::calcMaxGold(uint8_t level) const
{
  return static_cast<uint32_t>(100.0f * std::pow(level, 1.1f));
}

uint32_t GameFormulas::calcExpLimit(uint8_t level) const
{
  return static_cast<uint32_t>(1000.0f * std::pow(level, 1.8f));
}

uint32_t GameFormulas::calcExpOnHit(int16_t damage,
                                    uint8_t attackerLevel,
                                    uint8_t targetLevel) const
{
  const int diff =
      static_cast<int>(targetLevel) - static_cast<int>(attackerLevel) + 10;

  if (diff <= 0)
  {
    return 0;
  }

  return static_cast<uint32_t>(damage * diff);
}

uint32_t GameFormulas::calcExpOnKill(int16_t targetMaxHp,
                                     uint8_t attackerLevel,
                                     uint8_t targetLevel) const
{
  const int diff =
      static_cast<int>(targetLevel) - static_cast<int>(attackerLevel) + 10;

  if (diff <= 0)
  {
    return 0;
  }

  const float factor = static_cast<float>(std::rand() % 101) / 1000.0f;

  return static_cast<uint32_t>(
      factor * static_cast<float>(targetMaxHp) * static_cast<float>(diff));
}

uint32_t GameFormulas::calcNpcGoldDrop(int16_t npcMaxHp) const
{
  const float factor = static_cast<float>(std::rand() % 201) / 1000.0f;

  return static_cast<uint32_t>(
      factor * static_cast<float>(npcMaxHp));
}

uint32_t GameFormulas::calcExcessGold(uint32_t gold,
                                      uint32_t maxGold) const
{
  if (gold <= maxGold)
  {
    return 0;
  }

  return gold - maxGold;
}

float GameFormulas::calcHpRegen(const RaceStats &race,
                                float deltaSeconds) const
{
  return race.recovery * deltaSeconds;
}

float GameFormulas::calcManaRegen(const RaceStats &race,
                                  float deltaSeconds) const
{
  return race.recovery * deltaSeconds;
}

float GameFormulas::calcManaRegenMeditating(const ClassStats &cls,
                                            const RaceStats &race,
                                            float deltaSeconds) const
{
  return cls.meditation * race.intelligence * deltaSeconds;
}

std::string GameFormulas::rollNpcDrop(ZoneType zone) const
{
  const double nothingChance =
      config["npc"]["drop_chance_nothing"].value_or<double>(0.0);
  const double goldChance =
      config["npc"]["drop_chance_gold"].value_or<double>(0.7);

  const int roll = std::rand() % 100;
  const double rollPct = static_cast<double>(roll) / 100.0;

  if (rollPct < nothingChance)
  {
    return "";
  }

  if (rollPct < nothingChance + goldChance)
  {
    return "GOLD";
  }

  const std::string poolKey = zoneNaming.toPoolPrefix(zone) + "_pool";
  const toml::array *poolArray = config["drops"][poolKey].as_array();

  if (poolArray == nullptr || poolArray->empty())
  {
    return "";
  }

  const std::size_t index =
      static_cast<std::size_t>(std::rand()) % poolArray->size();

  return (*poolArray)[index].value_or<std::string>("");
}
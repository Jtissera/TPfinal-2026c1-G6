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
  const double base =
      config["gold"]["gold_level_base"].value_or<double>(100.0);

  const double exponent =
      config["gold"]["gold_level_exponent"].value_or<double>(1.1);

  return static_cast<uint32_t>(base * std::pow(level, exponent));
}

uint32_t GameFormulas::calcExpLimit(uint8_t level) const
{
  const double base =
      config["experience"]["base"].value_or<double>(1000.0);

  const double exponent =
      config["experience"]["exponent"].value_or<double>(1.8);

  return static_cast<uint32_t>(base * std::pow(level, exponent));
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

uint32_t GameFormulas::calcExpOnKill(int16_t targetMaxHp, uint8_t attackerLevel, uint8_t targetLevel) const
{
  const int diff =
      static_cast<int>(targetLevel) - static_cast<int>(attackerLevel) + 10;

  if (diff <= 0)
  {
    return 0;
  }

  const double maxBonus =
      config["combat"]["npc_xp_bonus_max"].value_or<double>(0.10);

  const double random01 =
      static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);

  const double factor = random01 * maxBonus;

  return static_cast<uint32_t>(
      factor * static_cast<double>(targetMaxHp) * static_cast<double>(diff));
}

uint32_t GameFormulas::calcNpcGoldDrop(int16_t npcMaxHp) const
{
  const double maxFactor =
      config["combat"]["npc_gold_drop_max"].value_or<double>(0.35);

  const double random01 =
      static_cast<double>(std::rand()) / static_cast<double>(RAND_MAX);

  const double factor = random01 * maxFactor;

  const uint32_t gold =
      static_cast<uint32_t>(factor * static_cast<double>(npcMaxHp));

  return std::max<uint32_t>(1, gold);
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
      config["npc"]["drop_chance_nothing"].value_or<double>(0.70);

  const double goldChance =
      config["npc"]["drop_chance_gold"].value_or<double>(0.20);

  const double potionChance =
      config["npc"]["drop_chance_potion"].value_or<double>(0.05);

  const double itemChance =
      config["npc"]["drop_chance_item"].value_or<double>(0.05);

  const int roll = std::rand() % 100;
  const double rollPct = static_cast<double>(roll) / 100.0;

  double cumulativeChance = nothingChance;

  if (rollPct < cumulativeChance)
  {
    return "";
  }

  cumulativeChance += goldChance;
  if (rollPct < cumulativeChance)
  {
    return "GOLD";
  }

  cumulativeChance += potionChance;
  if (rollPct < cumulativeChance)
  {
    return (std::rand() % 2 == 0) ? "pocion_vida" : "pocion_mana";
  }

  cumulativeChance += itemChance;
  if (rollPct < cumulativeChance)
  {

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

  return "";
}
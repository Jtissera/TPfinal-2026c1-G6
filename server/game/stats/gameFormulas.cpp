#include "gameFormulas.h"
#include <cmath>
#include <cstdlib>

int16_t GameFormulas::calcMaxHp(const RaceStats &race, const ClassStats &cls,
                                uint8_t level) const {

  return static_cast<int16_t>(race.constitution * cls.health * race.health *
                              level);
}

int16_t GameFormulas::calcMaxMana(const RaceStats &race, const ClassStats &cls,
                                  uint8_t level) const {

  if (!cls.canUseMagic)
    return 0;

  return static_cast<int16_t>(race.intelligence * cls.mana * race.mana * level);
}

uint32_t GameFormulas::calcMaxGold(uint8_t level) const {
  // OroMax = 100 * Nivel^1.1
  return static_cast<uint32_t>(100.0f * std::pow(level, 1.1f));
}

uint32_t GameFormulas::calcExpLimit(uint8_t level) const {
  // Limite = 1000 * Nivel^1.8
  return static_cast<uint32_t>(1000.0f * std::pow(level, 1.8f));
}

uint32_t GameFormulas::calcExpOnHit(int16_t damage, uint8_t attackerLevel,
                                    uint8_t targetLevel) const {
  // Exp = Daño * max(NivelDelOtro - Nivel + 10, 0)
  int diff =
      static_cast<int>(targetLevel) - static_cast<int>(attackerLevel) + 10;
  if (diff <= 0)
    return 0;
  return static_cast<uint32_t>(damage * diff);
}

uint32_t GameFormulas::calcExpOnKill(int16_t targetMaxHp, uint8_t attackerLevel,
                                     uint8_t targetLevel) const {
  // Exp = rand(0, 0.1) * VidaMaxDelOtro * max(NivelDelOtro - Nivel + 10, 0)
  int diff =
      static_cast<int>(targetLevel) - static_cast<int>(attackerLevel) + 10;
  if (diff <= 0)
    return 0;
  float factor = (std::rand() % 101) / 1000.0f; // rand(0, 0.1)
  return static_cast<uint32_t>(factor * targetMaxHp * diff);
}

uint32_t GameFormulas::calcNpcGoldDrop(int16_t npcMaxHp) const {
  // Oro = rand(0, 0.2) * VidaMaxNPC
  float factor = (std::rand() % 201) / 1000.0f;
  return static_cast<uint32_t>(factor * npcMaxHp);
}

uint32_t GameFormulas::calcExcessGold(uint32_t gold, uint32_t maxGold) const {
  // El jugador puede tener hasta maxGold * 1.5

  if (gold <= maxGold)
    return 0;
  return gold - maxGold;
}

float GameFormulas::calcHpRegen(const RaceStats &race,
                                float deltaSeconds) const {
  return race.recovery * deltaSeconds;
}

float GameFormulas::calcManaRegen(const RaceStats &race,
                                  float deltaSeconds) const {
  // Mana = FRazaRecuperacion * segundos
  return race.recovery * deltaSeconds;
}

float GameFormulas::calcManaRegenMeditating(const ClassStats &cls,
                                            const RaceStats &race,
                                            float deltaSeconds) const {

  return cls.meditation * race.intelligence * deltaSeconds;
}
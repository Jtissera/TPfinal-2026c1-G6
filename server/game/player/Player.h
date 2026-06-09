#pragma once

#include <cstdint>
#include <string>
#include <toml++/toml.hpp>
#include <vector>

#include "../../../common/dtos/gameTypes.h"
#include "../stats/classRepository.h"
#include "../stats/raceRepository.h"
#include "combatant.h"
#include "inventory.h"
#include "playerState.h"

class Player : public Combatant
{
public:
  Player(uint32_t clientId, std::string name, const RaceStats &race,
         const ClassStats &cls, int16_t maxHp, int16_t maxMana,
         const toml::table &config);

  bool isAlive() const override;
  bool isGhost() const;
  bool isMeditating() const;

  void takeDamage(int16_t dmg) override;
  void heal(int16_t amount);
  void restoreMana(int16_t amount);
  bool spendMana(int16_t cost);
  void addGold(uint32_t amount);

  uint32_t die(uint32_t safeGold);
  void resurrect(int tileX, int tileY);

  void addExperience(uint32_t exp, uint32_t expLimit, int16_t newMaxHp,
                     int16_t newMaxMana);
  bool checkAndClearLevelUp();
  void levelUp(int16_t newMaxHp, int16_t newMaxMana);

  void startMeditating();
  void stopMeditating();
  void toggleInfiniteHp();
  void toggleInfiniteMana();
  bool hasInfiniteHp() const { return infiniteHp; }
  bool hasInfiniteMana() const { return infiniteMana; }
  void tick(float hpGained, float manaGained);

  std::vector<Item> purgeInventoryOnDeath();

  void setTilePos(int tx, int ty);
  void setPixelPos(float tx, float ty);

  int getTileX() const override { return static_cast<int>(pixelX) / TILE_SIZE; }
  int getTileY() const override { return static_cast<int>(pixelY) / TILE_SIZE; }
  uint32_t getId() const override;
  uint32_t getClientId() const;
  const std::string &getName() const;
  uint8_t getLevel() const override;
  int16_t getHp() const override;
  int16_t getMaxHp() const override;
  int16_t getMana() const;
  int16_t getMaxMana() const;
  uint32_t getExp() const;
  uint32_t getGold() const;
  uint8_t getAgility() const override;
  uint8_t getStrength() const override;
  int getAttackRange() const override;

  // Devuelve la posición X real en píxeles.
  float getPixelX() const;

  // Devuelve la posición Y real en píxeles.
  float getPixelY() const;

  uint16_t getWeaponDamageMin() const override;
  uint16_t getWeaponDamageMax() const override;
  uint16_t getArmorDefenseMin() const override;
  uint16_t getArmorDefenseMax() const override;
  uint16_t getHelmetDefenseMin() const override;
  uint16_t getHelmetDefenseMax() const override;
  uint16_t getShieldDefenseMin() const override;
  uint16_t getShieldDefenseMax() const override;

  void restoreFullHpAndMana();
  void spendGold(uint32_t amount);

  bool isResurrecting() const { return resurrecting; }
  void startResurrection() { resurrecting = true; }
  void stopResurrection() { resurrecting = false; }
  bool canInteract() const { return !isGhost() && !resurrecting; }

  const RaceStats &getRace() const;
  const ClassStats &getCls() const;

  Inventory &getInventory();
  const Inventory &getInventory() const;

  Player(const Player &) = delete;
  Player &operator=(const Player &) = delete;
  Player(Player &&) = default;
  Player &operator=(Player &&) = default;

private:
  bool resurrecting = false;

  uint32_t clientId;
  std::string name;
  const RaceStats &race;
  const ClassStats &cls;
  const int TILE_SIZE = 96;

  int tileX = 0;
  int tileY = 0;

  float pixelX;
  float pixelY;

  uint8_t level = 1;
  int16_t hp = 0;
  int16_t maxHp = 0;
  int16_t mana = 0;
  int16_t maxMana = 0;
  uint32_t gold = 0;
  uint32_t experience = 0;

  int rangedAttackRange;

  PlayerState state = PlayerState::ALIVE;
  bool didLevelUp = false;
  bool infiniteHp = false;
  bool infiniteMana = false;

  Inventory inventory;
};

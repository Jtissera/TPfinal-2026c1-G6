#include "Player.h"
#include <algorithm>

#include "Player.h"
#include <algorithm>

Player::Player(uint32_t clientId, std::string name, const RaceStats &race,
               const ClassStats &cls, int16_t maxHp, int16_t maxMana,
               const toml::table &config)
    : clientId(clientId),
      name(std::move(name)),
      race(race),
      cls(cls),
      maxHp(maxHp),
      maxMana(maxMana),
      hp(maxHp),
      mana(maxMana),
      tileSize(config["world"]["tile_size"].value_or(96)),
      rangedAttackRange(config["combat"]["ranged_attack_range"].value_or(10)),
      inventory(config["player"]["max_inventory_items"].value_or<std::size_t>(20))
{
}

bool Player::isAlive() const { return state == PlayerState::ALIVE; }
bool Player::isGhost() const { return state == PlayerState::DEAD; }
bool Player::isMeditating() const { return state == PlayerState::MEDITATING; }
bool Player::hasInfiniteHp() const { return infiniteHp; }
bool Player::hasInfiniteMana() const { return infiniteMana; }
bool Player::isResurrecting() const { return resurrecting; }
bool Player::canInteract() const { return !isGhost() && !resurrecting; }
bool Player::isClanFounder() const { return clanFounder; }
bool Player::hasReceivedInitialInventory() const { return initialInventoryGiven; }

void Player::startResurrection() { resurrecting = true; }
void Player::stopResurrection() { resurrecting = false; }
void Player::markInitialInventoryGiven() { initialInventoryGiven = true; }
void Player::setClanFounder(bool founder) { clanFounder = founder; }

const std::string &Player::getClanName() const { return clanName; }
void Player::setClanName(std::string n) { clanName = std::move(n); }

int Player::getTileX() const { return static_cast<int>(pixelX) / tileSize; }
int Player::getTileY() const { return static_cast<int>(pixelY) / tileSize; }

void Player::takeDamage(int16_t dmg)
{
  if (isGhost())
    return;

  if (infiniteHp)
    return;

  hp = std::max<int16_t>(0, static_cast<int16_t>(hp - dmg));
}

void Player::heal(int16_t amount)
{
  if (!isAlive())
    return;
  hp = std::min(maxHp, static_cast<int16_t>(hp + amount));
}

void Player::restoreMana(int16_t amount)
{
  if (!cls.canUseMagic)
    return;
  mana = std::min(maxMana, static_cast<int16_t>(mana + amount));
}

bool Player::spendMana(int16_t cost)
{
  if (!cls.canUseMagic)
    return false;
  if (infiniteMana)
    return true;
  if (mana < cost)
    return false;
  mana -= cost;
  return true;
}

void Player::addGold(uint32_t amount) { gold += amount; }

void Player::addExperience(uint32_t exp, uint32_t expLimit, int16_t newMaxHp,
                           int16_t newMaxMana)
{
  experience += exp;
  while (experience >= expLimit)
  {
    experience -= expLimit;
    levelUp(newMaxHp, newMaxMana);
    didLevelUp = true;
  }
}

bool Player::checkAndClearLevelUp()
{
  bool was = didLevelUp;
  didLevelUp = false;
  return was;
}

void Player::levelUp(int16_t newMaxHp, int16_t newMaxMana)
{
  level++;
  maxHp = newMaxHp;
  maxMana = newMaxMana;
  hp = maxHp;
  mana = maxMana;
}

void Player::startMeditating()
{
  if (!isAlive() || !cls.canUseMagic)
    return;
  state = PlayerState::MEDITATING;
}

void Player::stopMeditating()
{
  if (isMeditating())
    state = PlayerState::ALIVE;
}

uint32_t Player::die(uint32_t safeGold)
{
  if (!isAlive())
    return 0;
  state = PlayerState::DEAD;
  hp = 0;
  mana = 0;

  if (gold > safeGold)
  {
    uint32_t excess = gold - safeGold;
    gold = safeGold;
    return excess;
  }
  return 0;
}

void Player::resurrect(int tx, int ty)
{
  state = PlayerState::ALIVE;
  setTilePos(tx, ty);
  hp = maxHp / 2;
  mana = cls.canUseMagic ? maxMana / 2 : 0;
}

bool Player::tick(float hpGained, float manaGained)
{
  if (!isAlive() && !isMeditating())
    return false;

  const int16_t prevHp = hp;
  const int16_t prevMana = mana;

  if (isAlive())
  {
    hpAccumulator += hpGained;
    if (hpAccumulator >= 1.0f)
    {
      int16_t wholeHp = static_cast<int16_t>(hpAccumulator);
      hpAccumulator -= static_cast<float>(wholeHp);
      heal(wholeHp);
    }
  }

  if (cls.canUseMagic && !infiniteMana)
  {
    manaAccumulator += manaGained;
    if (manaAccumulator >= 1.0f)
    {
      int16_t wholeMana = static_cast<int16_t>(manaAccumulator);
      manaAccumulator -= static_cast<float>(wholeMana);
      restoreMana(wholeMana);
    }
  }

  return (hp != prevHp || mana != prevMana);
}

std::vector<Item> Player::purgeInventoryOnDeath()
{
  return inventory.removeAllItems();
}

int Player::getAttackRange() const
{
  const Item *weapon = inventory.getEquipped(EquipSlot::HAND);
  if (weapon && weapon->stats.isRanged)
    return rangedAttackRange;
  return 1;
}

uint16_t Player::getWeaponDamageMin() const
{
  const Item *w = inventory.getEquipped(EquipSlot::HAND);

  if (w == nullptr)
  {
    return 2;
  }

  return w->stats.damageMin;
}

uint16_t Player::getWeaponDamageMax() const
{

  const Item *w = inventory.getEquipped(EquipSlot::HAND);

  if (w == nullptr)
  {
    return 4;
  }

  return w->stats.damageMax;
}

uint16_t Player::getArmorDefenseMin() const
{
  const Item *a = inventory.getEquipped(EquipSlot::ARMOR);
  return a ? a->stats.defenseMin : 0;
}
uint16_t Player::getArmorDefenseMax() const
{
  const Item *a = inventory.getEquipped(EquipSlot::ARMOR);
  return a ? a->stats.defenseMax : 0;
}
uint16_t Player::getHelmetDefenseMin() const
{
  const Item *h = inventory.getEquipped(EquipSlot::HELMET);
  return h ? h->stats.defenseMin : 0;
}
uint16_t Player::getHelmetDefenseMax() const
{
  const Item *h = inventory.getEquipped(EquipSlot::HELMET);
  return h ? h->stats.defenseMax : 0;
}
uint16_t Player::getShieldDefenseMin() const
{
  const Item *s = inventory.getEquipped(EquipSlot::SHIELD);
  return s ? s->stats.defenseMin : 0;
}
uint16_t Player::getShieldDefenseMax() const
{
  const Item *s = inventory.getEquipped(EquipSlot::SHIELD);
  return s ? s->stats.defenseMax : 0;
}

uint32_t Player::getId() const { return clientId; }
uint32_t Player::getClientId() const { return clientId; }
const std::string &Player::getName() const { return name; }
uint8_t Player::getLevel() const { return level; }
int16_t Player::getHp() const { return hp; }
int16_t Player::getMaxHp() const { return maxHp; }
int16_t Player::getMana() const { return mana; }
int16_t Player::getMaxMana() const { return maxMana; }
uint32_t Player::getExp() const { return experience; }
uint32_t Player::getGold() const { return gold; }
uint8_t Player::getAgility() const { return race.agility; }
uint8_t Player::getStrength() const { return race.strength; }

const RaceStats &Player::getRace() const { return race; }
const ClassStats &Player::getCls() const { return cls; }

Inventory &Player::getInventory() { return inventory; }
const Inventory &Player::getInventory() const { return inventory; }

void Player::toggleInfiniteHp()
{
  infiniteHp = !infiniteHp;
  if (infiniteHp && isAlive())
    hp = maxHp;
}

void Player::toggleInfiniteMana()
{
  infiniteMana = !infiniteMana;
  if (infiniteMana && cls.canUseMagic)
    mana = maxMana;
}

void Player::restoreFullHpAndMana()
{
  hp = maxHp;
  mana = maxMana;
}

void Player::spendGold(uint32_t amount)
{
  gold = (gold >= amount) ? gold - amount : 0;
}

float Player::getPixelX() const
{
  return pixelX;
}

float Player::getPixelY() const
{
  return pixelY;
}

void Player::setPixelPos(float x, float y)
{
  pixelX = x;
  pixelY = y;
  tileX = static_cast<int>(pixelX) / tileSize;
  tileY = static_cast<int>(pixelY) / tileSize;
}

void Player::setTilePos(int tx, int ty)
{
  tileX = tx;
  tileY = ty;

  pixelX = static_cast<float>(tx * tileSize);
  pixelY = static_cast<float>(ty * tileSize);
}

void Player::setClientId(uint32_t id) { clientId = id; }

void Player::setHp(int16_t newHp) { hp = newHp; }

void Player::setMana(int16_t newMana) { mana = newMana; }

void Player::setLevel(uint8_t newLevel) { level = newLevel; }

void Player::setGold(uint32_t newGold) { gold = newGold; }

void Player::setExp(uint32_t newExp) { experience = newExp; }

void Player::forceGhostState() { state = PlayerState::DEAD; }
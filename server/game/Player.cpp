#include "Player.h"
#include <algorithm>

Player::Player(uint32_t clientId,
               std::string name,
               const RaceStats&  race,
               const ClassStats& cls,
               int16_t maxHp,
               int16_t maxMana)
    : clientId(clientId)
    , name(std::move(name))
    , race(race)
    , cls(cls)
    , maxHp(maxHp)
    , maxMana(maxMana)
    , hp(maxHp)
    , mana(maxMana)
{}

bool Player::isAlive()      const { return state == PlayerState::ALIVE;      }
bool Player::isGhost()      const { return state == PlayerState::DEAD;       }
bool Player::isMeditating() const { return state == PlayerState::MEDITATING; }

void Player::takeDamage(int16_t dmg) {
    if (!isAlive()) return;
    hp = std::max<int16_t>(0, hp - dmg);
}

void Player::heal(int16_t amount) {
    if (!isAlive()) return;
    hp = std::min(maxHp, static_cast<int16_t>(hp + amount));
}

void Player::restoreMana(int16_t amount) {
    if (!cls.canUseMagic) return;
    mana = std::min(maxMana, static_cast<int16_t>(mana + amount));
}

bool Player::spendMana(int16_t cost) {
    if (!cls.canUseMagic) return false;
    if (mana < cost)      return false;
    mana -= cost;
    return true;
}

void Player::addGold(uint32_t amount) {
    gold += amount;
}

// Las fórmulas vienen de afuera — Player no sabe calcularlas
void Player::addExperience(uint32_t exp, uint32_t expLimit,
                            int16_t newMaxHp, int16_t newMaxMana) {
    experience += exp;
    if (experience >= expLimit) {
        experience -= expLimit;
        levelUp(newMaxHp, newMaxMana);
        didLevelUp = true;
    }
}

bool Player::checkAndClearLevelUp() {
    bool was   = didLevelUp;
    didLevelUp = false;
    return was;
}

void Player::levelUp(int16_t newMaxHp, int16_t newMaxMana) {
    level++;
    maxHp   = newMaxHp;
    maxMana = newMaxMana;
    hp      = maxHp;
    mana    = maxMana;
}

void Player::startMeditating() {
    if (!isAlive() || !cls.canUseMagic) return;
    state = PlayerState::MEDITATING;
}

void Player::stopMeditating() {
    if (isMeditating()) state = PlayerState::ALIVE;
}


uint32_t Player::die(uint32_t safeGold) {
    if (!isAlive()) return 0;
    state = PlayerState::DEAD;
    hp    = 0;

    if (gold > safeGold) {
        uint32_t excess = gold - safeGold;
        gold = safeGold;
        return excess;
    }
    return 0;
}

void Player::resurrect(int tx, int ty) {
    state = PlayerState::ALIVE;
    tileX = tx;
    tileY = ty;
    hp    = maxHp / 2;
    mana  = 0;
}

void Player::tick(float hpGained, float manaGained) {
    if (!isAlive() && !isMeditating()) return;
    if (isAlive())        heal(static_cast<int16_t>(hpGained));
    if (cls.canUseMagic)  restoreMana(static_cast<int16_t>(manaGained));
}

std::vector<Item> Player::purgeInventoryOnDeath() {
    return inventory.removeAllItems();
}

int Player::getAttackRange() const {
    const Item* weapon = inventory.getEquipped(EquipSlot::HAND);
    if (weapon && weapon->stats.isRanged) return 10; // toml
    return 1;
}

uint16_t Player::getWeaponDamageMin() const {
    const Item* w = inventory.getEquipped(EquipSlot::HAND);
    return w ? w->stats.damageMin : 0;
}
uint16_t Player::getWeaponDamageMax() const {
    const Item* w = inventory.getEquipped(EquipSlot::HAND);
    return w ? w->stats.damageMax : 1;
}
uint16_t Player::getArmorDefenseMin() const {
    const Item* a = inventory.getEquipped(EquipSlot::ARMOR);
    return a ? a->stats.defenseMin : 0;
}
uint16_t Player::getArmorDefenseMax() const {
    const Item* a = inventory.getEquipped(EquipSlot::ARMOR);
    return a ? a->stats.defenseMax : 0;
}
uint16_t Player::getHelmetDefenseMin() const {
    const Item* h = inventory.getEquipped(EquipSlot::HELMET);
    return h ? h->stats.defenseMin : 0;
}
uint16_t Player::getHelmetDefenseMax() const {
    const Item* h = inventory.getEquipped(EquipSlot::HELMET);
    return h ? h->stats.defenseMax : 0;
}
uint16_t Player::getShieldDefenseMin() const {
    const Item* s = inventory.getEquipped(EquipSlot::SHIELD);
    return s ? s->stats.defenseMin : 0;
}
uint16_t Player::getShieldDefenseMax() const {
    const Item* s = inventory.getEquipped(EquipSlot::SHIELD);
    return s ? s->stats.defenseMax : 0;
}
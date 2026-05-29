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
    , hitbox{32, 32, 64, 96} 
{}

bool Player::isAlive()      const { return state == PlayerState::ALIVE; }
bool Player::isGhost()      const { return state == PlayerState::DEAD; }
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

void Player::addExperience(uint32_t exp) {

experience += exp;

    uint32_t currentLimit = formulas.calcExpLimit(level);

    while (experience >= currentLimit) {
        experience -= currentLimit;
        
        int16_t newMaxHp   = formulas.calcMaxHp(race, cls, level + 1);
        int16_t newMaxMana = formulas.calcMaxMana(race, cls, level + 1);
        
        levelUp(newMaxHp, newMaxMana);
        didLevelUp = true;  
        

        currentLimit = formulas.calcExpLimit(level);
    }

    this->limit = currentLimit;

}

bool Player::checkAndClearLevelUp() {
    bool wasLevelUp = didLevelUp; 
    didLevelUp = false;           
    return wasLevelUp;            
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

uint32_t Player::die() {
    state = PlayerState::DEAD;
    hp    = 0;

    uint32_t expLoss = formulas.calcExpLimit(level) / 10;
    experience = (experience > expLoss) ? experience - expLoss : 0;
    uint32_t safeGold = formulas.calcMaxGold(level);

    if (gold > safeGold) {
        uint32_t excess = gold - safeGold;
        this->gold = safeGold; 
        return excess;
    }

    return 0;
}

std::vector<Item> Player::purgeInventoryOnDeath() {
    return inventory.removeAllItems();
}

void Player::resurrect(int spawnX, int spawnY) {
    state = PlayerState::ALIVE;
    x = spawnX;
    y = spawnY;
    hp    = maxHp / 2;
    mana  = 0;
}

void Player::tick(float hpGained, float manaGained) {
    if (!isAlive() && !isMeditating()) return;

    if (isAlive())
        heal(static_cast<int16_t>(hpGained));

    if (cls.canUseMagic)
        restoreMana(static_cast<int16_t>(manaGained));
}

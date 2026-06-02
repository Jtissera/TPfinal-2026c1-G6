#include "Player.h"
#include <algorithm>

Player::Player(uint32_t clientId, std::string name,
               const RaceStats& race, const ClassStats& cls,
               int16_t maxHp, int16_t maxMana,
               const toml::table& config)
    : clientId(clientId)
    , name(std::move(name))
    , race(race)
    , cls(cls)
    , tileSize(config["world"]["tile_size"].value_or(96))
    , hitboxW(config["world"]["player_hitbox_w"].value_or(32.0f))
    , hitboxH(config["world"]["player_hitbox_h"].value_or(16.0f))
    , meleeRangePx(config["world"]["melee_range_px"].value_or(144.0f))
    , rangedAttackRangePx(config["world"]["ranged_attack_range_px"].value_or(960.0f))
    , stepPx(config["world"]["player_step_px"].value_or(96.0f))
    , maxHp(maxHp)
    , maxMana(maxMana)
    , hp(maxHp)
    , mana(maxMana)
    , inventory(config)
{}

// --- Combatant ---

int Player::getTileX() const {
    return static_cast<int>(pixelX) / tileSize;
}

int Player::getTileY() const {
    return static_cast<int>(pixelY) / tileSize;
}

bool Player::isAlive() const {
    return state == PlayerState::ALIVE;
}

bool Player::isGhost() const {
    return state == PlayerState::DEAD;
}

bool Player::isMeditating() const {
    return state == PlayerState::MEDITATING;
}

bool Player::isResurrecting() const {
    return state == PlayerState::RESURRECTING;
}

float Player::getAttackRangePx() const {
    const Item* weapon = inventory.getEquipped(EquipSlot::HAND);
    if (weapon && weapon->stats.isRanged)
        return rangedAttackRangePx;
    return meleeRangePx;
}

// --- Movimiento ---

void Player::setPixelPos(float px, float py) {
    pixelX = px;
    pixelY = py;
}

// --- Combate ---

void Player::takeDamage(int16_t dmg) {
    if (!isAlive())   return;
    if (infiniteHp)   return;
    hp = std::max<int16_t>(0, hp - dmg);
    if (hp == 0)
        state = PlayerState::DEAD;
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
    if (infiniteMana)     return true;
    if (mana < cost)      return false;
    mana -= cost;
    return true;
}

void Player::addGold(uint32_t amount) { gold += amount; }

void Player::spendGold(uint32_t amount) {
    gold = (gold >= amount) ? gold - amount : 0;
}

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
    bool was = didLevelUp;
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

// --- Estado ---

void Player::startMeditating() {
    if (!isAlive() || !cls.canUseMagic) return;
    state = PlayerState::MEDITATING;
}

void Player::stopMeditating() {
    if (isMeditating()) state = PlayerState::ALIVE;
}

void Player::startResurrection() {
    state = PlayerState::RESURRECTING;
}

void Player::stopResurrection() {
    // GameWorld llama a resurrect() justo despues
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

void Player::resurrect(float px, float py) {
    state  = PlayerState::ALIVE;
    pixelX = px;
    pixelY = py;
    hp     = maxHp / 2;
    mana   = 0;
}

void Player::tick(float hpGained, float manaGained) {
    if (!isAlive() && !isMeditating()) return;
    if (isAlive())
        heal(static_cast<int16_t>(hpGained));
    if (cls.canUseMagic && !infiniteMana)
        restoreMana(static_cast<int16_t>(manaGained));
}

std::vector<Item> Player::purgeInventoryOnDeath() {
    return inventory.removeAllItems();
}

// --- Items equipados ---

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

// --- Cheats ---

void Player::toggleInfiniteHp() {
    infiniteHp = !infiniteHp;
    if (infiniteHp && isAlive()) hp = maxHp;
}

void Player::toggleInfiniteMana() {
    infiniteMana = !infiniteMana;
    if (infiniteMana && cls.canUseMagic) mana = maxMana;
}

void Player::restoreFullHpAndMana() {
    hp   = maxHp;
    mana = maxMana;
}
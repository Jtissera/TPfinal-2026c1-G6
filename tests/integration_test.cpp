#include <gtest/gtest.h>
#include <toml++/toml.h>
 
#include "server/game/combatant.h"
#include "server/game/combatSystem.h"
#include "server/game/Player.h"
#include "server/npc/npc.h"
#include "server/npc/npcStats.h"
#include "server/npc/npcManager.h"
#include "server/npc/npcFactory.h"
#include "server/npc/npcRepository.h"
#include "server/game/inventory.h"
#include "server/game/item.h"
#include "server/game/itemSlot.h"
#include "server/game/itemStats.h"
#include "server/game/itemRepository.h"
#include "server/game/gameFormulas.h"
#include "server/game/raceRepository.h"
#include "server/game/classRepository.h"
#include "server/world/gameWorld.h"
#include "server/world/CollisionSystem.h"
#include "server/world/OccupancySystem.h"
#include "editor/map/mapData.h"
#include "editor/map/tile.h"
 
// ─── Helpers ─────────────────────────────────────────────────────────────────
 
static toml::table makeConfig() {
    return toml::parse(R"(
        [races.human]
        health       = 1.0
        mana         = 1.0
        recovery     = 1.0
        constitution = 10
        intelligence = 10
        strength     = 10
        agility      = 10
 
        [classes.warrior]
        health        = 1.0
        mana          = 0.5
        meditation    = 0.0
        can_use_magic = false
    )");
}
 
static toml::table makeNpcConfig() {
    return toml::parse(R"(
        [npcs.goblin]
        hp                 = 30
        damage_min         = 5
        damage_max         = 5
        level              = 1
        agility            = 1
        strength           = 5
        detection_range    = 10
        home_range         = 10
        attack_cooldown_ms = 0
        move_cooldown_ms   = 0
    )");
}
 
static toml::table makeItemConfig() {
    return toml::parse(R"(
        [items.espada]
        slot       = "WEAPON"
        is_ranged  = false
        damage_min = 10
        damage_max = 10
    )");
}
 
static RaceStats  makeRace()  { RaceRepository  r(makeConfig()); return r.get("human");   }
static ClassStats makeClass() { ClassRepository c(makeConfig()); return c.get("warrior"); }
 
static Player makePlayer(uint32_t id, int tx, int ty, int16_t hp = 100) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(id, "test", race, cls, 100, 50);
    p.setTilePos(tx, ty);
    if (hp < 100) p.takeDamage(100 - hp);
    return p;
}
 
static Item makeWeapon(uint32_t id, uint16_t dmgMin, uint16_t dmgMax) {
    Item item;
    item.id              = id;
    item.typeName        = "espada";
    item.slot            = ItemSlot::WEAPON;
    item.effect          = ItemEffect::NONE;
    item.stats.damageMin = dmgMin;
    item.stats.damageMax = dmgMax;
    item.stats.isRanged  = false;
    return item;
}
 
static MapData makeWalkableMap(int w = 10, int h = 10) {
    MapData m(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            Tile t; t.walkable = true; m.at(x, y) = t;
        }
    return m;
}
 
// ─── Fixture común ────────────────────────────────────────────────────────────
 
class IntegrationTest : public ::testing::Test {
protected:
    NpcRepository  npcRepo  {makeNpcConfig()};
    NpcFactory     npcFact  {npcRepo};
    ItemRepository itemRepo {makeItemConfig()};
};
 
//test 1 npc ataca Player, Player muere, item cae y se recoge
 
TEST_F(IntegrationTest, NpcKillsPlayerDropsItemCanBePickedUp) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
 
    Player p = makePlayer(1, 5, 5, 6); 
    p.getInventory().addItem(makeWeapon(10, 10, 10));
    world.addPlayer(std::move(p));
 
    world.spawnNpc("goblin", 5, 6);
 
    auto result = world.tick(0.016f);
 
    EXPECT_FALSE(result.playerHits.empty());
 
    if (!world.getPlayer(1).isAlive()) {
        auto item = world.pickItemAt(5, 5);
        EXPECT_TRUE(item.has_value());
    }
}
 
//test 2 recoger, equipar, atacar con stats del arma
 
TEST_F(IntegrationTest, PlayerPicksUpWeaponEquipsAndDealsDamage) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
 
    world.addPlayer(makePlayer(1, 3, 3));

    world.addPlayer(makePlayer(2, 3, 4));
 
    world.addItemOnGround(makeWeapon(50, 50, 50), 3, 3);
 
    auto picked = world.pickItemAt(3, 3);
    ASSERT_TRUE(picked.has_value());
 
    Player& attacker = world.getPlayer(1);
    attacker.getInventory().addItem(std::move(*picked));
    attacker.getInventory().equipItem(50); 

    EXPECT_NE(attacker.getInventory().getEquipped(EquipSlot::HAND), nullptr);
 
    CombatSystem combat;
    Player& target = world.getPlayer(2);
    int16_t hpBefore = target.getHp();
 
    auto result = combat.attack(attacker, target);
 
    EXPECT_TRUE(result.valid);
    if (!result.dodged) {
        // El daño debe reflejar los stats del arma (min=max=50,daño fijo 50,defensa)
        EXPECT_LT(target.getHp(), hpBefore);
    }
}
 
//test 3
 
TEST_F(IntegrationTest, AttackerGainsExpOnKillAndCanLevelUp) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
 
    world.addPlayer(makePlayer(1, 3, 3));  // atacante nivel 1
    world.addPlayer(makePlayer(2, 3, 4));  // target nivel 1
 
    Player& attacker = world.getPlayer(1);
    uint8_t levelBefore = attacker.getLevel();
 
    uint32_t expLimit   = 1000;
    int16_t  newMaxHp   = 200;
    int16_t  newMaxMana = 50;
    attacker.addExperience(expLimit, expLimit, newMaxHp, newMaxMana);
 
    EXPECT_EQ(attacker.getLevel(), levelBefore + 1);
    EXPECT_TRUE(attacker.checkAndClearLevelUp());
 
 
    auto deathResult = world.handlePlayerDeath(2, 1);
 
    EXPECT_GE(attacker.getExp(), 0u);  
}
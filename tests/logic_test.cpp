#include <gtest/gtest.h>
#include <toml++/toml.h>

#include "server/game/combatant.h"
#include "server/game/combatSystem.h"
#include "server/game/Player.h"
#include "server/npc/npc.h"
#include "server/npc/npcStats.h"
#include "server/game/inventory.h"
#include "server/game/item.h"
#include "server/game/itemSlot.h"
#include "server/game/itemStats.h"
#include "server/game/itemEffectHandler.h"
#include "server/game/gameFormulas.h"
#include "server/game/raceRepository.h"
#include "server/game/classRepository.h"
#include "server/world/gameWorld.h"
#include "server/world/CollisionSystem.h"
#include "server/world/OccupancySystem.h"
#include "editor/map/mapData.h"
#include "editor/map/tile.h"

// ─── Helpers ────────────────────────────────────────────────────────────────

static toml::table makeConfig() {
    return toml::parse(R"(
        [races.human]
        health = 1.0
        mana   = 1.0
        recovery = 1.0
        constitution = 10
        intelligence = 10
        strength = 10
        agility  = 10

        [classes.warrior]
        health     = 1.0
        mana       = 0.5
        meditation = 0.0
        can_use_magic = false

        [classes.mage]
        health     = 0.8
        mana       = 1.5
        meditation = 1.0
        can_use_magic = true
    )");
}

static RaceStats  makeRace()  { RaceRepository  r(makeConfig()); return r.get("human");   }
static ClassStats makeClass() { ClassRepository c(makeConfig()); return c.get("warrior"); }
static ClassStats makeMageClass() { ClassRepository c(makeConfig()); return c.get("mage"); }

static Player makePlayer(uint32_t id, int tx, int ty, int16_t hp = 100) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(id, "test", race, cls, 100, 50); 
    p.setTilePos(tx, ty);
    if (hp < 100) p.takeDamage(100 - hp);     
    return p;
}
static Player makeMagePlayer(uint32_t id, int tx, int ty) {
    static auto race = makeRace();
    static auto cls  = makeMageClass();
    Player p(id, "mage", race, cls, 80, 100);
    p.setTilePos(tx, ty);
    return p;
}

static NpcStats makeNpcStats(int16_t hp = 50,
                              uint16_t dmgMin = 5, uint16_t dmgMax = 10,
                              int detRange = 5, int homeRange = 10) {
    NpcStats s;
    s.typeName         = "test_npc";
    s.maxHp            = hp;
    s.damageMin        = dmgMin;
    s.damageMax        = dmgMax;
    s.level            = 1;
    s.agility          = 5;
    s.strength         = 5;
    s.detectionRange   = detRange;
    s.homeRange        = homeRange;
    s.attackCooldownMs = 0;
    s.moveCooldownMs   = 0;
    return s;
}

[[maybe_unused]]
static Npc makeNpc(uint32_t id, int tx, int ty, int16_t hp = 50) {
    static NpcStats stats = makeNpcStats(hp);
    return Npc(id, stats, tx, ty);
}

static MapData makeMap(int w, int h, bool walkable = true) {
    MapData m(w, h);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            Tile t;
            t.walkable = walkable;
            m.at(x, y) = t;
        }
    return m;
}

static Item makeWeapon(uint16_t dmgMin, uint16_t dmgMax, bool ranged = false) {
    Item item;
    item.instanceId              = 1;
    item.typeName        = "sword";
    item.slot            = ItemSlot::WEAPON;
    item.effect          = ItemEffect::NONE;
    item.stats.damageMin = dmgMin;
    item.stats.damageMax = dmgMax;
    item.stats.isRanged  = ranged;
    return item;
}

static Item makeArmor(uint16_t defMin, uint16_t defMax) {
    Item item;
    item.instanceId               = 2;
    item.typeName         = "armor";
    item.slot             = ItemSlot::ARMOR;
    item.stats.defenseMin = defMin;
    item.stats.defenseMax = defMax;
    return item;
}

static Item makeStaff(uint32_t id, ItemEffect effect,
                      uint16_t dmgMin = 0, uint16_t dmgMax = 0,
                      uint16_t healAmt = 0, uint16_t manaCost = 0) {
    Item item;
    item.instanceId              = id;
    item.typeName        = (effect == ItemEffect::HEAL) ? "flauta_elfica" : "vara_fresno";
    item.slot            = ItemSlot::STAFF;
    item.effect          = effect;
    item.stats.damageMin = dmgMin;
    item.stats.damageMax = dmgMax;
    item.stats.healAmount = healAmt;
    item.stats.manaCost  = manaCost;
    item.stats.isRanged  = true;
    return item;
}

static Item makePotion(uint32_t id, uint16_t healAmt, uint16_t manaAmt) {
    Item item;
    item.instanceId              = id;
    item.typeName        = healAmt > 0 ? "pocion_vida" : "pocion_mana";
    item.slot            = ItemSlot::CONSUMABLE;
    item.effect          = ItemEffect::NONE;
    item.stats.healAmount = healAmt;
    item.stats.manaAmount = manaAmt;
    return item;
}

// ─── CombatSystem ───────────────────────────────────────────────────────────

TEST(CombatSystemTest, AttackReducesTargetHp) {
    CombatSystem combat;
    Player attacker = makePlayer(1, 0, 0);
    Player target   = makePlayer(2, 0, 1);

    attacker.getInventory().addItem(makeWeapon(10, 10));
    attacker.getInventory().equipItem(1);

    int16_t hpBefore = target.getHp();
    auto result = combat.attack(attacker, target);

    EXPECT_TRUE(result.valid);
    if (!result.dodged) {
        EXPECT_LT(target.getHp(), hpBefore);
    }
}

TEST(CombatSystemTest, AttackOutOfRangeIsInvalid) {
    CombatSystem combat;
    Player attacker = makePlayer(1, 0, 0);
    Player target   = makePlayer(2, 5, 5);

    attacker.getInventory().addItem(makeWeapon(10, 10));
    attacker.getInventory().equipItem(1);

    auto result = combat.attack(attacker, target);
    EXPECT_FALSE(result.valid);
}

TEST(CombatSystemTest, RangedAttackReachesDistantTarget) {
    CombatSystem combat;
    Player attacker = makePlayer(1, 0, 0);
    Player target   = makePlayer(2, 8, 0);

    attacker.getInventory().addItem(makeWeapon(5, 5, true));
    attacker.getInventory().equipItem(1);

    auto result = combat.attack(attacker, target);
    EXPECT_TRUE(result.valid);
}

TEST(CombatSystemTest, DeadAttackerCannotAttack) {
    CombatSystem combat;
    Player attacker = makePlayer(1, 0, 0, 1);
    Player target   = makePlayer(2, 0, 1);

    attacker.takeDamage(100);
    auto result = combat.attack(attacker, target);
    EXPECT_FALSE(result.valid);
}

TEST(CombatSystemTest, DeadTargetCannotBeAttacked) {
    CombatSystem combat;
    Player attacker = makePlayer(1, 0, 0);
    Player target   = makePlayer(2, 0, 1, 1);

    target.takeDamage(100);
    auto result = combat.attack(attacker, target);
    EXPECT_FALSE(result.valid);
}

TEST(CombatSystemTest, ArmorReducesDamage) {
    CombatSystem combat;
    Player attacker  = makePlayer(1, 0, 0);
    Player noArmor   = makePlayer(2, 0, 1);
    Player withArmor = makePlayer(3, 0, 1);

    attacker.getInventory().addItem(makeWeapon(20, 20));
    attacker.getInventory().equipItem(1);

    withArmor.getInventory().addItem(makeArmor(10, 10));
    withArmor.getInventory().equipItem(2);

    auto r1 = combat.attack(attacker, noArmor);

    Player attacker2 = makePlayer(4, 0, 0);
    attacker2.getInventory().addItem(makeWeapon(20, 20));
    attacker2.getInventory().equipItem(1);
    auto r2 = combat.attack(attacker2, withArmor);

    if (r1.valid && !r1.dodged && r2.valid && !r2.dodged) {
        EXPECT_GE(r1.damage, r2.damage);    
    }
}

TEST(CombatSystemTest, NpcAttacksPlayer) {
    CombatSystem combat;
    static NpcStats stats = makeNpcStats(50, 10, 10);
    Npc npc(1, stats, 0, 0);
    Player target = makePlayer(2, 0, 1);

    auto result = combat.attack(npc, target);
    EXPECT_TRUE(result.valid);
}

TEST(CombatSystemTest, PlayerAttacksNpc) {
    CombatSystem combat;
    Player attacker = makePlayer(1, 0, 0);
    static NpcStats stats = makeNpcStats(50, 1, 2);
    Npc npc(2, stats, 0, 1);

    attacker.getInventory().addItem(makeWeapon(10, 10));
    attacker.getInventory().equipItem(1);

    auto result = combat.attack(attacker, npc);
    EXPECT_TRUE(result.valid);
}

TEST(CombatSystemTest, PvPRestrictedBelowLevel12) {
    CombatSystem combat;
    Player attacker = makePlayer(1, 0, 0);
    Player target   = makePlayer(2, 0, 1);

    attacker.getInventory().addItem(makeWeapon(10, 10));
    attacker.getInventory().equipItem(1);

    auto result = combat.attackPlayer(attacker, target);
    EXPECT_FALSE(result.valid);
}

// ─── Inventario ─────────────────────────────────────────────────────────────

TEST(InventoryTest, AddItemSucceeds) {
    Inventory inv;
    EXPECT_TRUE(inv.addItem(makeWeapon(5, 10)));
    EXPECT_EQ(inv.getItems().size(), 1u);
}

TEST(InventoryTest, EquipWeaponSucceeds) {
    Inventory inv;
    inv.addItem(makeWeapon(5, 10));
    EXPECT_TRUE(inv.equipItem(1));
    EXPECT_NE(inv.getEquipped(EquipSlot::HAND), nullptr);
}

TEST(InventoryTest, RemoveItemClearsEquip) {
    Inventory inv;
    inv.addItem(makeWeapon(5, 10));
    inv.equipItem(1);
    inv.removeItem(1);
    EXPECT_EQ(inv.getEquipped(EquipSlot::HAND), nullptr);
    EXPECT_TRUE(inv.getItems().empty());
}

TEST(InventoryTest, MaxItemsRespected) {
    Inventory inv;
    for (std::size_t i = 0; i < Inventory::MAX_ITEMS; i++) {
        Item item = makeWeapon(1, 2);
        item.instanceId = i + 1;
        EXPECT_TRUE(inv.addItem(item));
    }
    Item extra = makeWeapon(1, 2);
    extra.instanceId = 999;
    EXPECT_FALSE(inv.addItem(extra));
}

TEST(InventoryTest, RemoveAllItemsClearsInventory) {
    Inventory inv;
    inv.addItem(makeWeapon(5, 10));
    inv.addItem(makeArmor(3, 5));
    auto removed = inv.removeAllItems();
    EXPECT_EQ(removed.size(), 2u);
    EXPECT_TRUE(inv.getItems().empty());
}

TEST(InventoryTest, StaffReplacesWeaponInHand) {
    Inventory inv;

    Item weapon = makeWeapon(5, 10);
    weapon.instanceId   = 1;
    Item staff  = makeStaff(2, ItemEffect::DAMAGE, 2, 4, 0, 5);

    inv.addItem(weapon);
    inv.addItem(staff);

    EXPECT_TRUE(inv.equipItem(1));  // equipa arma
    EXPECT_TRUE(inv.equipItem(2));  // staff reemplaza al arma — válido
    EXPECT_EQ(inv.getEquipped(EquipSlot::HAND)->instanceId, 2u);  // queda el staff
}
TEST(InventoryTest, StaffReplacesStaff) {
    Inventory inv;

    Item staff1 = makeStaff(1, ItemEffect::DAMAGE, 2, 4, 0, 5);
    Item staff2 = makeStaff(2, ItemEffect::HEAL,   0, 0, 100, 100);

    inv.addItem(staff1);
    inv.addItem(staff2);

    EXPECT_TRUE(inv.equipItem(1));
    EXPECT_TRUE(inv.equipItem(2));  // reemplaza el staff anterior
    EXPECT_EQ(inv.getEquipped(EquipSlot::HAND)->instanceId, 2u);
}

// ─── Items — efectos ─────────────────────────────────────────────────────────

TEST(ItemEffectTest, HealthPotionHealsPlayer) {
    ItemEffectHandler handler;
    Player p = makePlayer(1, 0, 0, 50);  // hp = 50, max = 100
    Item potion = makePotion(1, 40, 0);

    EXPECT_TRUE(handler.apply(potion, p));
    EXPECT_EQ(p.getHp(), 90);
}

TEST(ItemEffectTest, HealthPotionDoesNotExceedMaxHp) {
    ItemEffectHandler handler;
    Player p = makePlayer(1, 0, 0, 100);  // hp lleno
    Item potion = makePotion(1, 50, 0);

    handler.apply(potion, p);
    EXPECT_EQ(p.getHp(), 100);  // no supera el max
}

TEST(ItemEffectTest, ManaPotionRestoresMana) {
    ItemEffectHandler handler;
    // mago para que pueda usar magia
    Player p = makeMagePlayer(1, 0, 0);
    p.spendMana(50);  // gastar para tener espacio

    Item potion = makePotion(1, 0, 30);
    handler.apply(potion, p);
    EXPECT_GT(p.getMana(), 50);
}

TEST(ItemEffectTest, FlautaElficaHealsPlayer) {
    ItemEffectHandler handler;
    // la flauta cura al usuario — necesita maná
    Player mage = makeMagePlayer(1, 0, 0);
    mage.takeDamage(40);  // bajarle vida

    int16_t hpBefore = mage.getHp();
    Item flauta = makeStaff(1, ItemEffect::HEAL, 0, 0, 100, 100);
    // el mago tiene 100 de maná y la flauta cuesta 100

    bool applied = handler.apply(flauta, mage);
    EXPECT_TRUE(applied);
    EXPECT_GT(mage.getHp(), hpBefore);
}

TEST(ItemEffectTest, FlautaElficaFailsWithoutMana) {
    ItemEffectHandler handler;
    Player mage = makeMagePlayer(1, 0, 0);
    mage.takeDamage(40);
    mage.spendMana(100);  // gastar todo el maná

    int16_t hpBefore = mage.getHp();
    Item flauta = makeStaff(1, ItemEffect::HEAL, 0, 0, 100, 100);

    bool applied = handler.apply(flauta, mage);
    EXPECT_FALSE(applied);
    EXPECT_EQ(mage.getHp(), hpBefore);  // no curó
}

// ─── Level up ────────────────────────────────────────────────────────────────

TEST(PlayerTest, LevelUpOnExpLimit) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(1, "test", race, cls, 100, 50);

    // límite nivel 1 = 1000 * 1^1.8 = 1000
    uint32_t limit    = 1000;
    int16_t newMaxHp  = 200;
    int16_t newMaxMana = 50;

    p.addExperience(limit, limit, newMaxHp, newMaxMana);

    EXPECT_EQ(p.getLevel(), 2);
    EXPECT_TRUE(p.checkAndClearLevelUp());
    EXPECT_FALSE(p.checkAndClearLevelUp());  // se limpia después de leer
}

TEST(PlayerTest, LevelUpIncreasesMaxHp) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(1, "test", race, cls, 100, 50);

    p.addExperience(1000, 1000, 200, 50);
    EXPECT_EQ(p.getMaxHp(), 200);
    EXPECT_EQ(p.getHp(), 200);  // se restaura al subir nivel
}

TEST(PlayerTest, NoLevelUpBelowLimit) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(1, "test", race, cls, 100, 50);

    p.addExperience(500, 1000, 200, 50);
    EXPECT_EQ(p.getLevel(), 1);
    EXPECT_FALSE(p.checkAndClearLevelUp());
}

// ─── Muerte y drops ──────────────────────────────────────────────────────────

TEST(PlayerTest, ResurrectionRestoresHalfHp) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(1, "test", race, cls, 100, 50);

    uint32_t safeGold = 100;
    p.die(safeGold);
    p.resurrect(0, 0);

    EXPECT_EQ(p.getHp(), 50);   // maxHp / 2
    EXPECT_EQ(p.getMana(), 0);
    EXPECT_TRUE(p.isAlive());
}

// ─── GameFormulas ─────────────────────────────────────────────────────────────

TEST(GameFormulasTest, MaxGoldLevel1) {
    GameFormulas f;
    // OroMax = 100 * 1^1.1 = 100
    EXPECT_EQ(f.calcMaxGold(1), 100u);
}

TEST(GameFormulasTest, ExpLimitLevel1) {
    GameFormulas f;
    // Limite = 1000 * 1^1.8 = 1000
    EXPECT_EQ(f.calcExpLimit(1), 1000u);
}

TEST(GameFormulasTest, ExpLimitLevel2) {
    GameFormulas f;
    // Limite = 1000 * 2^1.8 ≈ 3482
    uint32_t limit = f.calcExpLimit(2);
    EXPECT_GT(limit, 3000u);
    EXPECT_LT(limit, 4000u);
}

TEST(GameFormulasTest, ExcessGoldZeroIfUnderMax) {
    GameFormulas f;
    EXPECT_EQ(f.calcExcessGold(80, 100), 0u);
}

TEST(GameFormulasTest, ExcessGoldCalculatedCorrectly) {
    GameFormulas f;
    EXPECT_EQ(f.calcExcessGold(150, 100), 50u);
}

// ─── Colisiones ─────────────────────────────────────────────────────────────

TEST(CollisionTest, WalkableTileAllowsMovement) {
    auto map = makeMap(5, 5, true);
    CollisionSystem col(map);
    EXPECT_TRUE(col.isWalkable(2, 2));
}

TEST(CollisionTest, NonWalkableTileBlocksMovement) {
    auto map = makeMap(5, 5, true);
    map.at(2, 2).walkable = false;
    CollisionSystem col(map);
    EXPECT_FALSE(col.isWalkable(2, 2));
}

TEST(CollisionTest, OutOfBoundsIsNotWalkable) {
    auto map = makeMap(5, 5, true);
    CollisionSystem col(map);
    EXPECT_FALSE(col.isWalkable(-1, 0));
    EXPECT_FALSE(col.isWalkable(0, -1));
    EXPECT_FALSE(col.isWalkable(5, 0));
    EXPECT_FALSE(col.isWalkable(0, 5));
}

TEST(OccupancyTest, OccupyAndIsOccupied) {
    OccupancySystem occ;
    EXPECT_TRUE(occ.occupy(1, 1, 42));
    EXPECT_TRUE(occ.isOccupied(1, 1));
}

TEST(OccupancyTest, CannotOccupySameTileTwice) {
    OccupancySystem occ;
    occ.occupy(1, 1, 42);
    EXPECT_FALSE(occ.occupy(1, 1, 99));
}

TEST(OccupancyTest, FreeReleasesToccupancy) {
    OccupancySystem occ;
    occ.occupy(1, 1, 42);
    occ.free(1, 1);
    EXPECT_FALSE(occ.isOccupied(1, 1));
}

TEST(OccupancyTest, MoveUpdatesOccupancy) {
    OccupancySystem occ;
    occ.occupy(1, 1, 42);
    EXPECT_TRUE(occ.move(1, 1, 2, 2, 42));
    EXPECT_FALSE(occ.isOccupied(1, 1));
    EXPECT_TRUE(occ.isOccupied(2, 2));
}

TEST(OccupancyTest, MoveFailsIfDestinationOccupied) {
    OccupancySystem occ;
    occ.occupy(1, 1, 42);
    occ.occupy(2, 2, 99);
    EXPECT_FALSE(occ.move(1, 1, 2, 2, 42));
}

// ─── GameWorld ───────────────────────────────────────────────────────────────

class GameWorldTest : public ::testing::Test {
protected:
    toml::table config = makeConfig();
    NpcRepository npcRepo {toml::parse(R"(
        [npcs.goblin]
        hp                 = 30
        level              = 1
        detection_range    = 5
        home_range         = 10
        attack_cooldown_ms = 1000
        move_cooldown_ms   = 500
    )")};
    NpcFactory     npcFact  {npcRepo};
    ItemRepository itemRepo {toml::parse(R"([items])")};

    MapData makeWalkableMap() {
        MapData m(10, 10);
        for (int y = 0; y < 10; y++)
            for (int x = 0; x < 10; x++) {
                Tile t; t.walkable = true; m.at(x, y) = t;
            }
        for (int y = 0; y < 10; y++) m.at(5, y).walkable = false;
        return m;
    }
};

TEST_F(GameWorldTest, PlayerMovesOnWalkableTile) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    world.addPlayer(makePlayer(1, 3, 3));
    EXPECT_TRUE(world.movePlayer(1, Direction::RIGHT));
    EXPECT_EQ(world.getTileX(1), 4);
}

TEST_F(GameWorldTest, PlayerBlockedByNonWalkableTile) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    world.addPlayer(makePlayer(1, 4, 3));
    EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));
    EXPECT_EQ(world.getTileX(1), 4);
}

TEST_F(GameWorldTest, TwoPlayersCannotOccupySameTile) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    world.addPlayer(makePlayer(1, 3, 3));
    world.addPlayer(makePlayer(2, 4, 3));
    EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));
}

TEST_F(GameWorldTest, PlayerAndNpcCannotOccupySameTile) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    world.addPlayer(makePlayer(1, 3, 3));
    world.spawnNpc("goblin", 4, 3);
    EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));
}

TEST_F(GameWorldTest, PlayerDiesAndDropsExcessGold) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    Player p = makePlayer(1, 3, 3);
    p.addGold(200);
    world.addPlayer(std::move(p));

    auto result = world.handlePlayerDeath(1, 0);
    EXPECT_GT(result.excessGold, 0u);
    EXPECT_TRUE(world.getPlayer(1).isGhost());
}

TEST_F(GameWorldTest, ExcessGoldCanBePickedFromGround) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    Player p = makePlayer(1, 3, 3);
    p.addGold(200);
    world.addPlayer(std::move(p));

    world.handlePlayerDeath(1, 0);

    // el oro en exceso debería estar en el tile del jugador
    auto gold = world.pickGoldAt(3, 3);
    EXPECT_TRUE(gold.has_value());
    EXPECT_GT(*gold, 0u);
}

TEST_F(GameWorldTest, NoGoldDropIfUnderSafeAmount) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    Player p = makePlayer(1, 3, 3);
    p.addGold(50);  // menos que safeGold nivel 1 = 100
    world.addPlayer(std::move(p));

    world.handlePlayerDeath(1, 0);

    auto gold = world.pickGoldAt(3, 3);
    EXPECT_FALSE(gold.has_value());
}

TEST_F(GameWorldTest, PlayerDiesAndDropsItems) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    Player p = makePlayer(1, 3, 3);
    p.getInventory().addItem(makeWeapon(5, 10));
    world.addPlayer(std::move(p));

    auto result = world.handlePlayerDeath(1, 0);
    EXPECT_EQ(result.droppedItems.size(), 1u);
    EXPECT_TRUE(world.getPlayer(1).getInventory().getItems().empty());
}

TEST_F(GameWorldTest, DroppedItemCanBePickedFromGround) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    Player p = makePlayer(1, 3, 3);
    p.getInventory().addItem(makeWeapon(5, 10));
    world.addPlayer(std::move(p));

    world.handlePlayerDeath(1, 0);

    auto item = world.pickItemAt(3, 3);
    EXPECT_TRUE(item.has_value());
    EXPECT_EQ(item->typeName, "sword");
}

TEST_F(GameWorldTest, PlayerCanPickItemFromGround) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    world.addPlayer(makePlayer(1, 3, 3));
    world.addItemOnGround(makeWeapon(5, 10), 3, 3);

    auto item = world.pickItemAt(3, 3);
    EXPECT_TRUE(item.has_value());
    EXPECT_EQ(item->typeName, "sword");
}

TEST_F(GameWorldTest, PickItemEmptyIfNothingThere) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    auto item = world.pickItemAt(3, 3);
    EXPECT_FALSE(item.has_value());
}

TEST_F(GameWorldTest, PlayerResurrects) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    world.addPlayer(makePlayer(1, 3, 3));
    world.handlePlayerDeath(1, 0);
    EXPECT_TRUE(world.getPlayer(1).isGhost());

    world.resurrectPlayer(1, 2, 2);
    EXPECT_TRUE(world.getPlayer(1).isAlive());
}

TEST_F(GameWorldTest, PlayerResurrectedAtTargetZone) {
    GameWorld world(makeWalkableMap(), npcFact, itemRepo);
    world.addPlayer(makePlayer(1, 3, 3));
    world.handlePlayerDeath(1, 0);

    world.resurrectPlayer(1, 2, 2);

    int tx = world.getTileX(1);
    int ty = world.getTileY(1);
    // debe estar en (2,2) o en un tile adyacente si estaba ocupado
    EXPECT_LE(std::abs(tx - 2), 1);
    EXPECT_LE(std::abs(ty - 2), 1);
}

// ─── NPC ─────────────────────────────────────────────────────────────────────

TEST(NpcTest, TakesDamageCorrectly) {
    static NpcStats stats = makeNpcStats(50);
    Npc npc(1, stats, 0, 0);
    npc.takeDamage(20);
    EXPECT_EQ(npc.getHp(), 30);
}

TEST(NpcTest, DiesWhenHpReachesZero) {
    static NpcStats stats = makeNpcStats(10);
    Npc npc(1, stats, 0, 0);
    npc.takeDamage(10);
    EXPECT_FALSE(npc.isAlive());
}

TEST(NpcTest, CannotGoBelowZeroHp) {
    static NpcStats stats = makeNpcStats(10);
    Npc npc(1, stats, 0, 0);
    npc.takeDamage(100);
    EXPECT_EQ(npc.getHp(), 0);
}

TEST(NpcTest, AttackCooldownRespected) {
    NpcStats stats = makeNpcStats(50);
    stats.attackCooldownMs = 5000;
    Npc npc(1, stats, 0, 0);
    EXPECT_TRUE(npc.canAttack());
    npc.resetAttackCooldown();
    EXPECT_FALSE(npc.canAttack());
}

// ─── Restricciones de clase ───────────────────────────────────────────────────

TEST(PlayerTest, WarriorCannotRestoreMana) {
    static auto race = makeRace();
    static auto cls  = makeClass();  // warrior, canUseMagic = false
    Player p(1, "warrior", race, cls, 100, 0);

    p.restoreMana(50);
    EXPECT_EQ(p.getMana(), 0);  // no cambia
}

TEST(PlayerTest, WarriorCannotSpendMana) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(1, "warrior", race, cls, 100, 0);

    EXPECT_FALSE(p.spendMana(10));  // falla siempre
}

TEST(PlayerTest, WarriorManaPotionHasNoEffect) {
    ItemEffectHandler handler;
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(1, "warrior", race, cls, 100, 0);

    Item potion = makePotion(1, 0, 50);
    handler.apply(potion, p);
    EXPECT_EQ(p.getMana(), 0);  
}

TEST(PlayerTest, WarriorCannotMeditate) {
    static auto race = makeRace();
    static auto cls  = makeClass();
    Player p(1, "warrior", race, cls, 100, 0);

    p.startMeditating();
    EXPECT_FALSE(p.isMeditating()); 
}

TEST(PlayerTest, MageCanUseMagic) {
    static auto race = makeRace();
    static auto cls  = makeMageClass();
    Player p(1, "mage", race, cls, 80, 100);

    EXPECT_TRUE(p.spendMana(10));
    EXPECT_EQ(p.getMana(), 90);
}
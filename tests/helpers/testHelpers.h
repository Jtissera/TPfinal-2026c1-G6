#include "../server/game/player/Player.h"
#include "editor/map/mapData.h"
#include "editor/map/tile.h"
#include "server/game/combat/combatSystem.h"
#include "server/game/items/item.h"
#include "server/game/items/itemRepository.h"
#include "server/game/items/itemSlot.h"
#include "server/game/items/itemStats.h"
#include "server/game/player/combatant.h"
#include "server/game/player/inventory.h"
#include "server/game/stats/classRepository.h"
#include "server/game/stats/gameFormulas.h"
#include "server/game/stats/raceRepository.h"
#include "server/npc/npc.h"
#include "server/npc/npcFactory.h"
#include "server/npc/npcManager.h"
#include "server/npc/npcRepository.h"
#include "server/npc/npcStats.h"
#include "server/world/CollisionSystem.h"
#include "server/world/OccupancySystem.h"
#include "server/world/gameWorld.h"
#include <toml++/toml.h>

[[maybe_unused]] static toml::table makeConfig() {
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

        [classes.mage]
        health        = 0.8
        mana          = 1.5
        meditation    = 1.0
        can_use_magic = true

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

        [items.espada]
        catalog_id = 1
        slot       = "WEAPON"
        is_ranged  = false
        damage_min = 10
        damage_max = 10

        [world]
        tile_size = 96

        [npc]
        spawn_interval_ticks = 200
        max_population       = 20
        spawn_batch_size     = 4

        [combat]
        attack_range        = 1
        max_level_diff      = 10
        ranged_attack_range = 10

        [player]
        newbie_max_level    = 12
        max_inventory_items = 20
    )");
}

[[maybe_unused]] static Inventory makeInventory() {
  static auto config = makeConfig();
  return Inventory(config);
}

[[maybe_unused]] static RaceStats makeRace() {
  RaceRepository r(makeConfig());
  return r.get("human");
}

[[maybe_unused]] static ClassStats makeClass() {
  ClassRepository c(makeConfig());
  return c.get("warrior");
}

[[maybe_unused]] static Player makePlayer(uint32_t id, int tx, int ty,
                                          int16_t hp = 100) {
  static auto race = makeRace();
  static auto cls = makeClass();
  static auto config = makeConfig();
  Player p(id, "test", race, cls, 100, 50, config);
  p.setTilePos(tx, ty);
  if (hp < 100)
    p.takeDamage(100 - hp);
  return p;
}

[[maybe_unused]] static Item makeWeaponWithId(uint32_t id, uint16_t dmgMin,
                                              uint16_t dmgMax) {
  Item item;
  item.catalogId = id;
  item.typeName = "espada";
  item.slot = ItemSlot::WEAPON;
  item.effect = ItemEffect::NONE;
  item.stats.damageMin = dmgMin;
  item.stats.damageMax = dmgMax;
  item.stats.isRanged = false;
  return item;
}

[[maybe_unused]] static MapData makeWalkableMap(int w = 10, int h = 10) {
  MapData m(w, h);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      Tile t;
      t.walkable = true;
      m.at(x, y) = t;
    }
  return m;
}

[[maybe_unused]] static ClassStats makeMageClass() {
  ClassRepository c(makeConfig());
  return c.get("mage");
}

[[maybe_unused]] static Player makeMagePlayer(uint32_t id, int tx, int ty) {
  static auto race = makeRace();
  static auto cls = makeMageClass();
  static auto config = makeConfig();
  Player p(id, "mage", race, cls, 80, 100, config);
  p.setTilePos(tx, ty);
  return p;
}

[[maybe_unused]] static NpcStats
makeNpcStats(int16_t hp = 50, uint16_t dmgMin = 5, uint16_t dmgMax = 10,
             int detRange = 5, int homeRange = 10) {
  NpcStats s;
  s.typeName = "test_npc";
  s.maxHp = hp;
  s.damageMin = dmgMin;
  s.damageMax = dmgMax;
  s.level = 1;
  s.agility = 5;
  s.strength = 5;
  s.detectionRange = detRange;
  s.homeRange = homeRange;
  s.attackCooldownMs = 0;
  s.moveCooldownMs = 0;
  return s;
}

[[maybe_unused]] static Npc makeNpc(uint32_t id, int tx, int ty,
                                    int16_t hp = 50) {
  static NpcStats stats = makeNpcStats(hp);
  return Npc(id, stats, tx, ty);
}

[[maybe_unused]] static MapData makeMap(int w, int h, bool walkable = true) {
  MapData m(w, h);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      Tile t;
      t.walkable = walkable;
      m.at(x, y) = t;
    }
  return m;
}

[[maybe_unused]] static Item makeWeapon(uint16_t dmgMin, uint16_t dmgMax,
                                        bool ranged = false) {
  Item item;
  item.catalogId = 1;
  item.instanceId = 1;  
  item.typeName = "sword";
  item.slot = ItemSlot::WEAPON;
  item.effect = ItemEffect::NONE;
  item.stats.damageMin = dmgMin;
  item.stats.damageMax = dmgMax;
  item.stats.isRanged = ranged;
  return item;
}

[[maybe_unused]] static Item makeArmor(uint16_t defMin, uint16_t defMax) {
  Item item;
  item.catalogId = 2;
  item.instanceId = 2;
  item.typeName = "armor";
  item.slot = ItemSlot::ARMOR;
  item.stats.defenseMin = defMin;
  item.stats.defenseMax = defMax;
  return item;
}

[[maybe_unused]] static Item makeStaff(uint32_t id, ItemEffect effect,
                                       uint16_t dmgMin = 0, uint16_t dmgMax = 0,
                                       uint16_t healAmt = 0,
                                       uint16_t manaCost = 0) {
  Item item;
  item.catalogId = id;
  item.typeName =
      (effect == ItemEffect::HEAL) ? "flauta_elfica" : "vara_fresno";
  item.slot = ItemSlot::STAFF;
  item.effect = effect;
  item.stats.damageMin = dmgMin;
  item.stats.damageMax = dmgMax;
  item.stats.healAmount = healAmt;
  item.stats.manaCost = manaCost;
  item.stats.isRanged = true;
  return item;
}

[[maybe_unused]] static Item makePotion(uint32_t id, uint16_t healAmt,
                                        uint16_t manaAmt) {
  Item item;
  item.catalogId = id;
  item.typeName = healAmt > 0 ? "pocion_vida" : "pocion_mana";
  item.slot = ItemSlot::CONSUMABLE;
  item.effect = ItemEffect::NONE;
  item.stats.healAmount = healAmt;
  item.stats.manaAmount = manaAmt;
  return item;
}
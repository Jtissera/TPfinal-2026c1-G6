#include <gtest/gtest.h>
#include "helpers/testHelpers.h"
#include "server/npc/npcRepository.h"
#include "server/npc/npcStats.h"
#include "editor/map/tile.h"

// ---------------------------------------------------------------------------
// NpcStats — zonas y multiplicadores
// ---------------------------------------------------------------------------

static toml::table makeDungeonConfig()
{
    return toml::parse(R"(
        [cavern]
        gold_multiplier = 1.5
        xp_multiplier   = 1.5
        item_multiplier = 1.5

        [dungeon]
        gold_multiplier = 3.0
        xp_multiplier   = 2.5
        item_multiplier = 2.5

        [npcs.goblin_cave]
        hp = 80
        damage_min = 5
        damage_max = 9
        level = 6
        agility = 9
        strength = 7
        detection_range = 6
        home_range = 8
        attack_cooldown_ms = 900
        move_cooldown_ms = 450
        zones = ["CAVERN"]

        [npcs.goblin_dungeon]
        hp = 150
        damage_min = 12
        damage_max = 20
        level = 14
        agility = 11
        strength = 12
        detection_range = 8
        home_range = 10
        attack_cooldown_ms = 800
        move_cooldown_ms = 400
        zones = ["DUNGEON"]

        [npcs.goblin]
        hp = 50
        damage_min = 2
        damage_max = 5
        level = 3
        agility = 8
        strength = 5
        detection_range = 5
        home_range = 10
        attack_cooldown_ms = 1000
        move_cooldown_ms = 500
        zones = ["COMBAT"]

        [world]
        tile_size = 96
        [npc]
        spawn_interval_ticks = 200
        max_population = 20
        spawn_batch_size = 4
        [combat]
        attack_range = 1
        max_level_diff = 10
        ranged_attack_range = 10
        [player]
        newbie_max_level = 12
        max_inventory_items = 20
    )");
}

TEST(NpcZone, CombatNpcHasDefaultMultipliers)
{
    auto config = makeDungeonConfig();
    NpcRepository repo(config);
    const NpcStats &stats = repo.get("goblin");

    EXPECT_EQ(stats.homeZone, ZoneType::COMBAT);
    EXPECT_FLOAT_EQ(stats.goldMultiplier, 1.0f);
    EXPECT_FLOAT_EQ(stats.xpMultiplier, 1.0f);
    EXPECT_FLOAT_EQ(stats.itemMultiplier, 1.0f);
}

TEST(NpcZone, CavernNpcHasCavernZone)
{
    auto config = makeDungeonConfig();
    NpcRepository repo(config);
    const NpcStats &stats = repo.get("goblin_cave");

    EXPECT_EQ(stats.homeZone, ZoneType::CAVERN);
}

TEST(NpcZone, CavernNpcHasCavernMultipliers)
{
    auto config = makeDungeonConfig();
    NpcRepository repo(config);
    const NpcStats &stats = repo.get("goblin_cave");

    EXPECT_FLOAT_EQ(stats.goldMultiplier, 1.5f);
    EXPECT_FLOAT_EQ(stats.xpMultiplier, 1.5f);
    EXPECT_FLOAT_EQ(stats.itemMultiplier, 1.5f);
}

TEST(NpcZone, DungeonNpcHasDungeonZone)
{
    auto config = makeDungeonConfig();
    NpcRepository repo(config);
    const NpcStats &stats = repo.get("goblin_dungeon");

    EXPECT_EQ(stats.homeZone, ZoneType::DUNGEON);
}

TEST(NpcZone, DungeonNpcHasDungeonMultipliers)
{
    auto config = makeDungeonConfig();
    NpcRepository repo(config);
    const NpcStats &stats = repo.get("goblin_dungeon");

    EXPECT_FLOAT_EQ(stats.goldMultiplier, 3.0f);
    EXPECT_FLOAT_EQ(stats.xpMultiplier, 2.5f);
    EXPECT_FLOAT_EQ(stats.itemMultiplier, 2.5f);
}

TEST(NpcZone, DungeonNpcStrongerThanCavernNpc)
{
    auto config = makeDungeonConfig();
    NpcRepository repo(config);

    EXPECT_GT(repo.get("goblin_dungeon").maxHp,
              repo.get("goblin_cave").maxHp);
    EXPECT_GT(repo.get("goblin_dungeon").level,
              repo.get("goblin_cave").level);
}

TEST(NpcZone, CavernNpcStrongerThanCombatNpc)
{
    auto config = makeDungeonConfig();
    NpcRepository repo(config);

    EXPECT_GT(repo.get("goblin_cave").maxHp,
              repo.get("goblin").maxHp);
}

// ---------------------------------------------------------------------------
// Tile — entradas y serialización de targetMap
// ---------------------------------------------------------------------------

TEST(TileDungeon, DungeonEntranceTileHasCorrectType)
{
    Tile t;
    t.type = TileType::DUNGEON_ENTRANCE;
    t.zone = ZoneType::DUNGEON;
    t.walkable = true;
    t.targetMap = "maps/dungeon01.argmap";

    EXPECT_EQ(t.type, TileType::DUNGEON_ENTRANCE);
    EXPECT_EQ(t.zone, ZoneType::DUNGEON);
    EXPECT_EQ(t.targetMap, "maps/dungeon01.argmap");
}

TEST(TileDungeon, CavernEntranceTileHasCorrectType)
{
    Tile t;
    t.type = TileType::CAVERN_ENTRANCE;
    t.zone = ZoneType::CAVERN;
    t.walkable = true;
    t.targetMap = "maps/cavern01.argmap";

    EXPECT_EQ(t.type, TileType::CAVERN_ENTRANCE);
    EXPECT_EQ(t.zone, ZoneType::CAVERN);
}

TEST(TileDungeon, ExitTileHasCorrectType)
{
    Tile t;
    t.type = TileType::EXIT;
    t.walkable = true;

    EXPECT_EQ(t.type, TileType::EXIT);
    EXPECT_TRUE(t.targetMap.empty());
}

// ---------------------------------------------------------------------------
// MapData — serialización round-trip con targetMap
// ---------------------------------------------------------------------------

#include "editor/map/mapSerializer.h"
#include <filesystem>
#include <cstdio>

TEST(MapSerializer, RoundTripPreservesTargetMap)
{
    MapData map(5, 5);
    Tile entrance;
    entrance.type = TileType::DUNGEON_ENTRANCE;
    entrance.zone = ZoneType::DUNGEON;
    entrance.walkable = true;
    entrance.targetMap = "maps/dungeon01.argmap";
    map.at(2, 2) = entrance;

    const std::string path = "/tmp/test_dungeon_roundtrip.argmap";
    MapSerializer::save(map, path);
    MapData loaded = MapSerializer::load(path);
    std::remove(path.c_str());

    EXPECT_EQ(loaded.at(2, 2).type, TileType::DUNGEON_ENTRANCE);
    EXPECT_EQ(loaded.at(2, 2).zone, ZoneType::DUNGEON);
    EXPECT_EQ(loaded.at(2, 2).targetMap, "maps/dungeon01.argmap");
}

TEST(MapSerializer, RoundTripPreservesCavernEntrance)
{
    MapData map(5, 5);
    Tile entrance;
    entrance.type = TileType::CAVERN_ENTRANCE;
    entrance.zone = ZoneType::CAVERN;
    entrance.walkable = true;
    entrance.targetMap = "maps/cavern01.argmap";
    map.at(1, 1) = entrance;

    const std::string path = "/tmp/test_cavern_roundtrip.argmap";
    MapSerializer::save(map, path);
    MapData loaded = MapSerializer::load(path);
    std::remove(path.c_str());

    EXPECT_EQ(loaded.at(1, 1).targetMap, "maps/cavern01.argmap");
    EXPECT_EQ(loaded.at(1, 1).zone, ZoneType::CAVERN);
}

TEST(MapSerializer, ExitTileRoundTrip)
{
    MapData map(5, 5);
    Tile exit;
    exit.type = TileType::EXIT;
    exit.walkable = true;
    map.at(3, 3) = exit;

    const std::string path = "/tmp/test_exit_roundtrip.argmap";
    MapSerializer::save(map, path);
    MapData loaded = MapSerializer::load(path);
    std::remove(path.c_str());

    EXPECT_EQ(loaded.at(3, 3).type, TileType::EXIT);
    EXPECT_TRUE(loaded.at(3, 3).targetMap.empty());
}

TEST(MapSerializer, NonEntranceTilesHaveEmptyTargetMap)
{
    MapData map(3, 3);
    // todos los tiles son GRASS por defecto

    const std::string path = "/tmp/test_grass_roundtrip.argmap";
    MapSerializer::save(map, path);
    MapData loaded = MapSerializer::load(path);
    std::remove(path.c_str());

    for (uint16_t y = 0; y < 3; y++)
        for (uint16_t x = 0; x < 3; x++)
            EXPECT_TRUE(loaded.at(x, y).targetMap.empty());
}
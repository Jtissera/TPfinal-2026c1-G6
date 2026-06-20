#include "helpers/testHelpers.h"
#include <gtest/gtest.h>

// ─── Colisiones ──────────────────────────────────────────────────────────────

TEST(CollisionTest, WalkableTileAllowsMovement)
{
  auto map = makeMap(5, 5, true);
  CollisionSystem col(map);
  EXPECT_TRUE(col.isWalkable(2, 2));
}

TEST(CollisionTest, NonWalkableTileBlocksMovement)
{
  auto map = makeMap(5, 5, true);
  map.at(2, 2).walkable = false;
  CollisionSystem col(map);
  EXPECT_FALSE(col.isWalkable(2, 2));
}

TEST(CollisionTest, OutOfBoundsIsNotWalkable)
{
  auto map = makeMap(5, 5, true);
  CollisionSystem col(map);
  EXPECT_FALSE(col.isWalkable(-1, 0));
  EXPECT_FALSE(col.isWalkable(0, -1));
  EXPECT_FALSE(col.isWalkable(5, 0));
  EXPECT_FALSE(col.isWalkable(0, 5));
}

// ─── Ocupancia ───────────────────────────────────────────────────────────────

TEST(OccupancyTest, OccupyAndIsOccupied)
{
  OccupancySystem occ;
  EXPECT_TRUE(occ.occupy(1, 1, 42));
  EXPECT_TRUE(occ.isOccupied(1, 1));
}

TEST(OccupancyTest, CannotOccupySameTileTwice)
{
  OccupancySystem occ;
  occ.occupy(1, 1, 42);
  EXPECT_FALSE(occ.occupy(1, 1, 99));
}

TEST(OccupancyTest, FreeReleasesToccupancy)
{
  OccupancySystem occ;
  occ.occupy(1, 1, 42);
  occ.free(1, 1);
  EXPECT_FALSE(occ.isOccupied(1, 1));
}

TEST(OccupancyTest, MoveUpdatesOccupancy)
{
  OccupancySystem occ;
  occ.occupy(1, 1, 42);
  EXPECT_TRUE(occ.move(1, 1, 2, 2, 42));
  EXPECT_FALSE(occ.isOccupied(1, 1));
  EXPECT_TRUE(occ.isOccupied(2, 2));
}

TEST(OccupancyTest, MoveFailsIfDestinationOccupied)
{
  OccupancySystem occ;
  occ.occupy(1, 1, 42);
  occ.occupy(2, 2, 99);
  EXPECT_FALSE(occ.move(1, 1, 2, 2, 42));
}

// ─── GameWorld ───────────────────────────────────────────────────────────────

class GameWorldTest : public ::testing::Test
{
protected:
  toml::table config = makeConfig();
  NpcRepository npcRepo{config};
  NpcFactory npcFact{npcRepo};
  ItemRepository itemRepo{toml::parse(R"([items])")};
  
  std::string tempDir = std::filesystem::temp_directory_path().string();

  ClanArchive clanArchive{tempDir + "/test_clans.dat", tempDir + "/test_clans.idx"};
  CharacterArchive characterArchive{tempDir + "/test_chars.dat", tempDir + "/test_chars.idx"};
  ClanManager clanManager{clanArchive, characterArchive};

  MapData makeWalkableMap()
  {
    MapData m(10, 10);
    for (int y = 0; y < 10; y++)
      for (int x = 0; x < 10; x++)
      {
        Tile t;
        t.walkable = true;
        m.at(x, y) = t;
      }
    for (int y = 0; y < 10; y++)
      m.at(5, y).walkable = false;
    return m;
  }
};

TEST_F(GameWorldTest, PlayerMovesOnWalkableTile)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));
  EXPECT_TRUE(world.movePlayer(1, Direction::RIGHT));
  EXPECT_EQ(world.getTileX(1), 4);
}

TEST_F(GameWorldTest, PlayerBlockedByNonWalkableTile)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 4, 3));
  EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));
  EXPECT_EQ(world.getTileX(1), 4);
}

TEST_F(GameWorldTest, TwoPlayersCannotOccupySameTile)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));
  world.addPlayer(makePlayer(2, 4, 3));
  EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));
}

TEST_F(GameWorldTest, PlayerAndNpcCannotOccupySameTile)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));
  world.spawnNpc("goblin", 4, 3);
  EXPECT_FALSE(world.movePlayer(1, Direction::RIGHT));
}

TEST_F(GameWorldTest, PlayerDiesAndDropsExcessGold)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  Player p = makePlayer(1, 3, 3);
  p.addGold(200);
  world.addPlayer(std::move(p));

  auto result = world.handlePlayerDeath(1, 0);
  EXPECT_GT(result.excessGold, 0u);
  EXPECT_TRUE(world.getPlayer(1).isGhost());
}

TEST_F(GameWorldTest, ExcessGoldCanBePickedFromGround)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  Player p = makePlayer(1, 3, 3);
  p.addGold(200);
  world.addPlayer(std::move(p));
  auto deathResult = world.handlePlayerDeath(1,0);
  EXPECT_GT(deathResult.excessGold,0u);
  auto gold = world.pickGoldById(deathResult.goldInstanceId);
  EXPECT_TRUE(gold.has_value());
  EXPECT_GT(*gold, 0u);
}

TEST_F(GameWorldTest, NoGoldDropIfUnderSafeAmount)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  Player p = makePlayer(1, 3, 3);
  p.addGold(50);
  world.addPlayer(std::move(p));
  auto deathResult = world.handlePlayerDeath(1,0);
  EXPECT_EQ(deathResult.excessGold,0u);

}

TEST_F(GameWorldTest, PlayerDiesAndDropsItems)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  Player p = makePlayer(1, 3, 3);
  p.getInventory().addItem(makeWeapon(5, 10));
  world.addPlayer(std::move(p));

  auto result = world.handlePlayerDeath(1, 0);
  EXPECT_EQ(result.droppedItems.size(), 1u);
  EXPECT_TRUE(world.getPlayer(1).getInventory().getItems().empty());
}

TEST_F(GameWorldTest, DroppedItemCanBePickedFromGround)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  Player p = makePlayer(1, 3, 3);
  p.getInventory().addItem(makeWeapon(5, 10));
  world.addPlayer(std::move(p));
  auto deathResult = world.handlePlayerDeath(1, 0);
  ASSERT_EQ(deathResult.droppedItems.size(), 1u);
  auto item = world.pickItemById(deathResult.droppedItems[0].instanceId);
  EXPECT_TRUE(item.has_value());
  EXPECT_EQ(item->typeName, "sword");
}

TEST_F(GameWorldTest, PlayerCanPickItemFromGround)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));
  Item weapon = makeWeapon(5, 10);
  const uint32_t instanceId = weapon.instanceId;
  world.addItemOnGround(weapon, 3, 3);
  auto item = world.pickItemById(instanceId);
  EXPECT_TRUE(item.has_value());
  EXPECT_EQ(item->typeName, "sword");
}

TEST_F(GameWorldTest, PickItemEmptyIfNothingThere)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  auto item = world.pickItemById(999);
  EXPECT_FALSE(item.has_value());
}

TEST_F(GameWorldTest, PlayerResurrects)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));
  world.handlePlayerDeath(1, 0);
  EXPECT_TRUE(world.getPlayer(1).isGhost());

  world.resurrectPlayer(1, 2, 2);
  EXPECT_TRUE(world.getPlayer(1).isAlive());
}

TEST_F(GameWorldTest, PlayerResurrectedAtTargetZone)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));
  world.handlePlayerDeath(1, 0);

  world.resurrectPlayer(1, 2, 2);

  int tx = world.getTileX(1);
  int ty = world.getTileY(1);
  EXPECT_LE(std::abs(tx - 2), 1);
  EXPECT_LE(std::abs(ty - 2), 1);
}

// ─── NPC ─────────────────────────────────────────────────────────────────────

TEST(NpcTest, TakesDamageCorrectly)
{
  static NpcStats stats = makeNpcStats(50);
  Npc npc(1, stats, 0, 0);
  npc.takeDamage(20);
  EXPECT_EQ(npc.getHp(), 30);
}

TEST(NpcTest, DiesWhenHpReachesZero)
{
  static NpcStats stats = makeNpcStats(10);
  Npc npc(1, stats, 0, 0);
  npc.takeDamage(10);
  EXPECT_FALSE(npc.isAlive());
}

TEST(NpcTest, CannotGoBelowZeroHp)
{
  static NpcStats stats = makeNpcStats(10);
  Npc npc(1, stats, 0, 0);
  npc.takeDamage(100);
  EXPECT_EQ(npc.getHp(), 0);
}

TEST(NpcTest, AttackCooldownRespected)
{
  NpcStats stats = makeNpcStats(50);
  stats.attackCooldownMs = 5000;
  Npc npc(1, stats, 0, 0);
  EXPECT_TRUE(npc.canAttack());
  npc.resetAttackCooldown();
  EXPECT_FALSE(npc.canAttack());
}

TEST_F(GameWorldTest, PlayerOnEntranceTileGeneratesTransition)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);

  // Crear un mapa con tile de entrada
  MapData m = makeWalkableMap();
  Tile entrance;
  entrance.type = TileType::DUNGEON_ENTRANCE;
  entrance.walkable = true;
  entrance.targetMap = "dungeon.argmap";
  m.at(3, 3) = entrance;

  GameWorld worldWithEntrance(std::move(m), npcFact, itemRepo, config, clanManager);
  worldWithEntrance.addPlayer(makePlayer(1, 3, 3));

  auto result = worldWithEntrance.tick(0.016f);

  ASSERT_EQ(result.instanceTransitions.size(), 1u);
  EXPECT_EQ(result.instanceTransitions[0].playerId, 1u);
  EXPECT_EQ(result.instanceTransitions[0].targetMap, "dungeon.argmap");
}

TEST_F(GameWorldTest, PlayerOnExitTileGeneratesEmptyTransition)
{
  MapData m = makeWalkableMap();
  Tile exit;
  exit.type = TileType::EXIT;
  exit.walkable = true;
  m.at(3, 3) = exit;

  GameWorld world(std::move(m), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));

  auto result = world.tick(0.016f);

  ASSERT_EQ(result.instanceTransitions.size(), 1u);
  EXPECT_EQ(result.instanceTransitions[0].playerId, 1u);
  EXPECT_TRUE(result.instanceTransitions[0].targetMap.empty());
}

TEST_F(GameWorldTest, PlayerOnNormalTileGeneratesNoTransition)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));

  auto result = world.tick(0.016f);

  EXPECT_TRUE(result.instanceTransitions.empty());
}

TEST_F(GameWorldTest, FindSafeSpawnNearReturnsAdjacentNonEntranceTile)
{
  MapData m = makeWalkableMap();
  Tile entrance;
  entrance.type = TileType::DUNGEON_ENTRANCE;
  entrance.walkable = true;
  entrance.targetMap = "dungeon.argmap";
  m.at(3, 3) = entrance;

  GameWorld world(std::move(m), npcFact, itemRepo, config, clanManager);
  auto [tx, ty] = world.findSafeSpawnNear(3, 3);

  const Tile &t = world.getTileAt(tx, ty);
  EXPECT_NE(t.type, TileType::DUNGEON_ENTRANCE);
  EXPECT_NE(t.type, TileType::CAVERN_ENTRANCE);
  EXPECT_NE(t.type, TileType::EXIT);
  EXPECT_TRUE(t.walkable);
}
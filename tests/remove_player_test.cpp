#include <gtest/gtest.h>
#include <toml++/toml.h>

#include "helpers/testHelpers.h"

class ExtractPlayerTest : public ::testing::Test
{
protected:
  toml::table config = makeConfig();
  NpcRepository npcRepo{config};
  NpcFactory npcFact{npcRepo, config};
  ItemRepository itemRepo{config};
  
  std::string tempDir = std::filesystem::temp_directory_path().string();

  ClanArchive clanArchive{tempDir + "/test_clans.dat", tempDir + "/test_clans.idx"};
  CharacterArchive characterArchive{tempDir + "/test_chars.dat", tempDir + "/test_chars.idx"};
  ClanManager clanManager{clanArchive, characterArchive, config};
};

TEST_F(ExtractPlayerTest, RemoveReturnsPlayerWithCorrectId)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));

  auto extracted = world.removePlayer(1);
  EXPECT_EQ(extracted->getId(), 1u);
}

TEST_F(ExtractPlayerTest, RemoveRemovesPlayerFromWorld)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));

  world.removePlayer(1);
  EXPECT_THROW(world.getPlayer(1), std::runtime_error);
}

TEST_F(ExtractPlayerTest, RemoveFreesOccupancyTileForReuse)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  world.addPlayer(makePlayer(1, 3, 3));

  world.removePlayer(1);

  // Si el tile fue liberado, otro player puede ocuparlo
  EXPECT_NO_THROW(world.addPlayer(makePlayer(2, 3, 3)));
}

TEST_F(ExtractPlayerTest, RemovePreservesPlayerState)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  Player p = makePlayer(1, 3, 3);
  Item weapon = makeWeaponWithId(10, 10, 10);
  p.getInventory().addItem(weapon);
  uint32_t weaponId = p.getInventory().getItems().back().instanceId;
  world.addPlayer(std::move(p));

  world.getPlayer(1).takeDamage(30);
  int16_t hpBeforeExtract = world.getPlayer(1).getHp();

  auto extracted = world.removePlayer(1);

  EXPECT_EQ(extracted->getHp(), hpBeforeExtract);
  EXPECT_NE(extracted->getInventory().findItem(weaponId), nullptr);
}
TEST_F(ExtractPlayerTest, RemoveNonExistentPlayerReturnsNullopt)
{
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config, clanManager);
  EXPECT_EQ(world.removePlayer(999), std::nullopt);
}


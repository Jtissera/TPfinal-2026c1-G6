#include <gtest/gtest.h>
#include <toml++/toml.h>

#include "helpers/testHelpers.h"

class ExtractPlayerTest : public ::testing::Test {
protected:
  toml::table config = makeConfig();
  NpcRepository npcRepo{config};
  NpcFactory npcFact{npcRepo};
  ItemRepository itemRepo{config};
};
TEST_F(ExtractPlayerTest, RemoveReturnsPlayerWithCorrectId) {
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);
  world.addPlayer(makePlayer(1, 3, 3));

  auto extracted = world.removePlayer(1);
  EXPECT_EQ(extracted->getId(), 1u);
}

TEST_F(ExtractPlayerTest, RemoveRemovesPlayerFromWorld) {
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);
  world.addPlayer(makePlayer(1, 3, 3));

  world.removePlayer(1);
  EXPECT_THROW(world.getPlayer(1), std::runtime_error);
}

TEST_F(ExtractPlayerTest, RemoveFreesOccupancyTileForReuse) {
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);
  world.addPlayer(makePlayer(1, 3, 3));

  world.removePlayer(1);

  // Si el tile fue liberado, otro player puede ocuparlo
  EXPECT_NO_THROW(world.addPlayer(makePlayer(2, 3, 3)));
}

TEST_F(ExtractPlayerTest, RemovePreservesPlayerState) {
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);
  Player p = makePlayer(1, 3, 3);
  p.getInventory().addItem(makeWeaponWithId(10, 10, 10));
  world.addPlayer(std::move(p));

  // Dañamos al player antes de extraer
  world.getPlayer(1).takeDamage(30);
  int16_t hpBeforeExtract = world.getPlayer(1).getHp();

  auto extracted = world.removePlayer(1);

  EXPECT_EQ(extracted->getHp(), hpBeforeExtract);
  EXPECT_NE(extracted->getInventory().findItem(10), nullptr);
}

TEST_F(ExtractPlayerTest, RemoveNonExistentPlayerReturnsNullopt) {
  GameWorld world(makeWalkableMap(), npcFact, itemRepo, config);
  EXPECT_EQ(world.removePlayer(999), std::nullopt);
}